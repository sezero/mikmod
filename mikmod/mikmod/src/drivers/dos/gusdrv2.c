/*
Name: GUSDRV2.C

Description:
 GUS native driver (using onboard DRAM and mixer), based on the GUS timer
 interrupt.  This works inside Win95, and very well in the background
 playing in Win95.

Portability:

 MSDOS:  BC(y)   Watcom(y)       DJGPP(y)
 Win95:  n
 Os2:    n
 Linux:  n

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include "gus.h"
#include "mirq.h"


// Ultra[] holds the sample dram adresses in GUS ram
// Ultrs[] holds the size of the samples

static int   gus_voices;

static ULONG *Ultra;
static ULONG *Ultrs;
static GHOLD *ghld = NULL;


static SWORD GUS_Load(SAMPLOAD *sload, int type)
{
    SAMPLE  *s = sload->sample;
    int     handle,t;
    long    p,l;
    ULONG   length, loopstart, loopend;

    length    = s->length;
    loopstart = s->loopstart;
    loopend   = s->loopend;

    // Find empty slot to put sample address in

    for(handle=0; handle<MAXSAMPLEHANDLES; handle++)
       if(Ultra[handle]==0) break;

    if(handle==MAXSAMPLEHANDLES)
    {   _mm_errno = MMERR_OUT_OF_HANDLES;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return -1;
    }

    if(s->flags&SF_16BITS)
    {   if(length > 131000)    // GUS can't handle large 16bit samples
        {   SL_Sample16to8(sload);
        } else
        {   length    <<= 1;
            loopstart <<= 1;
            loopend   <<= 1;
        }
    }

    // Allocate GUS dram and store the address in Ultra[handle]
    // Alloc 8 bytes more for anticlick measures. see below.

    if(!(Ultra[handle]=(s->flags&SF_16BITS) ? UltraMalloc16(length+8) : UltraMalloc(length+8) ))
    {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return -1;
    }

    // Load the sample

    Ultrs[handle] = length+8;
    p = Ultra[handle];
    l = length;

    SL_SampleSigned(sload);

    while(l>0)
    {   static UBYTE  buffer[2048];
        long          todo;

        todo = (l>2048) ? 2048 : l;
        SL_Load(buffer,sload,(s->flags&SF_16BITS) ? todo>>1 : todo);
        UltraPokeChunk(p,buffer,todo);

        p+=todo;
        l-=todo;
    }

    if(s->flags&SF_LOOP && !(s->flags&SF_BIDI))  // looping sample ?
    {   //  Anticlick for looping samples:  Copy the first bytes in the loop
        //                                  beyond the end of the loop

        for(t=0; t<8; t++)
        {   UltraPoke(Ultra[handle]+loopend+t,
                      UltraPeek(Ultra[handle]+loopstart+t));
        }
    } else
    {   //  Anticlick for one-shot samples: Zero the bytes beyond the end
        //                                  of the sample.

        for(t=0; t<8; t++)
            UltraPoke(Ultra[handle]+length+t,0);
    }

    return handle;
}


static void GUS_UnLoad(SWORD handle)
//   callback routine to unload samples
//   smp  =  sampleinfo of sample that is being freed
{
    UltraFree(Ultrs[handle],Ultra[handle]);
    Ultra[handle] = 0;
}


static BOOL GUS_SetNumVoices(void)
{
    if(ghld!=NULL) free(ghld);
    if((ghld = _mm_calloc(sizeof(GHOLD),gus_voices = md_hardchn)) == NULL) return 1;

    return 0;
}    


static void GUS_Update(void)
{
    UBYTE t;
    GHOLD *aud;
    ULONG base,start,size,reppos,repend;
    UWORD curvol, bigvol = 0, bigvoc = 0;

    if(timecount < timeskip)
    {   timecount++;
        return;
    }
    timecount = 1;

    md_player();

    if(GUS_BPM != md_bpm)
    {   UltraSetBPM(md_bpm);
        GUS_BPM = md_bpm;
    }

    // ramp down voices that need to be started next

    for(t=0; t<gus_voices; t++)
    {   UltraSelectVoice(t);
        aud = &ghld[t];
        if(aud->kick)
        {   curvol = UltraReadVolume();
            if(bigvol <= curvol)
            {   bigvol = curvol;
                bigvoc = t;
            }
            UltraVectorLinearVolume(0,0x3f,0);
        } else
        {   // stop inactive voices [voices stopped by GUS_VoiceStop()]           
            if(!aud->active) UltraStopVoice();

            // check for any voices stopped by the hardware
            if(UltraVoiceStopped()) aud->active = 0;
        }
    }  

    for(t=0; t<gus_voices; t++)
    {   UltraSelectVoice(t);
        aud = &ghld[t];

        if(aud->kick)
        {   aud->kick = 0;

            base   = Ultra[aud->handle];
            start  = aud->start;
            reppos = aud->reppos;
            repend = aud->repend;
            size   = aud->size;

            if(aud->flags & SF_16BITS)
            {   start  <<= 1;
                reppos <<= 1;
                repend <<= 1;
                size   <<= 1;
            }

            // Stop current sample and start a new one
            UltraStopVoice();

            UltraSetFrequency(aud->frq);
            UltraVectorLinearVolume(aud->vol,0x3f,0);
            UltraSetBalance(aud->pan>>4);

            if(aud->flags & SF_LOOP)
            {   // Start a looping sample

                UltraStartVoice(base + start, base + reppos, base + repend,
                                0x8 | ((aud->flags & SF_16BITS) ? 4 : 0)
                                | ((aud->flags & SF_BIDI) ? 16 : 0));
            } else
            {   // Start a one-shot sample

                UltraStartVoice(base + start, base + start, base + size + 2,
                                (aud->flags & SF_16BITS) ? 4 : 0);
            }
        } else if(aud->active)
        {    UltraSetFrequency(aud->frq);
             UltraVectorLinearVolume(aud->vol, 0x3f, 0);
             UltraSetBalance(aud->pan >> 4);
        }
    }
}


static BOOL GUS_Init(void)
{
    md_mode |= DMODE_16BITS;
    md_mode &= ~(DMODE_SOFT_SNDFX | DMODE_SOFT_MUSIC | DMODE_SURROUND);

    if((Ultra = calloc(MAXSAMPLEHANDLES,sizeof(ULONG))) == NULL) return 1;
    if((Ultrs = calloc(MAXSAMPLEHANDLES,sizeof(ULONG))) == NULL) return 1;

    UltraOpen(14);
    UltraTimer1Handler(GUS_Update);

    return 0;
}



static void GUS_Exit(void)
{
    UltraClose();

    if(ghld!=NULL) free(ghld);
    if(Ultra!=NULL) free(Ultra);
    if(Ultrs!=NULL) free(Ultrs);
    ghld = NULL;
    Ultra = NULL;
    Ultrs = NULL;
}


static BOOL GUS_Reset(void)
{
    return 0;
}


static BOOL GUS_PlayStart(void)
{
    int t;

    for(t=0; t<gus_voices; t++)
    {   ghld[t].flags  = 0;
        ghld[t].handle = 0;
        ghld[t].kick   = 0;
        ghld[t].active = 0;
        ghld[t].frq    = 10000;
        ghld[t].vol    = 0;
        ghld[t].pan    = (t&1) ? 0 : 255;
    }

    UltraNumVoices(gus_voices);
    UltraEnableOutput();
    GUS_BPM = 125;

    UltraSetBPM(125);
    return 0;
}


static void GUS_PlayStop(void)
{
    UltraStopTimer(1);
    UltraDisableOutput();
}


static void GUS_VoiceSetVolume(UBYTE voice, UWORD vol)
{
    ghld[voice].vol = (vol*6)/4;
}


static void GUS_VoiceSetFrequency(UBYTE voice, ULONG frq)
{
    ghld[voice].frq = frq;
}


static void GUS_VoiceSetPanning(UBYTE voice, ULONG pan)
{
    if(pan == PAN_SURROUND || !(md_mode & DMODE_STEREO)) pan = 128;
    ghld[voice].pan = pan;
}


static void GUS_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
    if(start>=size) return;

    if(flags&SF_LOOP)
       if(repend > size) repend = size;    // repend can't be bigger than size
   
    ghld[voice].flags  = flags;
    ghld[voice].handle = handle;
    ghld[voice].start  = start;
    ghld[voice].size   = size;
    ghld[voice].reppos = reppos;
    ghld[voice].repend = repend;
    ghld[voice].kick   = 1;
    ghld[voice].active = 1;
}


static void GUS_VoiceStop(UBYTE voice)
{
    UltraSelectVoice(voice);
    UltraVectorLinearVolume(0,0x3f,0);
    ghld[voice].active = 0;
}


static BOOL GUS_VoiceStopped(UBYTE voice)
{
    return !ghld[voice].active;
}


static SLONG GUS_VoiceGetPosition(UBYTE voice)
{
    UltraSelectVoice(voice);
    return(UltraReadVoice()-Ultra[ghld[voice].handle]);
}


static void GUS_VoiceReleaseSustain(UBYTE voice)
{

}


static ULONG GUS_VoiceRealVolume(UBYTE voice)
{
    ULONG i, k, j, t;
    ULONG addr, size, start;
    
    UltraSelectVoice(voice);
    addr  = UltraReadVoice();
    size  = addr - ghld[voice].size;
    start = Ultra[ghld[voice].handle];

    i=64; k=0; j=0;
    addr -= (ghld[voice].flags & SF_16BITS) ? 128 : 64;

    if(i>size) i = size;
    if(addr<start) addr = start;
    if((addr-start)+i > size) addr = start+(size-i);

    if(ghld[voice].flags & SF_16BITS)
    {   for(; i; i--, addr++)
        {   t = UltraPeek(addr++);
            t += UltraPeek(addr)<<8;
            if(k<t) k = t;
            if(j>t) j = t;
        }
        k = abs(k-j);
    } else
    {   for(; i; i--, addr++)
        {   t = UltraPeek(addr);
            if(k<t) k = t;
            if(j>t) j = t;
        }
        k = abs(k-j)<<8;
    }

    return(k);
}


MDRIVER drv_gus2 =
{   "Gravis UltraSound",
    "Gravis UltraSound Hardware-Only Driver v3.0",
    32,0,

    NULL,
    GUS_IsThere,
    GUS_Load,
    GUS_UnLoad,
    GUS_SampleSpace,
    GUS_SampleLength,
    GUS_Init,
    GUS_Exit,
    GUS_Reset,
    GUS_SetNumVoices,
    GUS_PlayStart,
    GUS_PlayStop,
    GUS_Dummy,
    GUS_VoiceSetVolume,
    GUS_VoiceSetFrequency,
    GUS_VoiceSetPanning,
    GUS_VoicePlay,
    GUS_VoiceStop,
    GUS_VoiceStopped,
    GUS_VoiceReleaseSustain,
    GUS_VoiceGetPosition,
    GUS_VoiceRealVolume
};  


