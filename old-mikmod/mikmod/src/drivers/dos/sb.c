/*

Name:  DRV_SB.C

Description:
 Mikmod driver for output on Creative Labs Soundblasters & compatibles
 (through DSP)

Portability:

 MSDOS:  BC(y)   Watcom(y)   DJGPP(y)
 Win95:  n
 Os2:    n
 Linux:  n

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include <dos.h>
#include <conio.h>
#ifndef __DJGPP__
#include <mem.h>
#endif

#include "mikmod.h"
#include "mirq.h"
#include "sb.h"


/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> Lowlevel SB stuff <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

//  Define some important SB i/o ports:

#define MIXER_ADDRESS       (sb_port + 0x4)
#define MIXER_DATA          (sb_port + 0x5)
#define DSP_RESET           (sb_port + 0x6)
#define DSP_READ_DATA       (sb_port + 0xa)
#define DSP_WRITE_DATA      (sb_port + 0xc)
#define DSP_WRITE_STATUS    (sb_port + 0xc)
#define DSP_DATA_AVAIL      (sb_port + 0xe)

UWORD sb_port;          // sb base port
unsigned int AWE32Base = 0x620;

void SB_MixerStereo(void)
// Enables stereo output for DSP versions 3.00 >= ver < 4.00
{
    outportb(MIXER_ADDRESS,0xe);
    outportb(MIXER_DATA,inportb(MIXER_DATA) | 2);
}


void SB_MixerMono(void)
// Disables stereo output for DSP versions 3.00 >= ver < 4.00
{
    outportb(MIXER_ADDRESS,0xe);
    outportb(MIXER_DATA,inportb(MIXER_DATA)&0xfd);
}


BOOL SB_WaitDSPWrite(void)
// Waits until the DSP is ready to be written to.
// returns FALSE on timeout
{
    UWORD timeout = 32767;

    while(timeout--)
        if(!(inportb(DSP_WRITE_STATUS) & 0x80)) return 1;
    return 0;
}


static BOOL SB_WaitDSPRead(void)
// Waits until the DSP is ready to read from.
// returns FALSE on timeout
{
    UWORD timeout = 32767;

    while(timeout--)
        if(inportb(DSP_DATA_AVAIL) & 0x80) return 1;
    return 0;
}


BOOL SB_WriteDSP(UBYTE data)
// Writes byte 'data' to the DSP.
// returns FALSE on timeout.
{
    if(!SB_WaitDSPWrite()) return 0;
    outportb(DSP_WRITE_DATA,data);
    return 1;
}



UWORD SB_ReadDSP(void)
// Reads a byte from the DSP.
// returns 0xffff on timeout.
{
    if(!SB_WaitDSPRead()) return 0xffff;
    return(inportb(DSP_READ_DATA));
}


void SB_SpeakerOn(void)
// Enables DAC speaker output.
{
    SB_WriteDSP(0xd1);
}


void SB_SpeakerOff(void)
// Disables DAC speaker output
{
    SB_WriteDSP(0xd3);
}


void SB_ResetDSP(void)
/*
    Resets the DSP.
*/
{
    int t;

    // reset the DSP by sending 1, (delay), then 0

    outportb(DSP_RESET,1);
    for(t=0; t<8; t++) inportb(DSP_RESET);
    outportb(DSP_RESET,0);
}



BOOL SB_Ping(void)
/*
    Checks if a SB is present at the current baseport by
    resetting the DSP and checking if it returned the value 0xaa.

    returns: TRUE   => SB is present
             FALSE  => No SB detected
*/
{
    SB_ResetDSP();
    return(SB_ReadDSP()==0xaa);
}



static UWORD SB_GetDSPVersion(void)

// Gets SB-dsp version. returns 0xffff if dsp didn't respond.

{
    UWORD hi,lo;

    if(!SB_WriteDSP(0xe1)) return 0xffff;

    hi=SB_ReadDSP();
    lo=SB_ReadDSP();

    return((hi<<8) | lo);
}



/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual SB driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

DMAMEM *SB_DMAMEM = NULL;
SBYTE  *SB_DMABUF;

UBYTE SB_TIMECONSTANT;

UWORD sb_int;           // interrupt vector that belongs to sb_irq
UWORD sb_ver;           // DSP version number
UBYTE sb_irq;           // sb irq
UBYTE sb_lodma;         // 8 bit dma channel (1.0/2.0/pro)
UBYTE sb_hidma;         // 16 bit dma channel (16/16asp)
UBYTE sb_dma;           // current dma channel
UBYTE sb_mode;

static UWORD sb_dmabufsize;   // DMA buffer size in bytes


BOOL SB_IsThere(void)
{
    CHAR        *envptr,c;
    static CHAR *endptr;

    sb_port   = 0xffff;
    sb_irq    = 0xff;
    sb_lodma  = 0xff;
    sb_hidma  = 0xff;
    AWE32Base = 0xffff;

    if((envptr = getenv("BLASTER"))==NULL) return 0;

    while(1)
    {   // skip whitespace
        do c = *(envptr++); while(c==' ' || c=='\t');

        // reached end of string? -> exit
        if(c==0) break;

        switch(c)
        {   case 'a':
            case 'A':
                sb_port = strtol(envptr,&endptr,16);
            break;

            case 'i':
            case 'I':
                sb_irq = strtol(envptr,&endptr,10);
            break;

            case 'd':
            case 'D':
                sb_lodma = strtol(envptr,&endptr,10);
            break;

            case 'h':
            case 'H':
                sb_hidma = strtol(envptr,&endptr,10);
            break;

            case 'e':
            case 'E':
                AWE32Base = strtol(envptr,&endptr,16);
            break;

            default:
                strtol(envptr,&endptr,16);
            break;
        }
        envptr = endptr;
    }

    if((sb_port==0xffff) || (sb_irq==0xff) || (sb_lodma==0xff)) return 0;

    // determine interrupt vector
    sb_int = (sb_irq > 7) ? (sb_irq + 104) : (sb_irq + 8);

    if(!SB_Ping()) return 0;

    // get dsp version.
    if((sb_ver = SB_GetDSPVersion())==0xffff) return 0;

    return 1;
}


static void interrupt newhandler(MIRQARGS)
{
    if(sb_irq == 7)
    {   // make sure it's a REAL IRQ!
        outportb(0x20,0xb);
        if(!(inportb(0x20) & 128)) return;
    }

    if(sb_ver < 0x200)
    {   SB_WriteDSP(0x14);
        SB_WriteDSP(0xff);
        SB_WriteDSP(0xfe);
    }

    if(sb_mode & DMODE_16BITS)
       inportb(sb_port + 0xf);
    else
       inportb(DSP_DATA_AVAIL);

    MIrq_EOI(sb_irq);
}


static PVI oldhandler;

BOOL SB_DMAreset(void)
{
    ULONG t, ctmp;

    if(sb_ver < 0x400)
    {   // DSP versions below 4.00 can't do 16 bit sound.
        md_mode &= ~DMODE_16BITS;
    }

    if(sb_ver < 0x300)
    {   // DSP versions below 3.00 can't do stereo sound.
        md_mode &= ~DMODE_STEREO;
    }


    // Use low dma channel for 8 bit, high dma for 16 bit
    sb_dma = (md_mode & DMODE_16BITS) ? sb_hidma : sb_lodma;

    if(sb_ver < 0x400)
    {   t = md_mixfreq;
        if(md_mode & DMODE_STEREO) t <<= 1;

        SB_TIMECONSTANT = 256 - (1000000L/t);

        if(sb_ver < 0x201)
        {   if(SB_TIMECONSTANT > 210) SB_TIMECONSTANT = 210;
        } else
        {   if(SB_TIMECONSTANT > 233) SB_TIMECONSTANT = 233;
        }

        md_mixfreq = 1000000L / (256-SB_TIMECONSTANT);
        if(md_mode & DMODE_STEREO) md_mixfreq >>= 1;
    }

    // Do the DMA buffer stuff - calculate the buffer size in bytes from
    // the specification in milliseconds.

    ctmp = md_mixfreq * ((md_mode & DMODE_STEREO) ? 2 : 1) * ((md_mode & DMODE_16BITS) ? 2 : 1);
    sb_dmabufsize = (ctmp * ((md_dmabufsize < 12) ? 12 : md_dmabufsize)) / 1000;

    // no larger than 32000, and make sure it's size is rounded to 16 bytes.

    if(sb_dmabufsize > 32000) sb_dmabufsize = 32000;
    sb_dmabufsize = (sb_dmabufsize + 15) & ~15;
    md_dmabufsize = (sb_dmabufsize * 1000) / ctmp;

    if(SB_DMAMEM != NULL) MDma_FreeMem(SB_DMAMEM);
    SB_DMAMEM = MDma_AllocMem(sb_dmabufsize);
    if(SB_DMAMEM==NULL) return 1;
    SB_DMABUF = (SBYTE *)MDma_GetPtr(SB_DMAMEM);

    sb_mode = md_mode;

    return 0;
}    


BOOL SB_Init(void)
{
    if((sb_ver >= 0x400) && (sb_hidma == 0xff))
    {   _mm_errno = MMERR_INITIALIZING_DRIVER;
        return 1;
    }

    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;

    if(SB_DMAreset()) return 1;
    if(VC_Init()) return 1;

    oldhandler = MIrq_SetHandler(sb_irq, newhandler);

    return 0;
}


BOOL SB_Reset(void)
{
    if(SB_DMAreset()) return 1;
    VC_Exit();

    return VC_Init();
}


void SB_Exit(void)
{
    MIrq_OnOff(sb_irq, 0);
    MIrq_SetHandler(sb_irq,oldhandler);

    if(SB_DMAMEM != NULL) MDma_FreeMem(SB_DMAMEM);
    SB_DMAMEM = NULL;

    VC_Exit();     SB_SpeakerOff();
    SB_ResetDSP(); SB_ResetDSP();
}


BOOL SB_CommonPlayStart(void)
{
    if(VC_PlayStart()) return 1;
    MIrq_OnOff(sb_irq, 1);

    // clear the dma buffer

    VC_SilenceBytes(SB_DMABUF, sb_dmabufsize);
    MDma_Commit(SB_DMAMEM, 0, sb_dmabufsize);
    MDma_Start(sb_dma,SB_DMAMEM,sb_dmabufsize,INDEF_WRITE);

    return 0;
}
    

static UWORD last = 0;
static UWORD curr = 0;


void SB_Update(void)
{
    UWORD todo, index;

    curr = ((sb_dmabufsize - MDma_Todo(sb_dma)) & 0xfffc);
    if(curr==last)  return;

    if(curr > last)
    {   todo  = curr-last; index = last;
        last += VC_WriteBytes(&SB_DMABUF[index],todo);
        MDma_Commit(SB_DMAMEM,index,todo);
        if(last >= sb_dmabufsize) last = 0;
    } else
    {   todo = sb_dmabufsize-last;
        VC_WriteBytes(&SB_DMABUF[last],todo);
        MDma_Commit(SB_DMAMEM, last, todo);
        last = VC_WriteBytes(SB_DMABUF, curr);
        MDma_Commit(SB_DMAMEM,0,curr);
    }
}


void SB_PlayStop(void)
{
    VC_PlayStop();
    SB_SpeakerOff();
    SB_ResetDSP();
    SB_ResetDSP();
    MDma_Stop(sb_dma);
    MIrq_OnOff(sb_irq,0);
}

