/*

Name:  WSS.C

Description:
 Mikmod driver for output on Microsoft Windows Sound System & compatibles

 written by Mario Koeppen
 mk2@irz.inf.tu-dresden.de
 http://www.inf.tu-dresden.de/~mk2

 derived from drv_sb.c and gus sdk examples

Portability:

 MSDOS:  BC(?)   Watcom(y)       DJGPP(y)
 Win95:  BC(?)
 Linux:  ?

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include <string.h>
#include <dos.h>
#include <conio.h>
#include <mem.h>

#include "mikmod.h"
#include "mdma.h"
#include "mirq.h"


/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> Lowlevel WSS stuff <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static UWORD wss_port;        // wss base port

/*
        Define some important WSS i/o ports:
*/

#define CODEC_ADDR (wss_port+4)
#define CODEC_DATA (wss_port+5)
#define CODEC_STATUS (wss_port+6)
#define CODEC_PIO (wss_port+7)

/* Definitions for CODEC_ADDR register */
/* Bits 0-3. Select an internal register to read/write */
#define LEFT_INPUT      0x00            /* Left input control register */
#define RIGHT_INPUT     0x01            /* RIght input control register */
#define GF1_LEFT_INPUT  0x02            /* Left Aux #1 input control */
#define GF1_RIGHT_INPUT 0x03            /* Right Aux #1 input control */
#define CD_LEFT_INPUT   0x04            /* Left Aux #2 input control */
#define CD_RIGHT_INPUT  0x05            /* Right Aux #2 input control */
#define LEFT_OUTPUT     0x06            /* Left output control */
#define RIGHT_OUTPUT    0x07            /* Right output control */
#define PLAYBK_FORMAT   0x08            /* Clock and data format */
#define IFACE_CTRL      0x09            /* Interface control */
#define PIN_CTRL        0x0a            /* Pin control */
#define TEST_INIT       0x0b            /* Test and initialization */
#define MISC_INFO       0x0c            /* Miscellaneaous information */
#define LOOPBACK        0x0d            /* Digital Mix */
#define PLY_UPR_CNT     0x0e            /* Playback Upper Base Count */
#define PLY_LWR_CNT     0x0f            /* Playback Lower Base Count */

/* Definitions for CODEC_ADDR register */
#define CODEC_INIT      0x80            /* CODEC is initializing */
#define CODEC_MCE       0x40            /* Mode change enable */
#define CODEC_TRD       0x20            /* Transfer Request Disable */
/* bits 3-0 are indirect register address (0-15) */

/* Definitions for CODEC_STATUS register */
#define CODEC_CUL       0x80            /* Capture data upper/lower byte */
#define CODEC_CLR       0x40            /* Capture left/right sample */
#define CODDEC_CRDY     0x20            /* Capture data read */
#define CODEC_SOUR      0x10            /* Playback over/under run error */
#define CODEC_PUL       0x08            /* Playback upper/lower byte */
#define CODEC_PLR       0x04            /* Playback left/right sample */
#define CODEC_PRDY      0x02            /* Playback data register read */
#define CODEC_INT       0x01            /* interrupt status */

/* Definitions for Left output level register */
#define MUTE_OUTPUT     0x80            /* Mute this output source */
/* bits 5-0 are left output attenuation select (0-63) */
/* bits 5-0 are right output attenuation select (0-63) */

/* Definitions for clock and data format register */
#define BIT_16_LINEAR   0x40    /* 16 bit twos complement data */
#define BIT_8_LINEAR    0x00    /* 8 bit unsigned data */
#define TYPE_STEREO     0x10    /* stero mode */
/* Bits 3-1 define frequency divisor */
#define XTAL1           0x00    /* 24.576 crystal */
#define XTAL2           0x01    /* 16.9344 crystal */

/* Definitions for interface control register */
#define CAPTURE_PIO     0x80    /* Capture PIO enable */
#define PLAYBACK_PIO    0x40    /* Playback PIO enable */
#define AUTOCALIB       0x08    /* auto calibrate */
#define SINGLE_DMA      0x04    /* Use single DMA channel */
#define PLAYBACK_ENABLE 0x01    /* playback enable */

/* Definitions for Pin control register */
#define IRQ_ENABLE      0x02    /* interrupt enable */
#define XCTL1           0x40    /* external control #1 */
#define XCTL0           0x80    /* external control #0 */

#define CALIB_IN_PROGRESS 0x20  /* auto calibrate in progress */


/*static BOOL WSS_WaitCODEC (void)

        Waits until the DSP is ready.

        returns FALSE on timeout

{
        UWORD timeout=32767;

        while (timeout--)
        {
                if(!(inportb (CODEC_ADDR) & 0x80)) return 1;
        }
        return 0;
}
*/

static void WSS_WriteCODECReg (UBYTE reg, UBYTE data)
/*
        Writes byte 'data' to CODEC register 'reg'.
*/
{
        UBYTE oldreg;

        oldreg = inportb (CODEC_ADDR);
        outportb (CODEC_ADDR, (oldreg & 0xf0) | (reg & 0x0f));
        outportb (CODEC_DATA, data);
        outportb (CODEC_ADDR, oldreg);
}



static UBYTE WSS_ReadCODECReg (UBYTE reg)
/*
        Reads a byte from CODEC register 'reg'.
*/
{
        UBYTE oldreg, retval;

        oldreg = inportb (CODEC_ADDR);
        outportb (CODEC_ADDR, (oldreg & 0xf0) | (reg & 0x0f));
        retval = inportb (CODEC_DATA);
        outportb (CODEC_ADDR, oldreg);
        return retval;
}



/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual WSS driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/


static DMAMEM *WSS_DMAMEM;
static SBYTE  *WSS_DMABUF;

static UWORD wss_int;           // interrupt vector that belongs to wss_irq
static UBYTE wss_irq;           // wss irq
static UBYTE wss_dma;           // current dma channel
static UWORD wss_bufsamples, wss_dmabufsize;

static BOOL WSS_IsThere(void)
{
        CHAR *envptr,c;
        static CHAR *endptr;

        wss_port = 0xffff;
        wss_irq = 0xff;
        wss_dma = 0xff;

        if((envptr = getenv("WSS"))==NULL) return 0;

        while (1)
        {
                // skip whitespace
                do c=*(envptr++); while(c==' ' || c=='\t');

                // reached end of string? -> exit
                if (c==0) break;

                switch(c)
                {       case 'a':
                        case 'A':
                                wss_port = strtol(envptr,&endptr,16);
                                break;

                        case 'i':
                        case 'I':
                                wss_irq = strtol(envptr,&endptr,10);
                                break;

                        case 'd':
                        case 'D':
                                wss_dma = strtol(envptr,&endptr,10);
                                break;

                        default:
                                strtol(envptr,&endptr,16);
                                break;
                }
                envptr = endptr;
        }

        if(wss_port==0xffff || wss_irq==0xff || wss_dma==0xff) return 0;

        // determine interrupt vector

        wss_int = (wss_irq>7) ? wss_irq+104 : wss_irq+8;

        return 1;
}


static void 
#ifndef __DJGPP__
interrupt 
#endif
newhandler(void)
{
    WSS_WriteCODECReg (PLY_LWR_CNT, wss_bufsamples & 0xff);
    WSS_WriteCODECReg (PLY_UPR_CNT, wss_bufsamples >> 8); 
    MIrq_EOI(wss_irq);
}

#ifdef __DJGPP__
static void EOnewhandler() { }
#endif

void SetPlaybackFormat (UBYTE FormatFreq)
{
    disable (); 

    outportb (CODEC_ADDR, CODEC_MCE | PLAYBK_FORMAT);
    outportb (CODEC_DATA, FormatFreq);
    inportb (CODEC_DATA);           // ERRATA SHEETS ...
    inportb (CODEC_DATA);           // ERRATA SHEETS ...

    // wait till sync is done ...
    while(inportb (CODEC_DATA) & CODEC_INIT);

    // turn off the MCE bit
    outportb (CODEC_ADDR, PLAYBK_FORMAT);

    // Need this. outp doesn't always take ...
    while (inportb (CODEC_ADDR) != PLAYBK_FORMAT)
        outportb (CODEC_ADDR, PLAYBK_FORMAT);

    outportb (CODEC_ADDR, TEST_INIT);

    while(inportb (CODEC_ADDR) != TEST_INIT)
        outportb (CODEC_ADDR, TEST_INIT);

    while(inportb (CODEC_ADDR) & CALIB_IN_PROGRESS)
        outportb (CODEC_ADDR, TEST_INIT);

    enable (); 
}


ULONG MatchRate (UWORD *InRate)
{
    ULONG Rates [14][2] =
    {{5510, 1}, {6620, 15}, {8000, 0}, {9600, 14}, {11025, 3},
     {16000, 2}, {18900, 5}, {22050, 7}, {27420, 4}, {32000, 6},
     {33075, 13}, {37800, 9}, {44100, 11}, {48000, 12}};
    ULONG D, DOld = 48000, R = 0, N, Rate = *InRate;

    for (N = 0; N < 14; N++)
{
        D = abs (Rates [N][0] - Rate);
        if (D < DOld)
        {
            R = N;
            DOld = D;
        }
    }
    *InRate = Rates [R][0];
    return Rates [R][1];
}


static PVI oldhandler;

static BOOL WSS_Init (void)
{
    UBYTE format;
    int   ctmp;

    if(!WSS_IsThere())
    {   _mm_errno = MMERR_INITIALIZING_DRIVER;
        return 1;
    }

    format = MatchRate (&md_mixfreq);

    // Do the DMA buffer stuff - calculate the buffer size in bytes from
    // the specification in milliseconds.

    ctmp = md_mixfreq * ((md_mode & DMODE_STEREO) ? 2 : 1) * ((md_mode & DMODE_16BITS) ? 2 : 1);
    wss_dmabufsize = (ctmp * ((md_dmabufsize < 12) ? 12 : md_dmabufsize)) / 1000;

    // no larger than 32000, and make sure it's size is rounded to 16 bytes.

    if(wss_dmabufsize > 32000) wss_dmabufsize = 32000;
    wss_dmabufsize = (wss_dmabufsize+15) & ~15;
    md_dmabufsize = (wss_dmabufsize*1000) / ctmp;

    if(md_mode & DMODE_16BITS)
    {   wss_bufsamples >>= 1;
        format |= BIT_16_LINEAR;
    }

    if(md_mode & DMODE_STEREO)
    {   wss_bufsamples >>= 1;
        format |= TYPE_STEREO;
    }

    SetPlaybackFormat(format);

    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
    if(VC_Init())  return 1;

    WSS_DMAMEM = MDma_AllocMem(wss_dmabufsize);

    if(WSS_DMABUF == NULL)
    {   VC_Exit();
        return 1;
    }

    WSS_DMABUF = (SBYTE *)MDma_GetPtr(WSS_DMAMEM);
    oldhandler = MIrq_SetHandler (wss_irq, newhandler);
    return 0;
}


static void WSS_Exit(void)
{
    MIrq_SetHandler(wss_irq,oldhandler);
    MDma_FreeMem(WSS_DMAMEM);
    VC_Exit();
}


static UWORD last = 0;
static UWORD curr = 0;


static void WSS_Update (void)
{
    UWORD todo, index;

    curr = (wss_dmabufsize - MDma_Todo(wss_dma)) & 0xfffc;
    if(curr==last) return;
        
    if (curr > last)
    {   todo = curr-last;  index = last;
        last += VC_WriteBytes(&WSS_DMABUF[index], todo);
        MDma_Commit(WSS_DMAMEM, index, todo);
        if(last >= wss_dmabufsize)  last=0;
    } else
    {   todo = wss_dmabufsize-last;
        VC_WriteBytes(&WSS_DMABUF [last], todo);
        MDma_Commit(WSS_DMAMEM,last,todo);
        last = VC_WriteBytes (WSS_DMABUF, curr);
        MDma_Commit(WSS_DMAMEM,0,curr);            
    }
}


static BOOL WSS_PlayStart (void)
{
    if(VC_PlayStart()) return 1;
    MIrq_OnOff(wss_irq,1);

    // clear the dma buffer
    VC_SilenceBytes(WSS_DMABUF,wss_dmabufsize);
    MDma_Commit(WSS_DMAMEM,0,wss_dmabufsize);

    if(!MDma_Start(wss_dma, WSS_DMAMEM, wss_dmabufsize, INDEF_WRITE)) return 1;

    outportb (CODEC_ADDR, IFACE_CTRL);
    outportb (CODEC_DATA, inportb (CODEC_DATA) | PLAYBACK_ENABLE);
        
//        WSS_WriteCODECReg (IFACE_CTRL, WSS_ReadCODECReg (IFACE_CTRL) | PLAYBACK_ENABLE);

    return 0;
}


static void WSS_PlayStop (void)
{
    VC_PlayStop();
    outportb (CODEC_ADDR, IFACE_CTRL);
    outportb (CODEC_DATA, inportb (CODEC_DATA) & ~PLAYBACK_ENABLE);
//        WSS_WriteCODECReg (IFACE_CTRL, WSS_ReadCODECReg (IFACE_CTRL) & ~PLAYBACK_ENABLE);
    MDma_Stop(wss_dma);
    MIrq_OnOff(wss_irq, 0);
}


MDRIVER drv_wss =
{   "Windows Sound System (WSS) or compatible",
    "Windows Sound System (WSS) Driver v0.2",
    0,255,

    NULL,
    WSS_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    WSS_Init,
    WSS_Exit,
    NULL,
    VC_SetNumVoices,
    WSS_PlayStart,
    WSS_PlayStop,
    WSS_Update,
    VC_VoiceSetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceSetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

