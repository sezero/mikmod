/*

 MikMod Sound System

  By Jake Stine of Hour 13 Studios (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com

 -----------------------------------------
 sloader.c

  Routines for loading samples.  The sample loader utilizes the routines
  provided by the "registered" sample loader.  See SAMPLELOAD in
  MIKMOD.H for the sample loader structure.

 Portability:
  All systems - all compilers

*/

#include "mikmod.h"
#include "mmforbid.h"

#include "mminline.h"

#include <string.h>
#include <assert.h>

static int        sl_rlength;
static int        sl_old, sl_new, sl_ditherval;
static SWORD     *sl_buffer      = NULL;

static MM_ALLOC  *sl_allochandle = NULL;
static SAMPLOAD  *staticlist     = NULL;
static SL_DECOMPRESS_API   *sl_loadlist = NULL;


// Our sample loader critical section!
static MM_FORBID *mmcs           = NULL;


// ===================================================================
//  Sample Loader API class structure.
//  To make it sound all OO fancy-like!  Woo, because it matters!  yes!
//  Rememeber: Terminology does not make good code.  I do!

static void slfree01(void *block, uint nitems) { sl_buffer    = NULL; }
//static void slfree02(void *block, uint nitems) { sl_compress  = NULL; }

// =====================================================================================
    BOOL SL_Init(SAMPLOAD *s, MM_ALLOC *allochandle)        // returns 0 on error!
// =====================================================================================
// Preps the SAMPLOAD struct for loading and allocates needed buffers.
{    
    if(!mmcs) mmcs = _mmforbid_init();

    _mmforbid_enter(mmcs);

    if(!sl_buffer)
         if((sl_buffer=(SWORD *)_mm_mallocx("sl_buffer", sl_allochandle = allochandle, 131072, slfree01)) == NULL) return 0;

    // Put length of sample into sl_rlength
    // ------------------------------------

    sl_rlength = s->length;
    if(s->infmt & SF_STEREO) sl_rlength *= 2;

    sl_old = sl_new = 0;
    sl_ditherval    = 32;

    // Assign Decompression API
    // ------------------------

    {
        SL_DECOMPRESS_API   *cruise = sl_loadlist;

        while(cruise)
        {
            if(cruise->type == s->decompress.type)
            {   s->decompress.api       = cruise;
                s->decompress.handle    = cruise->init(s->mmfp);
                break;
            }
            cruise = cruise->next;
        }

        if(!cruise)
        {   _mmlog("Mikmod > SLoader > Failed to find suitable sample decompression API!");
            assert(FALSE);
        }
    }
    return 1;
}


// =====================================================================================
    void SL_Exit(SAMPLOAD *s)
// =====================================================================================
{
    // Done to ensure that sample libs/modules which don't use absolute seek indexes
    // still load sample data properly... because sometimes the scaling routines
    // won't load every last byte of the sample

    //if(sl_rlength > 0) _mm_fseek(s->mmfp,sl_rlength,SEEK_CUR);

    s->decompress.api->cleanup(s->decompress.handle);
    _mmforbid_exit(mmcs);
}


// =====================================================================================
    void SL_Cleanup(void)
// =====================================================================================
// This is called by the md_driver to make sure memory is freed up when mikmod is shut
// down.  I have this piciular setup in order to make much more efficient use of memory
// allocation.
{
    _mm_free(sl_allochandle, sl_buffer);
    _mmforbid_deinit(mmcs);
    mmcs = NULL;
}


// =====================================================================================
    void SL_Reset(void)
// =====================================================================================
{
    sl_old = 0;
}


// =====================================================================================
    void SL_RegisterDecompressor(SL_DECOMPRESS_API *ldr)
// =====================================================================================
// Checks if the requested decompressor is already regitsered, and quits if so!
{
    SL_DECOMPRESS_API   *cruise = sl_loadlist;

    while(cruise)
    {
        if(cruise == ldr) return;
        cruise = cruise->next;
    }

    if(sl_loadlist == NULL)
    {   sl_loadlist  = ldr;
        ldr->next    = NULL;
    } else
    {   ldr->next    = sl_loadlist;
        sl_loadlist  = ldr;
    }
}


// =====================================================================================
    void SL_Load(void *buffer, SAMPLOAD *smp, int length)
// =====================================================================================
// length  - number of samples to read.  Any unread data will be skipped.
//    This way, the drivers can dictate to only load a portion of the
//    sample, if such behaviour is needed for any reason.
{
    UWORD      infmt = smp->infmt, outfmt = smp->outfmt;
    int        t, u;

    SWORD     *wptr = (SWORD *)buffer;
    SBYTE     *bptr = (SBYTE *)buffer;
            
    int        inlen;        // length of the input and output (in samples)

    if(outfmt & SF_STEREO) 
        length *= 2;

    //_mmlog("\nLoading Sample > %d\n", length);

    while(length)
    {
        // Get the # of samples to process
        // -------------------------------
        // Must be less than the following: sl_rlength, 32768, and length!

        inlen  = MIN(sl_rlength, 32768);
        if(inlen > length) inlen  = length;

        // ---------------------------------------------
        // Load and decompress sample data

        if(smp->decompress.api)
        {
            if(infmt & SF_16BITS)
                inlen = smp->decompress.api->decompress16(smp->decompress.handle, sl_buffer, inlen, smp->mmfp);
            else
                inlen = smp->decompress.api->decompress8(smp->decompress.handle, sl_buffer, inlen, smp->mmfp);
        }

        // ---------------------------------------------
        // Delta-to-Normal sample conversion

        if(infmt & SF_DELTA)
        {   for(t=0; t<inlen; t++)
            {   sl_buffer[t] += sl_old;
                sl_old        = sl_buffer[t];
            }
        }

        // ---------------------------------------------
        // signed/unsigned sample conversion!
        
        if((infmt^outfmt) & SF_SIGNED)
        {   for(t=0; t<inlen; t++)
               sl_buffer[t] ^= 0x8000;
        }

        if(infmt & SF_STEREO)
        {   if(!(outfmt & SF_STEREO))
            {   // convert stereo to mono!  Easy!
                // NOTE: Should I divide the result by two, or not?
                SWORD  *s, *d;
                s = d = sl_buffer;

                for(t=0; t<inlen; t++, s++, d+=2)
                    *s = (*d + *(d+1)) / 2;
            }
        } else
        {   if(outfmt & SF_STEREO)
            {   // Yea, it might seem stupid, but I am sure someone will do
                // it someday - convert a mono sample to stereo!
                SWORD  *s, *d;
                s = d = sl_buffer;
                s += inlen;
                d += inlen;

                for(t=0; t<inlen; t++)
                {   s-=2;
                    d--;
                    *s = *(s+1) = *d;
                }
            }
        }

        if(smp->scalefactor)
        {   int   idx = 0;
            SLONG scaleval;

            // Sample Scaling... average values for better results.
            t = 0;
            while((t < inlen) && length)
            {   scaleval = 0;
                for(u=smp->scalefactor; u && (t < inlen); u--, t++)
                    scaleval += sl_buffer[t];
                sl_buffer[idx++] = scaleval / (smp->scalefactor-u);
                length--;
            }
        }
        
        length     -= inlen;
        sl_rlength -= inlen;

        // ---------------------------------------------
        // Normal-to-Delta sample conversion

        if(outfmt & SF_DELTA)
        {   for(t=0; t<inlen; t++)
            {   int   ewoo   = sl_new;
                sl_new       = sl_buffer[t];
                sl_buffer[t] = sl_buffer[t] - ewoo;
            }
        }

        if(outfmt & SF_16BITS)
        {   _mminline_memcpy_word(wptr, sl_buffer, inlen);
            wptr += inlen;
        } else
        {   if(outfmt & SF_SIGNED)
            {   for(t=0; t<inlen; t++, bptr++)
                {   int  eep     = (sl_buffer[t]+sl_ditherval);
                    *bptr        =  _mm_boundscheck(eep,-32768l, 32767l) >> 8;
                    sl_ditherval = sl_buffer[t] - (eep & ~255l);
                }
            } else            
            {   for(t=0; t<inlen; t++, bptr++)
                {   int  eep     = (sl_buffer[t]+sl_ditherval);
                    *bptr        =  _mm_boundscheck(eep, 0, 65535l) >> 8;
                    sl_ditherval = sl_buffer[t] - (eep & ~255l);
                }
            }
        }
    }
}


// =====================================================================================
    SAMPLOAD *SL_RegisterSample(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, MMSTREAM *fp, long seekpos)
// =====================================================================================
// Registers a sample for loading when SL_LoadSamples() is called.
// All samples are assumed to be static (else they would be allocated differently! ;)
{
    SAMPLOAD *news, *cruise;

    cruise = staticlist;

    // Allocate and add structure to the END of the list

    if((news=(SAMPLOAD *)_mm_calloc(md->allochandle, 1, sizeof(SAMPLOAD))) == NULL) return NULL;

    if(cruise)
    {   while(cruise->next)  cruise = cruise->next;
        cruise->next = news;
    } else
        staticlist = news;

    news->infmt           = news->outfmt = infmt;
    news->decompress.type = decompress;
    news->seekpos         = seekpos;
    news->mmfp            = fp;
    news->handle          = handle;
    news->length          = length;

    return news;
}


// =====================================================================================
    static void FreeSampleList(MDRIVER *md, SAMPLOAD *s)
// =====================================================================================
{
    while(s)
    {   SAMPLOAD *old = s;
        s = s->next;
        _mm_free(md->allochandle, old);
    }
}


/*static ULONG SampleTotal(SAMPLOAD *samplist, int type)
// Returns the total amount of memory required by the samplelist queue.
{
    int total = 0;

    while(samplist!=NULL)
    {   samplist->sample->flags = (samplist->sample->flags&~63) | samplist->outfmt;
        total += MD_SampleLength(type,samplist->sample);
        samplist = samplist->next;
    }

    return total;
}


static ULONG RealSpeed(SAMPLOAD *s)
{
    return(s->sample->speed / ((s->scalefactor==0) ? 1 : s->scalefactor));
}    
*/

// =====================================================================================
    static BOOL DitherSamples(MDRIVER *md, SAMPLOAD *samplist)
// =====================================================================================
{
    //SAMPLOAD  *c2smp;
    //ULONG     maxsize, speed;

    //if(!samplist) return 0;

    // make sure the samples will fit inside available RAM
    /*if((maxsize = MD_SampleSpace(type)*1024) != 0)
    {   while(SampleTotal(samplist, type) > maxsize)
        {   // First Pass - check for any 16 bit samples
            s = samplist;
            while(s!=NULL)
            {   if(s->outfmt & SF_16BITS)
                {   SL_Sample16to8(s);
                    break;
                }
                s = s->next;
            }
    
            // Second pass (if no 16bits found above) is to take the sample
            // with the highest speed and dither it by half.
            if(s==NULL)
            {   s = samplist;
                speed = 0;
                while(s!=NULL)
                {   if((s->sample->length) && (RealSpeed(s) > speed))
                    {   speed = RealSpeed(s);
                        c2smp = s;
                    }
                    s = s->next;
                }
                SL_HalveSample(c2smp);
            }
        }
    }*/
}

// =====================================================================================
    static BOOL __inline LoadSample(MDRIVER *md, SAMPLOAD *s)
// =====================================================================================
{
    if(s->length)
    {
        if(s->seekpos)
            _mm_fseek(s->mmfp, s->seekpos, SEEK_SET);

        // Call the sample load routine of the driver module.
        // It has to return a 'handle' (>=0) that identifies
        // the sample.

        *s->handle = MD_SampleLoad(md, MM_STATIC, s);

        if(*s->handle < 0)
        {   _mmlog("SampleLoader > Load failed: length = %d; seekpos = %d",s->length, s->seekpos);
            return 1;
        }
    }

    return 0;
}


// =====================================================================================
    static BOOL LoadSamples(MDRIVER *md, SAMPLOAD *samplist)
// =====================================================================================
{
    SAMPLOAD  *s;

    s = samplist;

    while(s)
    {   // sample has to be loaded ? -> increase number of
        // samples, allocate memory and load sample.

        LoadSample(md,s);
        s = s->next;
    }

    return 0;
}


// =====================================================================================
    BOOL SL_LoadSamples(MDRIVER *md)
// =====================================================================================
{
    BOOL ok;

    if(!staticlist) return 0;

    ok = LoadSamples(md, staticlist);
    FreeSampleList(md, staticlist);

    staticlist = NULL;

    return ok;
}


// =====================================================================================
    BOOL SL_LoadNextSample(MDRIVER *md)
// =====================================================================================
{
    BOOL ok;

    if(!staticlist) return 0;

    ok = LoadSample(md, staticlist);

    {   SAMPLOAD *old = staticlist;
        staticlist = staticlist->next;
        _mm_free(md->allochandle, old);
    }

    return staticlist ? TRUE : FALSE;
}


void SL_Sample16to8(SAMPLOAD *s)
{
    s->outfmt &= ~SF_16BITS;
}


void SL_Sample8to16(SAMPLOAD *s)
{
    s->outfmt |= SF_16BITS;
}


void SL_SampleSigned(SAMPLOAD *s)
{
    s->outfmt |= SF_SIGNED;
}


void SL_SampleUnsigned(SAMPLOAD *s)
{
    s->outfmt &= ~SF_SIGNED;
}


void SL_SampleDelta(SAMPLOAD *s, BOOL yesno)
{
    if(yesno)
        s->outfmt |= SF_DELTA;
    else
        s->outfmt &= ~SF_DELTA;
}


void SL_HalveSample(SAMPLOAD *s)
{
    if(s->scalefactor)
        s->scalefactor++;
    else
        s->scalefactor = 2;

    //s->length    = s->diskfmt.length    / s->sample->scalefactor;
    //s->loopstart = s->diskfmt.loopstart / s->sample->scalefactor;
    //s->loopend   = s->diskfmt.loopend   / s->sample->scalefactor;
}


