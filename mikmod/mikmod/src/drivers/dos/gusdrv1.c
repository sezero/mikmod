/*

Name: GUSDRV1.C

Description:
 GUS software mixer and streaming audio driver.  This driver has been de-
 signed to work in conjunction with the standard GUS hardware mixer dri-
 vers as a sound effects channel.

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
#include "mdma.h"



// Ultra[] holds the sample dram adresses in GUS ram
// Ultrs[] holds the size of the samples

static int gus_voices;

static UBYTE  *u_mode;   // 1 = loaded into GUS, 2 = loaded into VIRTCH
static ULONG  *Ultra;
static ULONG  *Ultrs;
static GHOLD  *ghld = NULL;
static UBYTE  gus_mode;               // keep a copy of the GUS config

static DMAMEM *GUS_LEFTMEM = NULL, *GUS_RIGHTMEM = NULL;
static SBYTE  *GUS_MIXBUF = NULL;
static SBYTE  *GUS_LEFTBUF, *GUS_RIGHTBUF;
static ULONG  gus_left, gus_right;    // sample-handles for gus memory
static ULONG  gus_dmabufsize;
static ULONG  gus_mixfreq;
static BOOL   gus_isplaying;

static int    hardsub, softsub;
static ULONG  last, curr;


static SWORD GUSMIX_Load(SAMPLOAD *sload, int type)
{
    SAMPLE  *s = sload->sample;
    int     handle,t;
    long    p,l;
    ULONG   length, loopstart, loopend;

    // Find empty slot to put sample address in
    
    for(handle=0; handle<MAXSAMPLEHANDLES; handle++)
       if((u_mode[handle]==0) && (Ultra[handle]==0)) break;

    if(handle==MAXSAMPLEHANDLES)
    {   _mm_errno = MMERR_OUT_OF_HANDLES;
        return -1;
    }
    
    if(type==MD_HARDWARE)
    {   length    = s->length;
        loopstart = s->loopstart;
        loopend   = s->loopend;
    
        u_mode[handle] = 0;

        if(s->flags & SF_16BITS)
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
    
        if(!(Ultra[handle]=(s->flags & SF_16BITS) ? UltraMalloc16(length+8) : UltraMalloc(length+8) ))
        {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
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
    
            todo = (l > 2048) ? 2048 : l;
            SL_Load(buffer,sload, (s->flags & SF_16BITS) ? todo>>1 : todo);
            UltraPokeChunk(p,buffer,todo);
    
            p += todo;
            l -= todo;
        }
    
        if((s->flags & SF_LOOP) && !(s->flags & SF_BIDI))  // looping sample ?
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
    } else
    {   
        Ultra[handle] = VC_SampleLoad(sload, type);
        u_mode[handle] = 1;
    }

    return handle;
}


void GUSMIX_UnLoad(SWORD handle)
//   callback routine to unload samples
//   smp  =  sampleinfo of sample that is being freed
{
    if(Ultra[handle] != 0)
    {   if(u_mode[handle])
        {   VC_SampleUnload(Ultra[handle]);
            Ultra[handle] = 0; u_mode[handle] = 0;
        } else
        {   UltraFree(Ultrs[handle],Ultra[handle]);
            Ultra[handle] = 0; u_mode[handle] = 0;
        }
    }
}


static BOOL forbid = 0, update = 0;

static void GUSMIX_Update(void)
{
    UBYTE  t;
    GHOLD  *aud;
    ULONG  base, start, size, reppos, repend;
    UWORD  curvol, bigvol = 0, bigvoc = 0;

    if(gus_mode & DMODE_SOFT_MUSIC) return;

    if(forbid)
    {   update = 1;
        return;
    }

    if(timecount < timeskip)
    {  timecount++;
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
        {   // check for any voices stopped by the hardware
            if(UltraVoiceStopped())
                aud->active = 0;
            else
            {   // stop inactive voices [voices stopped by GUS_VoiceStop()]
                if(!aud->active) UltraStopVoice();
            }
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
                                0x8 | ((aud->flags & SF_16BITS) ? 4 : 0) |
                                ((aud->flags & SF_BIDI) ? 16 : 0));
            } else
            {   // Start a one-shot sample

                UltraStartVoice(base + start, base + start, base + size + 2,
                                (aud->flags & SF_16BITS) ? 4 : 0);
            }
        } else if(aud->active)
        {    UltraSetFrequency(aud->frq);
             UltraVectorLinearVolume(aud->vol,0x3f,0);
             UltraSetBalance(aud->pan>>4);
        }
    }
}


#define GSTAT_RIGHT   1

static UBYTE status;
static ULONG index;

static void GUSMIX_SoftUpdate(void)
{
    int    i;
    static UWORD todo;

    // The GUS software mixer streams audio just as the GUSDK says:
    // Allocate two sample spaces in GUS RAM, one for left and one for
    // right.  Now use DMA to stream audio into these buffers, cycling
    // constantly to avoid skips and pops.

    if(UltraDramDmaActive()) return;    // do nothing if DMA still running

    if(status == 0)
    {   UltraSelectVoice(gus_voices);
        curr = (UltraReadVoice() - gus_left);
        if(curr == last) return;

        if(index == 0)
        {   forbid = 1;
            if(gus_mode & DMODE_STEREO)
            {   for(i=0; i<8; i++)
                {   UltraPoke(gus_left+gus_dmabufsize+i,UltraPeek(gus_left+i));
                    UltraPoke(gus_right+gus_dmabufsize+i,UltraPeek(gus_right+i));
                }
            } else
            {   for(i=0; i<8; i++)
                    UltraPoke(gus_left+gus_dmabufsize+i,UltraPeek(gus_left+i));
            }
            forbid = 0;
        }

        if(curr > last)
        {   todo = curr - last;
        } else
        {   todo = gus_dmabufsize - last;
        }

        index = last;     // Set index to start up where we left off before

        if((todo+index) > gus_dmabufsize)
            todo = gus_dmabufsize - index;

        todo &= ~0x1f;

        // Music is mixed to a std. left/right alternating format.  We need
        // them completely separate.  So do that here -->

        if(gus_mode & DMODE_STEREO)
        {   VC_WriteBytes(GUS_MIXBUF, todo<<1);
            last += todo;
            if(last >= gus_dmabufsize) last = 0;
            if(gus_mode & DMODE_16BITS)
            {   SWORD  *ww, *cw, *dw;

                ww = (SWORD *)GUS_MIXBUF;
                cw = (SWORD *)GUS_LEFTBUF;
                dw = (SWORD *)GUS_RIGHTBUF;
                for(i=todo>>1; i; i--, cw++, dw++)
                {   *cw = *ww++;
                    *dw = *ww++;
                }
            } else
            {   UBYTE  *bb, *cb, *db;

                bb = (UBYTE *)GUS_MIXBUF;
                cb = (UBYTE *)GUS_LEFTBUF;
                db = (UBYTE *)GUS_RIGHTBUF;
                for(i=todo; i; i--, cb++, db++)
                {   *cb = *bb++;
                    *db = *bb++;
                }
            }
        } else
        {   last += VC_WriteBytes(GUS_LEFTBUF, todo);
            if(last >= gus_dmabufsize) last = 0;
        }

        // Start the left-channel DMA

        if(todo < 16) todo = 16;

        forbid = 1;
        MDma_Commit(GUS_LEFTMEM,0,todo);
        UltraDownloadDma(gus_left + index);
        MDma_Start(GUS_DRAM_DMA, GUS_LEFTMEM, todo, WRITE_DMA);
        forbid = 0;

        if(gus_mode & DMODE_STEREO)
        {   MDma_Commit(GUS_RIGHTMEM,0,todo);
            forbid = 1;

            // start the right channel if the left is already done

            if(!UltraDramDmaActive())
            {   UltraDownloadDma(gus_right + index);
                MDma_Start(GUS_DRAM_DMA, GUS_RIGHTMEM, todo, WRITE_DMA);
            } else
                status = (gus_mode & DMODE_STEREO) ? GSTAT_RIGHT : 0;
        }
    } else if(status & GSTAT_RIGHT)
    {
        // start the right channel DMA

        forbid = 1;
        UltraDownloadDma(gus_right + index);
        MDma_Start(GUS_DRAM_DMA, GUS_RIGHTMEM, todo, WRITE_DMA);
        status = 0;
    }
    
    forbid = 0;
    if(update)
    {   GUSMIX_Update();
        update = 0;
    }
}


static void ClearMixer(void)
{
    // clear the dma buffer and mixer voices

    VC_SilenceBytes(GUS_LEFTBUF,gus_dmabufsize);
    MDma_Commit(GUS_LEFTMEM,0,gus_dmabufsize);
    UltraDownloadDma(gus_left);
    MDma_Start(GUS_DRAM_DMA,GUS_LEFTMEM,gus_dmabufsize,WRITE_DMA);

    if(gus_mode & DMODE_STEREO)
    {   VC_SilenceBytes(GUS_RIGHTBUF,gus_dmabufsize);
        MDma_Commit(GUS_RIGHTMEM,0,gus_dmabufsize);
        UltraWaitDramDma();  UltraDownloadDma(gus_right);
        MDma_Start(GUS_DRAM_DMA,GUS_RIGHTMEM,gus_dmabufsize,WRITE_DMA);
    }
}


static void StartMixer(void)
{
    UltraSelectVoice(gus_voices);
    UltraSetFrequency(gus_mixfreq);
    UltraVectorLinearVolume(384,0x3f,0);
    UltraSetBalance((gus_mode & DMODE_STEREO) ? 0 : 8);
    UltraStartVoice(gus_left, gus_left, gus_left+gus_dmabufsize,
                    0x8 | ((gus_mode & DMODE_16BITS) ? 4 : 0));

    if(gus_mode & DMODE_STEREO)
    {   UltraSelectVoice(gus_voices+1);
        UltraSetFrequency(gus_mixfreq);
        UltraVectorLinearVolume(384,0x3f,0);
        UltraSetBalance(15);
        UltraStartVoice(gus_right, gus_right, gus_right+gus_dmabufsize,
                        0x8 | ((gus_mode & DMODE_16BITS) ? 4 : 0));
    }
}


static BOOL GUSMIX_SetMixingRate(void)
{
    ULONG ctmp;

    if(GUS_LEFTMEM != NULL)
    {   MDma_FreeMem(GUS_LEFTMEM);
        UltraFree(gus_left, gus_dmabufsize);
    }

    if((gus_mode & DMODE_STEREO) && (GUS_RIGHTMEM != NULL))
    {   MDma_FreeMem(GUS_RIGHTMEM);
        UltraFree(gus_right, gus_dmabufsize);
    }

    if(GUS_MIXBUF != NULL) free(GUS_MIXBUF);

    GUS_LEFTMEM = GUS_RIGHTMEM = NULL;
    GUS_MIXBUF  = NULL;

    if(md_mixfreq > 44100) md_mixfreq = 44100;
    if(md_mixfreq < 4000)  md_mixfreq = 4000;
    gus_mixfreq = md_mixfreq;

    // Do the DMA buffer stuff - calculate the buffer size in bytes from
    // the specification in milliseconds.

    ctmp = md_mixfreq * ((md_mode & DMODE_STEREO) ? 2 : 1) * ((md_mode & DMODE_16BITS) ? 2 : 1);
    gus_dmabufsize = (ctmp * ((md_dmabufsize < 12) ? 12 : md_dmabufsize)) / 1000;

    // no larger than 32000, and make sure it's size is rounded to 32 bytes.

    if(gus_dmabufsize > 32000) gus_dmabufsize = 32000;
    gus_dmabufsize = (gus_dmabufsize+31) & ~31;
    md_dmabufsize  = (gus_dmabufsize*1000) / ctmp;

    // Allocate GUS memory for the GUS software mixer

    gus_left = (gus_mode & SF_16BITS) ? UltraMalloc16(gus_dmabufsize+8) : UltraMalloc(gus_dmabufsize+8);
    if(gus_mode & DMODE_STEREO)
    {   gus_right  = (gus_mode & SF_16BITS) ? UltraMalloc16(gus_dmabufsize+8) : UltraMalloc(gus_dmabufsize+8);
        gus_right += 0x1f;  gus_right &= ~0x1f;
    }
    
    if((GUS_LEFTMEM = MDma_AllocMem(gus_dmabufsize)) == NULL) return 1;
    if(gus_mode & DMODE_STEREO)
    {   if((GUS_RIGHTMEM = MDma_AllocMem(gus_dmabufsize)) == NULL) return 1;
        if((GUS_MIXBUF = _mm_malloc(gus_dmabufsize*2)) == NULL) return 1;
    }

    GUS_LEFTBUF  = (SBYTE *)MDma_GetPtr(GUS_LEFTMEM);
    if(gus_mode & DMODE_STEREO) GUS_RIGHTBUF = (SBYTE *)MDma_GetPtr(GUS_RIGHTMEM);

    return 0;
}


static BOOL GUSMIX_Init(void)
{
    if((u_mode = calloc(MAXSAMPLEHANDLES,sizeof(UBYTE))) == NULL) return 1;
    if((Ultra = calloc(MAXSAMPLEHANDLES,sizeof(ULONG))) == NULL) return 1;
    if((Ultrs = calloc(MAXSAMPLEHANDLES,sizeof(ULONG))) == NULL) return 1;

    UltraDmaHandler(NULL);
    UltraSetDramDma(DMA_IRQ_ENABLE |
                    ((GUS_DRAM_DMA >= 4) ? DMA_WIDTH_16 : 0) |
                    ((md_mode&DMODE_16BITS) ? DMA_DATA_16 : DMA_TWOS_COMP) );


    gus_mode = md_mode;
    if(VC_Init()) return 1;

    UltraOpen(14);
    UltraTimer1Handler(GUSMIX_Update);

    gus_isplaying = 0;
    if(GUSMIX_SetMixingRate()) return 1;
    
    return 0;
}


static void GUSMIX_Exit(void)
{
    UltraClose();

    if(ghld!=NULL)   free(ghld);
    if(u_mode!=NULL) free(u_mode);
    if(Ultra!=NULL)  free(Ultra);
    if(Ultrs!=NULL)  free(Ultrs);
    ghld   = NULL;
    u_mode = NULL;
    Ultra  = NULL;
    Ultrs  = NULL;

    if(GUS_LEFTMEM!=NULL)
    {   MDma_FreeMem(GUS_LEFTMEM);
        UltraFree(gus_left, gus_dmabufsize);
    }

    if((gus_mode & DMODE_STEREO) && (GUS_RIGHTMEM!=NULL))
    {   MDma_FreeMem(GUS_LEFTMEM);
        UltraFree(gus_right, gus_dmabufsize);
    }

    if(GUS_MIXBUF!=NULL) free(GUS_MIXBUF);

    GUS_LEFTMEM = GUS_RIGHTMEM = NULL;
    GUS_MIXBUF  = NULL;

    VC_Exit();
}


static BOOL GUSMIX_SetNumVoices(void)
{
    if(ghld!=NULL) free(ghld);
    if((gus_voices = md_hardchn) != 0)
    {   if((ghld = calloc(sizeof(GHOLD),gus_voices)) == NULL) return 1;
    } else
    {   ghld = NULL;
    }

    // update the gus_mode variable to the current md_mode settings

    gus_mode &= ~(DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX);
    gus_mode |= md_mode & (DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX);

    // Initialize the software mixer!

    VC_SetNumVoices();

    // set up the 'hardsub' and 'softsub' variables, which are used to help
    // divide out hardware and software voice command dispatch.

    hardsub = softsub = 0;

    if(gus_mode & DMODE_SOFT_MUSIC)
        hardsub = md_sngchn;
    else
        softsub = md_sngchn;

    return 0;
}


static BOOL GUSMIX_Reset(void)
{
    if(GUSMIX_SetMixingRate()) return 1;

    if(gus_isplaying)
    {   UltraSelectVoice(gus_voices);
        UltraVectorLinearVolume(0,0x3f,0);
        if(gus_mode & DMODE_STEREO)
        {   UltraSelectVoice(gus_voices+1);
            UltraVectorLinearVolume(0,0x3f,0);
        }
    }

    ClearMixer();
    if(gus_isplaying) StartMixer();

    return 0;
}


static BOOL GUSMIX_PlayStart(void)
{
    int t;

    if(gus_isplaying) return 0;

    gus_isplaying = 1;
    if(VC_PlayStart()) return 1;

    for(t=0; t<gus_voices; t++)
    {   ghld[t].flags  = 0;
        ghld[t].handle = 0;
        ghld[t].kick   = 0;
        ghld[t].active = 0;
        ghld[t].frq    = 10000;
        ghld[t].vol    = 0;
        ghld[t].pan    = (t&1) ? 0 : 255;
    }

    UltraNumVoices(gus_voices + ((gus_mode & DMODE_STEREO) ? 2 : 1));
    UltraEnableOutput();
    GUS_BPM = 125;

    ClearMixer(); StartMixer();

    last   = curr = 0;
    index  = 0;
    status = 0;

    UltraSetBPM(125);
    return 0;
}


static void GUSMIX_PlayStop(void)
{
    if(gus_isplaying)
    {   gus_isplaying = 0;
        UltraStopTimer(1);
        UltraDisableOutput();
        VC_PlayStop();
    }
}


static void GUSMIX_VoiceSetVolume(UBYTE voice, UWORD vol)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            VC_VoiceSetVolume(voice,vol);
        else
            ghld[voice].vol = (vol*6)/4;
    } else
    {   voice -= softsub;
        if(gus_mode & DMODE_SOFT_SNDFX)
            VC_VoiceSetVolume(voice-softsub,vol);
        else
            ghld[voice-hardsub].vol = (vol*6)/4;
    }
}


static void GUSMIX_VoiceSetFrequency(UBYTE voice, ULONG frq)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            VC_VoiceSetFrequency(voice,frq);
        else
            ghld[voice].frq = frq;
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            VC_VoiceSetFrequency(voice-softsub,frq);
        else
            ghld[voice-hardsub].frq = frq;
    }
}


static void GUSMIX_VoiceSetPanning(UBYTE voice, int pan)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            VC_VoiceSetPanning(voice, pan);
        else
        {   if((pan == PAN_SURROUND) || !(md_mode & DMODE_STEREO)) pan = 128;
            ghld[voice].pan = pan;
        }
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            VC_VoiceSetPanning(voice-softsub,pan);
        else
        {   if((pan == PAN_SURROUND) || !(md_mode & DMODE_STEREO)) pan = 128;
            ghld[voice-hardsub].pan = pan;
        }
    }
}


static void GUSMIX_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            VC_VoicePlay(voice,Ultra[handle],start,size,reppos,repend,flags);
        else
        {   ghld[voice].flags  = flags;
            ghld[voice].handle = handle;
            ghld[voice].start  = start;
            ghld[voice].size   = size;
            ghld[voice].reppos = reppos;
            ghld[voice].repend = repend;
            ghld[voice].kick   = 1;
            ghld[voice].active = 1;
        }
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            VC_VoicePlay(voice-softsub,Ultra[handle],start,size,reppos,repend,flags);
        else
        {   voice-=hardsub;
            ghld[voice].flags  = flags;
            ghld[voice].handle = handle;
            ghld[voice].start  = start;
            ghld[voice].size   = size;
            ghld[voice].reppos = reppos;
            ghld[voice].repend = repend;
            ghld[voice].kick   = 1;
            ghld[voice].active = 1;
        }
    }
}


static void GUSMIX_VoiceStop(UBYTE voice)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            VC_VoiceStop(voice);
        else
        {   UltraSelectVoice(voice);
            UltraVectorLinearVolume(0,0x3f,0);
            ghld[voice].active = 0;
        }
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            VC_VoiceStop(voice-softsub);
        else
        {   voice-=hardsub;
            UltraSelectVoice(voice);
            UltraVectorLinearVolume(0,0x3f,0);
            ghld[voice].active = 0;
        }
    }
}


static BOOL GUSMIX_VoiceStopped(UBYTE voice)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            return VC_VoiceStopped(voice);
        else
            return !ghld[voice].active;
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            return VC_VoiceStopped(voice-softsub);
        else
            return !ghld[voice-hardsub].active;
    }
}


static SLONG GUSMIX_VoiceGetPosition(UBYTE voice)
{
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            return VC_VoiceGetPosition(voice);
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            return VC_VoiceGetPosition(voice-softsub);
        else
            voice-=hardsub;
    }

    UltraSelectVoice(voice);
    return(UltraReadVoice()-Ultra[ghld[voice].handle]);
}

    
static void GUSMIX_VoiceReleaseSustain(UBYTE voice)
{

}


static ULONG GUSMIX_VoiceRealVolume(UBYTE voice)
{
    ULONG i,k,j,t;
    ULONG addr,size,start;
    
    if(voice < md_sngchn)
    {   if(gus_mode & DMODE_SOFT_MUSIC)
            return VC_VoiceRealVolume(voice);
    } else
    {   if(gus_mode & DMODE_SOFT_SNDFX)
            return VC_VoiceRealVolume(voice-softsub);
        else
            voice-=hardsub;
    }

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


int GUSMIX_GetActiveVoices(void)
{
    return 0;
}


MDRIVER drv_gus =
{   "Gravis UltraSound",
    "Gravis UltraSound Driver v4.0",
    30,
    255,

    NULL,
    GUS_IsThere,
    GUSMIX_Load,
    GUSMIX_UnLoad,
    GUS_SampleSpace,
    GUS_SampleLength,
    GUSMIX_Init,
    GUSMIX_Exit,
    GUSMIX_Reset,
    GUSMIX_SetNumVoices,
    GUSMIX_GetActiveVoices,
    GUSMIX_PlayStart,
    GUSMIX_PlayStop,
    GUSMIX_SoftUpdate,
    GUSMIX_VoiceSetVolume,
    GUSMIX_VoiceSetFrequency,
    GUSMIX_VoiceSetPanning,
    GUSMIX_VoicePlay,
    GUSMIX_VoiceStop,
    GUSMIX_VoiceStopped,
    GUSMIX_VoiceReleaseSustain,
    GUSMIX_VoiceGetPosition,
    GUSMIX_VoiceRealVolume
};  

