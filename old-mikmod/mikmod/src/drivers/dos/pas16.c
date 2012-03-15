/*

Name:
DRV_PAS.C

Description:
Mikmod driver for output on Pro Audio Spectrum 16 & compatibles

Copyright 1996 by Arnout "Whisko" Cosman <A.P.Cosman@twi.tudelft.nl>.
Distribution is allowed only as part of the MikMod package.

#define std_disclaimer \
"THIS DRIVER IS PROVIDED BY THE AUTHOR `AS IS', WITHOUT WARRANTY OF ANY "\
"KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED "\
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. "\
"IN NO EVENT SHALL THE AUTHOR BE LIABLE and so on and so forth."

Portability:

MSDOS:  BC(?)   Watcom(y)       DJGPP(y)
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
#include "mdma.h"


/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> Lowlevel PAS stuff <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static UWORD pas_port;          // PAS base port
static UWORD translat_code;
static UWORD pas_ver;           // version number indicates soundcard type
static UWORD pas_dmabufsize;    // dma buffer size in bytes

// 1=PAS+, 2=CDPC, 3=PAS16, 4=PAS16D

UBYTE O_M_1_to_card[] =
    { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 4, 0, 2, 3 };
UBYTE I_C_2_to_DMA[] =
    { 4, 1, 2, 3, 0, 5, 6, 7 };

/*
 * The 'BaseList' below states a list of likely base addresses for PAS cards
 * 'NUMBASES' tells how many of these addresses are actually checked.
 * By default, only the first base address is checked, to minimize the
 * chance of conflicts with other cards.
 */
#define NUMBASES 1
UWORD BaseList[] =
    { 0x388, 0x384, 0x38C, 0x288, 0x280, 0x284, 0x28C };

//  Define some important PAS i/o ports:

#define INT_STAT        0x0B89  // Interrupt Status
#define FILT_FREQ       0x0B8A  // Filter Frequency Control
#define INT_MASK        0x0B8B  // Interrupt Mask
#define PCM_CTRL        0x0F8A  // PCM Control
#define S_RATE_TIMER    0x1388  // Sample Rate Timer 
#define S_BUFFER_CNT    0x1389  // Sample Buffer Count
#define S_CNT_CTRL      0x138B  // Sample Counter Control
#define SYS_CFG_2       0x8389  // System Configuration 2
#define OPR_MODE_1      0xEF8B  // Operation Mode Control 1
#define IO_CFG_2        0xF389  // IO Configuration Control 2
#define IO_CFG_3        0xF38A  // IO Configuration Control 3

#define PAS_DEFAULT_BASE        0x388   // Default Base Address

// Interrupt Status masks
#define PCM_S_BUFFER_IRQ        0x08

// Filter Frequency Control masks
#define PCM_RATE_CNT            0x40
#define PCM_BUFFER_CNT          0x80

// Sample Counter Control masks
#define SQUARE_WAVE             0x04
#define LSB_THEN_MSB            0x30
#define S_BUFFER                0x40

// System Configuration 2 masks
#define PCM_16_BIT              0x04

// Operation Mode Control 1 masks
#define PCM_TYPE                0x08

// PCM Control masks
#define PCM_DAC_MODE            0x10
#define PCM_MONO                0x20
#define PCM_ENABLE              0x40
#define PCM_DMA_ENABLE          0x80
#define MIXER_CROSS_R_TO_R      0x01
#define MIXER_CROSS_L_TO_L      0x08

void pas_outportb(UWORD portno, UBYTE value)
/*
 * Translates outportb to correct base address
 */
{
    outportb(portno^translat_code, value);
}

UBYTE pas_inportb(UWORD portno)
/*
 * Translates inportb to correct base address
 */
{
    return inportb(portno^translat_code);
}


static void PAS_ResetPCM(void)
/*
    Resets PCM.
*/
{
    pas_outportb(FILT_FREQ, pas_inportb(FILT_FREQ) & ~(PCM_RATE_CNT|PCM_BUFFER_CNT));
    pas_outportb(PCM_CTRL, pas_inportb(PCM_CTRL) & ~PCM_ENABLE);
    if (md_mode & DMODE_16BITS)
        pas_outportb(SYS_CFG_2, pas_inportb(SYS_CFG_2) & ~PCM_16_BIT);
}

/*
static void PAS_PausePCM(void)
// Pause PCM. (Currently unused)
{
    pas_outportb(FILT_FREQ, pas_inportb(FILT_FREQ) & ~PCM_RATE_CNT);
}

static void PAS_ResumePCM(void)
// Resume PCM after pause. (Currently unused)
{
    pas_outportb(FILT_FREQ, pas_inportb(FILT_FREQ) | PCM_RATE_CNT);
}
*/


static BOOL PAS_Ping(void)
/*
    Checks if a PAS is present at the current baseport.

    returns: TRUE   => PAS is present
             FALSE  => No PAS detected
*/
{
    UBYTE board_id, testval;

    translat_code=PAS_DEFAULT_BASE ^ pas_port;
    board_id=pas_inportb(INT_MASK);
    if (board_id == 0xFF)
        return 0;

    // Check for PAS2 series

    testval = board_id^0xE0;
    pas_outportb(INT_MASK, testval);
    testval = pas_inportb(INT_MASK);
    pas_outportb(INT_MASK, board_id);

    if (board_id != testval)              // If change succesful: no PAS2
        return 0;

    pas_ver=O_M_1_to_card[pas_inportb(OPR_MODE_1) & 0x0F];
    if (!pas_ver)
        return 0;

    PAS_ResetPCM();
    return 1;
}



/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual PAS driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static DMAMEM *PAS_DMAMEM;
static SBYTE  *PAS_DMABUF;

static UBYTE pas_dma;           // current dma channel


static BOOL PAS_IsThere(void)
{
    UWORD i;

    for(i=0; i<NUMBASES; i++)
    {   pas_port=BaseList[i];
        if (!PAS_Ping())
            pas_port=0xffff;
        else
            break;
    }

    if(pas_port==0xffff)
        return 0;

    pas_dma = I_C_2_to_DMA[pas_inportb(IO_CFG_2) & 0x07];

    if (pas_dma==0) return 0;

    return 1;
}


static BOOL PAS_Init(void)
{
    ULONG speed, ctmp;
    UBYTE filter, tmp;

    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;

    if (!(pas_inportb(OPR_MODE_1) & PCM_TYPE))
    {   // 8 bit DMA-channel or 16 bit not supported: do 8 bit sound.
        md_mode &= ~DMODE_16BITS;
    }
    
    pas_outportb(FILT_FREQ, PCM_BUFFER_CNT | PCM_RATE_CNT | 0x21);
    pas_outportb(PCM_CTRL, PCM_DMA_ENABLE | PCM_MONO | PCM_DAC_MODE | MIXER_CROSS_L_TO_L | MIXER_CROSS_R_TO_R);
    pas_outportb(FILT_FREQ, 0x21);

// Set 8 bit/16 bit

    tmp = pas_inportb(SYS_CFG_2);
    if (md_mode&DMODE_16BITS)
        tmp|=PCM_16_BIT;
    else
        tmp&=(~PCM_16_BIT);
    pas_outportb(SYS_CFG_2, tmp);

// Set channels (mono/stereo)

    tmp = pas_inportb(PCM_CTRL);
    if(md_mode & DMODE_STEREO)
        tmp &= (~PCM_MONO);
    else
        tmp |= PCM_MONO;
    pas_outportb(PCM_CTRL, tmp);

// Set speed

    if (md_mixfreq > 44100)
        md_mixfreq = 44100;               // That's as high as it goes
    if (md_mixfreq < 5000)
        md_mixfreq = 5000;                // That's as low as it goes :-)

    speed = (1193180L+(((ULONG)md_mixfreq)>>1)) / md_mixfreq;
    if (md_mode & DMODE_STEREO)
    {   speed = speed>>1;
        md_mixfreq = 596590L/speed;
    } else
        md_mixfreq = 1193180L/speed;

// Set anti-aliasing filters according to sample rate.

    filter = pas_inportb(FILT_FREQ);
    filter&=0xe0;
    if (md_mixfreq >= 35794)
        filter|=0x21;
    else if (md_mixfreq >= 31818)
        filter|=0x22;
    else if (md_mixfreq >= 23862)
        filter|=0x29;
    else if (md_mixfreq >= 17896)
        filter|=0x31;
    else if (md_mixfreq >= 11930)
        filter|=0x39;
    else if (md_mixfreq >= 5964)
        filter |= 0x24;
    else
        filter |= 0x37;

    pas_outportb(FILT_FREQ, filter & ~(PCM_RATE_CNT | PCM_BUFFER_CNT));
    pas_outportb(S_CNT_CTRL, LSB_THEN_MSB | SQUARE_WAVE);
    pas_outportb(S_RATE_TIMER, speed & 0xff);
    pas_outportb(S_RATE_TIMER, (speed >> 8) & 0xff);
    pas_outportb(FILT_FREQ, filter);

    if(VC_Init()) return 1;

    // Do the DMA buffer stuff - calculate the buffer size in bytes from
    // the specification in milliseconds.

    ctmp = md_mixfreq * ((md_mode & DMODE_STEREO) ? 2 : 1) * ((md_mode & DMODE_16BITS) ? 2 : 1);
    pas_dmabufsize = (ctmp * ((md_dmabufsize < 12) ? 12 : md_dmabufsize)) / 1000;

    // no larger than 32000, and make sure it's size is rounded to 16 bytes.

    if(pas_dmabufsize > 32000) pas_dmabufsize = 32000;
    pas_dmabufsize = (pas_dmabufsize+15) & ~15;
    md_dmabufsize = (pas_dmabufsize*1000) / ctmp;

    PAS_DMAMEM = MDma_AllocMem(pas_dmabufsize);

    if(PAS_DMAMEM==NULL)
    {   VC_Exit();
        return 1;
    }

    PAS_DMABUF = (SBYTE *)MDma_GetPtr(PAS_DMAMEM);

    return 0;
}


static void PAS_Exit(void)
{
    MDma_FreeMem(PAS_DMAMEM);
    VC_Exit();
}


static UWORD last = 0;
static UWORD curr = 0;


static void PAS_Update(void)
{
    UWORD todo,index;

    curr = (pas_dmabufsize-MDma_Todo(pas_dma))&0xfffc;
    if(curr==last) return;

    if(curr > last)
    {   todo = curr-last; index = last;
        last+=VC_WriteBytes(&PAS_DMABUF[index],todo);
        MDma_Commit(PAS_DMAMEM,index,todo);
        if(last >= pas_dmabufsize) last = 0;
    } else
    {   todo = pas_dmabufsize-last;
        VC_WriteBytes(&PAS_DMABUF[last],todo);
        MDma_Commit(PAS_DMAMEM,last,todo);
        last = VC_WriteBytes(PAS_DMABUF,curr);
        MDma_Commit(PAS_DMAMEM,0,curr);
    }
}


// pcm_output_block
static BOOL PAS_PlayStart(void)
{
    if(VC_PlayStart()) return 1;

    // clear the dma buffer

    VC_SilenceBytes(PAS_DMABUF,pas_dmabufsize);
    MDma_Commit(PAS_DMAMEM,0,pas_dmabufsize);

    if(!MDma_Start(pas_dma,PAS_DMAMEM,pas_dmabufsize,INDEF_WRITE)) return 1;

    pas_outportb(FILT_FREQ, pas_inportb(FILT_FREQ) & ~PCM_BUFFER_CNT);
    pas_outportb(S_CNT_CTRL, S_BUFFER | LSB_THEN_MSB | SQUARE_WAVE);
    pas_outportb(S_BUFFER_CNT, 0xff);
    pas_outportb(S_BUFFER_CNT, 0xff);
    pas_outportb(FILT_FREQ, pas_inportb(FILT_FREQ) | PCM_RATE_CNT);
    pas_outportb(PCM_CTRL, pas_inportb(PCM_CTRL) | PCM_ENABLE | PCM_DAC_MODE);

    return 0;
}


static void PAS_PlayStop(void)
{
    VC_PlayStop();
    PAS_ResetPCM();
    MDma_Stop(pas_dma);
}


MDRIVER drv_pas =
{   "PAS 16 & compatibles",
    "Pro Audio Spectrum 16 Driver v0.2 - By Whisko",
    0,255,

    NULL,
    PAS_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    PAS_Init,
    PAS_Exit,
    NULL,
    VC_SetNumVoices,
    VC_GetActiveVoices,
    PAS_PlayStart,
    PAS_PlayStop,
    PAS_Update,
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

