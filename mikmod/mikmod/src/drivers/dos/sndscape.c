/*

Name: SNDSCAPE.C

Description:
 Mikmod driver for output on Ensoniq Soundscape / Soundscape ELITE

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
#include <string.h>

#include "mikmod.h"
#include "mdma.h"
#include "mirq.h"

/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> Lowlevel SS stuff <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

/* Ensoniq gate-array chip defines ... */

#define ODIE            0       /* ODIE gate array */
#define OPUS            1       /* OPUS gate array */
#define MMIC            2       /* MiMIC gate array */

/* relevant direct register defines - offsets from base address */
#define GA_HOSTCTL_OFF  2       /* host port ctrl/stat reg */
#define GA_ADDR_OFF     4       /* indirect address reg */
#define GA_DATA_OFF     5       /* indirect data reg */
#define GA_CODEC_OFF    8       /* for some boards CoDec is fixed from base */

/* relevant indirect register defines */
#define GA_DMAB_REG     3       /* DMA chan B assign reg */
#define GA_INTCFG_REG   4       /* interrupt configuration reg */
#define GA_DMACFG_REG   5       /* DMA configuration reg */
#define GA_CDCFG_REG    6       /* CD-ROM/CoDec config reg */
#define GA_HMCTL_REG    9   /* host master control reg */


/* AD-1848 or compatible CoDec defines ... */
/* relevant direct register defines - offsets from base */
#define CD_ADDR_OFF     0       /* indirect address reg */
#define CD_DATA_OFF     1       /* indirect data reg */
#define CD_STATUS_OFF   2       /* status register */

/* relevant indirect register defines */
#define CD_ADCL_REG     0   /* left DAC input control reg */
#define CD_ADCR_REG     1   /* right DAC input control reg */
#define CD_CDAUXL_REG   2       /* left DAC output control reg */
#define CD_CDAUXR_REG   3       /* right DAC output control reg */
#define CD_DACL_REG     6       /* left DAC output control reg */
#define CD_DACR_REG     7       /* right DAC output control reg */
#define CD_FORMAT_REG   8       /* clock and data format reg */
#define CD_CONFIG_REG   9       /* interface config register */
#define CD_PINCTL_REG   10      /* external pin control reg */
#define CD_UCOUNT_REG   14      /* upper count reg */
#define CD_LCOUNT_REG   15      /* lower count reg */
#define CD_XFORMAT_REG  28  /* extended format reg - 1845 record */
#define CD_XUCOUNT_REG  30  /* extended upper count reg - 1845 record */
#define CD_XLCOUNT_REG  31  /* extended lower count reg - 1845 record */

#define CD_MODE_CHANGE  0x40    /* mode change mask for addr reg */



/****************************************************************************
hardware config info ...
****************************************************************************/

static UWORD    BasePort;   /* Gate Array/MPU-401 base port */
static UWORD    MidiIrq;    /* the MPU-401 IRQ */
static UWORD    WavePort;   /* the AD-1848 base port */
static UWORD    WaveIrq;    /* the PCM IRQ */
static UWORD    DmaChan;    /* the PCM DMA channel */

/****************************************************************************
all kinds of stuff ...
****************************************************************************/

static UWORD Windx;     /* Wave IRQ index - for reg writes */
static UWORD Mindx;     /* MIDI IRQ index - for reg writes */

static UBYTE IcType;        /* the Ensoniq chip type */
static UBYTE CdCfgSav;      /* gate array register save area */
static UBYTE DmaCfgSav;     /* gate array register save area */
static UBYTE IntCfgSav;     /* gate array register save area */

static UWORD const SsIrqs[4] = { 9, 5, 7, 10 };  /* Soundscape IRQs */
static UWORD const RsIrqs[4] = { 9, 7, 5, 15 };  /* an older IRQ set */
static UWORD const *Irqs;           /* pointer to one of the IRQ sets */

static UBYTE DacSavL;        /* DAC left volume save */
static UBYTE DacSavR;        /* DAC right volume save */
static UBYTE CdxSavL;        /* CD/Aux left volume save */
static UBYTE CdxSavR;        /* CD/Aux right volume save */
static UBYTE AdcSavL;        /* ADC left volume save */
static UBYTE AdcSavR;        /* ADC right volume save */

static DMAMEM *SS_DMAMEM;
static SBYTE  *SS_DMABUF;
static SWORD  ss_dmabufsize;

/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual SS driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/



UBYTE GaRead(UWORD rnum)
/*
  This function is used to read the indirect addressed registers in the
  Ensoniq Soundscape gate array.

  INPUTS:
    rnum  - the numner of the indirect register to be read

  RETURNS:
    the contents of the indirect register are returned
*/
{
    outportb(BasePort + GA_ADDR_OFF, rnum);
    return inportb(BasePort + GA_DATA_OFF);
}



void GaWrite(UWORD rnum,UBYTE value)
/*
  This function is used to write the indirect addressed registers in the
  Ensoniq Soundscape gate array.

  INPUTS:
    rnum   - the numner of the indirect register to be read
    value  - the byte value to be written to the indirect register

  RETURNS:
    Nothing
*/
{
    outportb(BasePort + GA_ADDR_OFF, rnum);
    outportb(BasePort + GA_DATA_OFF, value);
}


UBYTE CdRead(UWORD rnum)
/*
  This function is used to read the indirect addressed registers in the
  AD-1848 or compatible CoDec. It will preserve the special function bits
  in the upper-nibble of the indirect address register.

  INPUTS:
    rnum  - the numner of the indirect register to be read

  RETURNS:
    the contents of the indirect register are returned

*/
{
    outportb(WavePort + CD_ADDR_OFF,
        (inportb(WavePort + CD_ADDR_OFF) & 0xf0) | rnum);
    return inportb(WavePort+CD_DATA_OFF);
}



void CdWrite(UWORD rnum,UBYTE value)
/*
  This function is used to write the indirect addressed registers in the
  Ad-1848 or compatible CoDec. It will preserve the special function bits
  in the upper-nibble of the indirect address register.

  INPUTS:
    rnum   - the numner of the indirect register to be read
    value  - the byte value to be written to the indirect register

  RETURNS:
    Nothing
*/
{
    outportb(WavePort + CD_ADDR_OFF,
        (inportb(WavePort + CD_ADDR_OFF) & 0xf0) | rnum);
    outportb(WavePort + CD_DATA_OFF, value);
}




void SetDacVol(UBYTE lvol,UBYTE rvol)
/*
  This function sets the left and right DAC output level in the CoDec.

  INPUTS:
    lvol  - left volume, 0-127
    rvol  - right volume, 0-127

  RETURNS:
    Nothing

*/
{
    CdWrite(CD_DACL_REG, ~(lvol >> 1) & 0x3f);
    CdWrite(CD_DACR_REG, ~(rvol >> 1) & 0x3f);
}



void SetCdRomVol(UBYTE lvol,UBYTE rvol)
/*
  This function sets the left and right CD-ROM output level in the CoDec.

  INPUTS:
    lvol  - left volume, 0-127
    rvol  - right volume, 0-127

  RETURNS:
    Nothing
*/
{
    CdWrite(CD_CDAUXL_REG, ~(lvol >> 2) & 0x1f);
    CdWrite(CD_CDAUXR_REG, ~(rvol >> 2) & 0x1f);
}


void SetAdcVol(UBYTE lvol,UBYTE rvol)
/*
  This function sets the left and right ADC input level in the CoDec.

  INPUTS:
    lvol  - left volume, 0-127
    rvol  - right volume, 0-127

  RETURNS:
    Nothing

*/
{
    CdWrite(CD_ADCL_REG, (CdRead(CD_ADCL_REG) & 0xf0) | (lvol & 0x7f) >> 3);
    CdWrite(CD_ADCR_REG, (CdRead(CD_ADCR_REG) & 0xf0) | (rvol & 0x7f) >> 3);
}


void StopCoDec(void)
{
    //UWORD i;

    CdWrite(CD_CONFIG_REG,CdRead(CD_CONFIG_REG)&0xfc);

    /* Let the CoDec receive its last DACK(s). The DMAC must not be */
    /*  masked while the CoDec has DRQs pending. */
/*  for(i=0; i<256; ++i )
        if(!(inportb(DmacRegP->status) & (0x10 << DmaChan))) break;
*/
}



BOOL GetConfigEntry(char *entry, char *dest, FILE *fp)
/*
  This function parses a file (SNDSCAPE.INI) for a left-hand string and,
  if found, writes its associated right-hand value to a destination buffer.
  This function is case-insensitive.

  INPUTS:
    fp  - a file pointer to the open SNDSCAPE.INI config file
    dst - the destination buffer pointer
    lhp - a pointer to the right-hand string

  RETURNS:
    1   - if successful
    0   - if the right-hand string is not found or has no equate
*/
{
    static CHAR str[83];
    static CHAR tokstr[33];
    CHAR *p;

    // make a local copy of the entry, upper-case it
    strcpy(tokstr, entry);
    strupr(tokstr);

    // rewind the file and try to find it ...
    rewind(fp);

    for( ;; )
    {   // get the next string from the file

        fgets(str, 83, fp);
        if(feof(fp)) return 0;

        /* properly terminate the string */
        for( p = str; *p != '\0'; ++p ) {
            if( *p == ' ' || *p == '\t' || *p == 0x0a || *p == 0x0d ) {
                *p = '\0';
                break;
            }
        }

        /* see if it's an 'equate' string; if so, zero the '=' */
        if( !(p = strchr(str, '=')) ) continue;
        *p = '\0';

        /* upper-case the current string and test it */
        strupr(str);
        if( strcmp(str, tokstr) )
            continue;

        /* it's our string - copy the right-hand value to buffer */
        for( p = str + strlen(str) + 1; (*dest++ = *p++) != '\0'; );
        break;
    }
    return 1;
}


static BOOL SS_IsThere(void)
{
    static CHAR str[78];
    CHAR *envptr;
    FILE *fp;
    UBYTE tmp;

    if((envptr=getenv("SNDSCAPE"))==NULL) return 0;

    strcpy(str, envptr);
    if( str[strlen(str) - 1] == '\\' )
        str[strlen(str) - 1] = '\0';

    strcat(str, "\\SNDSCAPE.INI");

    if(!(fp=fopen(str, "r"))) return 0;

    // read all of the necessary config info ...
    if(!GetConfigEntry("Product",str,fp))
    {   fclose(fp);
        return 0;
    }

    // if an old product name is read, set the IRQs accordingly
    strupr(str);
    if(strstr(str,"SOUNDFX") || strstr(str,"MEDIA_FX"))
        Irqs = RsIrqs;
    else
        Irqs = SsIrqs;

    if(!GetConfigEntry("Port", str, fp))
    {   fclose(fp);
        return 1;
    }

    BasePort=strtol(str,NULL,16);

    if(!GetConfigEntry("WavePort",str,fp))
    {   fclose(fp);
        return 0;
    }

    WavePort=strtol(str,NULL,16);

    if(!GetConfigEntry("IRQ",str,fp))
    {   fclose(fp);
        return 0;
    }

    MidiIrq=strtol(str,NULL,10);
    if(MidiIrq==2) MidiIrq = 9;

    if(!GetConfigEntry("SBIRQ",str,fp))
    {   fclose(fp);
        return 0;
    }

    WaveIrq=strtol(str,NULL,10);
    if(WaveIrq==2) WaveIrq=9;

    if(!GetConfigEntry("DMA",str,fp))
    {   fclose(fp);
        return 0;
    }

    DmaChan=strtol(str,NULL,10);
    fclose(fp);

    // see if Soundscape is there by reading HW ...
    if((inportb(BasePort+GA_HOSTCTL_OFF)&0x78) != 0x00) return 0;
    if((inportb(BasePort+GA_ADDR_OFF)&0xf0)==0xf0) return 0;

    outportb(BasePort+GA_ADDR_OFF,0xf5);

    tmp = inportb(BasePort+GA_ADDR_OFF);

    if((tmp & 0xf0)==0xf0) return 0;
    if((tmp & 0x0f)!=0x05) return 0;

    /* formulate the chip ID */
    if( (tmp & 0x80) != 0x00 )
        IcType = MMIC;
    else if((tmp & 0x70) != 0x00)
        IcType = OPUS;
    else
        IcType = ODIE;

    // now do a quick check to make sure the CoDec is there too
    if((inportb(WavePort)&0x80)!=0x00) return 0;
    return 1;
}


#ifdef NEVER

static void interrupt newhandler(void)
{
    if(WaveIrq==7)
    {   outportb(0x20,0x0b);
        inportb(0x21);
        if(!(inportb(0x20)&0x80)) return;
    } else if(WaveIrq==15)
    {   outportb(0xa0,0x0b);
        inportb(0xa1);
        if(!(inportb(0xa0)&0x80)) return;
    }

    interruptcount++;

    /* if the CoDec is interrupting clear the AD-1848 interrupt */
    if(inportb(WavePort+CD_STATUS_OFF)&0x01)
        outportb(WavePort+CD_STATUS_OFF,0x00);

    MIrq_EOI(WaveIrq);
}


static PVI oldhandler;

#endif


static UWORD codecfreqs[14] =
{   5512, 6615, 8000, 9600,11025,16000,18900,
    22050,27428,32000,33075,37800,44100,48000
};


static UWORD codecformats[14] =
{   0x01, 0x0f, 0x00, 0x0e, 0x03, 0x02, 0x05,
    0x07, 0x04, 0x06, 0x0d, 0x09, 0x0b, 0x0c
};


static UBYTE codecformat;

static BOOL SS_Init(void)
{
    int t, ctmp;

    if(!SS_IsThere())
    {   _mm_errno = MMERR_INITIALIZING_DRIVER;
        return 0;
    }

    //printf("Ensoniq Soundscape at port 0x%x, irq %d, dma %d\n",WavePort,WaveIrq,DmaChan);

    // find closest codec frequency

    for(t=0; t<14; t++)
    {   if(t==13 || md_mixfreq<=codecfreqs[t])
        {   md_mixfreq = codecfreqs[t];
            break;
        }
    }

    codecformat = codecformats[t];
    if(md_mode & DMODE_STEREO) codecformat |= 0x10;
    if(md_mode & DMODE_16BITS) codecformat |= 0x40;

    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
    if(VC_Init()) return 1;


    // Do the DMA buffer stuff - calculate the buffer size in bytes from
    // the specification in milliseconds.

    ctmp = md_mixfreq * ((md_mode & DMODE_STEREO) ? 2 : 1) * ((md_mode & DMODE_16BITS) ? 2 : 1);
    ss_dmabufsize = (ctmp * ((md_dmabufsize < 12) ? 12 : md_dmabufsize)) / 1000;

    // no larger than 32000, and make sure it's size is rounded to 16 bytes.

    if(ss_dmabufsize > 32000) ss_dmabufsize = 32000;
    ss_dmabufsize = (ss_dmabufsize+15) & ~15;
    md_dmabufsize = (ss_dmabufsize*1000) / ctmp;

    SS_DMAMEM = MDma_AllocMem(ss_dmabufsize);

    if(SS_DMAMEM==NULL) return 1;

    SS_DMABUF = (SBYTE *)MDma_GetPtr(SS_DMAMEM);

    /* In case the CoDec is running, stop it */
    StopCoDec();

    /* Clear possible CoDec and SoundBlaster emulation interrupts */
    outportb(WavePort+CD_STATUS_OFF,0x00);
    inportb(0x22e);

    /* If necessary, save some regs, do some resource re-routing */
    if( IcType != MMIC)
    {
        /* derive the MIDI and Wave IRQ indices (0-3) for reg writes */
        for( Mindx = 0; Mindx < 4; ++Mindx )
            if( MidiIrq == *(Irqs + Mindx) )
                break;
        for( Windx = 0; Windx < 4; ++Windx )
            if( WaveIrq == *(Irqs + Windx) )
                break;

        /* setup the CoDec DMA polarity */
        GaWrite(GA_DMACFG_REG, 0x50);

        /* give the CoDec control of the DMA and Wave IRQ resources */
            CdCfgSav = GaRead(GA_CDCFG_REG);
        GaWrite(GA_CDCFG_REG, 0x89 | (DmaChan << 4) | (Windx << 1));

        /* pull the Sound Blaster emulation off of those resources */
        DmaCfgSav = GaRead(GA_DMAB_REG);
        GaWrite(GA_DMAB_REG, 0x20);
        IntCfgSav = GaRead(GA_INTCFG_REG);
        GaWrite(GA_INTCFG_REG, 0xf0 | (Mindx << 2) | Mindx);
    }

    /* Save all volumes that we might use, init some levels */
    DacSavL = CdRead(CD_DACL_REG);
    DacSavR = CdRead(CD_DACR_REG);
    CdxSavL = CdRead(CD_CDAUXL_REG);
    CdxSavR = CdRead(CD_CDAUXL_REG);
    AdcSavL = CdRead(CD_ADCL_REG);
    AdcSavR = CdRead(CD_ADCR_REG);

    SetDacVol(127, 127);
    SetAdcVol(96, 96);

    /* Select the mic/line input to the record mux; */
    /* if not ODIE, set the mic gain bit too */
    CdWrite(CD_ADCL_REG, (CdRead(CD_ADCL_REG) & 0x3f) |
        (IcType == ODIE ? 0x80 : 0xa0));
    CdWrite(CD_ADCR_REG, (CdRead(CD_ADCR_REG) & 0x3f) |
        (IcType == ODIE ? 0x80 : 0xa0));

    /* Put the CoDec into mode change state */
    outportb(WavePort + CD_ADDR_OFF, 0x40);

    /* Setup CoDec mode - single DMA chan, AutoCal on */
    CdWrite(CD_CONFIG_REG, 0x0c);

#ifdef NEVER
    /* enable the CoDec interrupt pin */
    CdWrite(CD_PINCTL_REG, CdRead(CD_PINCTL_REG) | 0x02);
    oldhandler=MIrq_SetHandler(WaveIrq,newhandler);
    MIrq_OnOff(WaveIrq,1);
#else
    /* disable the interrupt for mikmod */
    CdWrite(CD_PINCTL_REG, CdRead(CD_PINCTL_REG) & 0xfd);
#endif
    return 0;
}


static void SS_Exit(void)
{
    /* in case the CoDec is running, stop it */
    StopCoDec();

    /* mask the PC DMA Controller */
/*  outportb(DmacRegP->mask, 0x04 | DmaChan); */

#ifdef NEVER
    /* disable the CoDec interrupt pin */
    CdWrite(CD_PINCTL_REG, CdRead(CD_PINCTL_REG) & 0xfd);
    MIrq_OnOff(WaveIrq,0);
    MIrq_SetHandler(WaveIrq,oldhandler);
#endif

    /* restore all volumes ... */
    CdWrite(CD_DACL_REG, DacSavL);
    CdWrite(CD_DACR_REG, DacSavR);
    CdWrite(CD_CDAUXL_REG, CdxSavL);
    CdWrite(CD_CDAUXL_REG, CdxSavR);
    CdWrite(CD_ADCL_REG, AdcSavL);
    CdWrite(CD_ADCR_REG, AdcSavR);

    // if necessary, restore gate array resource registers
    if(IcType!=MMIC)
    {   GaWrite(GA_INTCFG_REG, IntCfgSav);
        GaWrite(GA_DMAB_REG, DmaCfgSav);
        GaWrite(GA_CDCFG_REG, CdCfgSav);
    }

    MDma_FreeMem(SS_DMAMEM);
    VC_Exit();
}


static UWORD last = 0;
static UWORD curr = 0;


static void SS_Update(void)
{
    UWORD todo, index;

    curr = (md_dmabufsize-MDma_Todo(DmaChan))&0xfffc;

    if(curr>=md_dmabufsize) return;
    if(curr==last) return;

    if(curr>last)
    {   todo  = curr-last; index = last;
        last += VC_WriteBytes(&SS_DMABUF[index],todo);
        MDma_Commit(SS_DMAMEM,index,todo);
        if(last>=md_dmabufsize) last = 0;
    } else
    {   todo = md_dmabufsize-last;
        VC_WriteBytes(&SS_DMABUF[last],todo);
        MDma_Commit(SS_DMAMEM,last,todo);
        last = VC_WriteBytes(SS_DMABUF,curr);
        MDma_Commit(SS_DMAMEM,0,curr);
    }
}


static BOOL SS_PlayStart(void)
{
    int   direction = 0;
    long  i;
    UWORD tmp;

    if(VC_PlayStart()) return 1;

    // make sure the the CoDec is in mode change state
    outportb(WavePort + CD_ADDR_OFF, 0x40);

    // and write the format register
    CdWrite(CD_FORMAT_REG, codecformat);

    // if not using ODIE and recording, setup extended format register
    if( IcType != ODIE && direction )
        CdWrite(CD_XFORMAT_REG, codecformat & 0x70);

    // delay for internal re-synch
    for( i = 0; i < 200000UL; ++i )
        inportb(BasePort + GA_ADDR_OFF);

    // clear the dma buffer

    VC_SilenceBytes(SS_DMABUF,md_dmabufsize);
    MDma_Commit(SS_DMAMEM,0,md_dmabufsize);

    // Write the CoDec interrupt count - sample frames per half-buffer.
    // If not using ODIE and recording, use extended count regs

    tmp = md_dmabufsize;
    if(md_mode & DMODE_STEREO) tmp>>=1;
    if(md_mode & DMODE_16BITS) tmp>>=1;
    tmp--;

    if( IcType != ODIE && direction )
    {   CdWrite(CD_XLCOUNT_REG, tmp);
        CdWrite(CD_XUCOUNT_REG, tmp >> 8);
    } else
    {   CdWrite(CD_LCOUNT_REG, tmp);
        CdWrite(CD_UCOUNT_REG, tmp >> 8);
    }

    if(!MDma_Start(DmaChan,SS_DMAMEM,md_dmabufsize,INDEF_WRITE)) return 1;

    // disable mode change state and start the CoDec
    outportb(WavePort + CD_ADDR_OFF, 0x00);
    CdWrite(CD_CONFIG_REG, direction ? 0x02 : 0x01);

    return 0;
}


static void SS_PlayStop(void)
{
    StopCoDec();
    VC_PlayStop();
}


MDRIVER drv_ss =
{   "Ensoniq Soundscape",
    "Ensoniq Soundscape Driver v0.1 - Thanks to CyberCerus",
    0,255,

    NULL,
    SS_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SS_Init,
    SS_Exit,
    NULL,
    VC_SetNumVoices,
    VC_GetActiveVoices,
    SS_PlayStart,
    SS_PlayStop,
    SS_Update,
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
