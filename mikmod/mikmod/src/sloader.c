/*

 MikMod Sound System

  By Jake Stine of Divine Entertainment (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Module: sloader.c

  Routines for loading samples.  The sample loader utilizes the routines
  provided by the "registered" sample loader.  See SAMPLELOADER in
  MIKMOD.H for the sample loader structure.

 Portability:
  All systems - all compilers

*/

#include "mikmod.h"
#include "mmforbid.h"

#include <string.h>

static int        sl_rlength;
static int        sl_old, sl_new, sl_ditherval;
static SWORD     *sl_buffer      = NULL;
static UBYTE     *sl_compress    = NULL;
static MM_ALLOC  *sl_allochandle = NULL;

static SAMPLOAD  *staticlist     = NULL;

// Our sample loader critical section!

static MM_FORBID *mmcs           = NULL;


// Courtesy of Justin Frankel!
//
// ImpulseTracker 8 and 16 bit compressed sample loaders.
// 100% tested and proven!
// These routines recieve src and dest buffers, the dest which must be no more
// and no less than 64K in size (that means equal folks).  The cbcount is the
// number of bytes to be read (never > than 32k).

// =====================================================================================
    void Decompress8Bit(const UBYTE *src, SWORD *dest, int cbcount1)
// =====================================================================================
{
    uint ebx = 0x00000900;
    uint ecx = 0;
    uint edx = 0;
    uint eax = 0;

D_Decompress8BitData1:
    eax   = *(int *)src;   //(eax&~0xffff) | (*((short *)src));
    
    {   UBYTE   ch = (ecx>>8) & 0xff;

        eax >>= ch;

        ch  += (ebx>>8) & 0xff;
        edx  = (edx&~0xff) | (ch>>3);
        ch  &= 7;
        ecx  = (ecx&~0xff00) | (ch<<8);
        src += edx;
    }
    if (((ebx>>8) &0xff) > 0x06) goto D_Decompress8BitA;
    eax <<= ecx&0xff;
    if ((eax&0xff) == 0x80) goto D_Decompress8BitDepthChange1;

D_Decompress8BitWriteData2:
    {   SBYTE  c = (eax & 0xff);

        c >>= (ecx&0xff);
        eax = (eax&~0xff) | c;
    }
D_Decompress8BitWriteData:
    {   UBYTE  c = (ebx & 0xff);

        c  += (eax&0xff);
        ebx = (ebx&~0xff) | c;
    }
    *dest++ = ((ebx<<8) & 0xffff);
        
    if (--cbcount1) goto D_Decompress8BitData1;
    return;

D_Decompress8BitDepthChange1:
    eax=(eax&~0xff)|((eax>>8)&0x7);
    {   UBYTE  ch=(ecx&0xff00)>>8;
        ch  += 3;
        edx  = (edx&~0xff)|(ch>>3);
        ch  &= 7;
        ecx  = (ecx&~0xff00)|(ch<<8);
    }
    src += edx;
    goto D_Decompress8BitD;

D_Decompress8BitA:
    if (((ebx&0xFF00)>>8) >  0x8) goto D_Decompress8BitC;
    if (((ebx&0xFF00)>>8) == 0x8) goto D_Decompress8BitB;

    {
        UBYTE  al=(eax&0xff);
        al <<= 1;
        eax  = (eax&~0xff) | al;
        if (al < 0x78) goto D_Decompress8BitWriteData2;
        if (al > 0x86) goto D_Decompress8BitWriteData2;
        al >>= 1;
        al  -= 0x3c;
        eax  = (eax&~0xff)|al;
    }
    goto D_Decompress8BitD;

D_Decompress8BitB:
    if ((eax & 0xff)  < 0x7C) goto D_Decompress8BitWriteData;
    if ((eax & 0xff)  > 0x83) goto D_Decompress8BitWriteData;

    {   UBYTE  al = (eax&0xff);
        al -= 0x7c;
        eax=(eax&~0xff) | al;
    }

D_Decompress8BitD:
        ecx = (ecx&~0xff)|0x8;
        {   unsigned short int ax=eax&0xffff;
            ax++;
            eax = (eax&~0xffff) | ax;
        }

        if (((ebx&0xff00)>>8) <= (eax&0xff))
        {   unsigned char al=(eax&0xff);
            al -= 0xff;
            eax = (eax&~0xff)|al;
        }

        ebx = (ebx&~0xff00) | ((eax&0xff)<<8);

        {   unsigned char cl = (ecx&0xff);
            unsigned char al = (eax&0xff);
            cl -= al;
            if ((eax&0xff) > (ecx&0xff)) cl++;
            ecx = (ecx&~0xff) | cl;             
        }
        goto D_Decompress8BitData1;
D_Decompress8BitC:
        eax &= 0x1ff;
        if (!(eax&0x100)) goto D_Decompress8BitWriteData;

    goto D_Decompress8BitD;
}

// =====================================================================================
    void Decompress16Bit(const UBYTE *src, SWORD *dest, int cbcount1)
// =====================================================================================
{
        uint ecx,edx,ebx,eax;
        uint ecx_save,edx_save;
        // esi=src,edi=dest, ebp=cbcount1
        ecx = 0x1100;
        edx = ebx = eax = 0;

D_Decompress16BitData1:
//        _mmlog("> StartThingie : %8x %8x %8x %8x",eax, ebx, ecx, edx);

        ecx_save = ecx;
        eax      = *((ULONG *)src);
        eax    >>= (UBYTE)(edx & 0xff);

        {   
            // get the sum of the low-byte of edx and the high byte of ecx.
            // Then assign it to the low-byte of edx.

            unsigned char c = edx&0xff;
            c   += (ecx&0xff00)>>8;
            edx &= ~0xff;
            edx |= c;
        }

        ecx  = edx>>3;
        src += ecx;
        edx &= 0xffffff07;
        ecx  = ecx_save;
        if ((ecx & 0xff00) > 0x0600) goto D_Decompress16BitA;
        eax <<= ecx&0xff;
        if ((eax & 0xffff) == 0x8000) goto D_Decompress16BitDepthChange1;

D_Decompress16BitD:
        //_mmlog("> DecompressD  : %8x %8x %8x %8x",eax, ebx, ecx, edx);
        {   short d = eax&0xffff;
            d >>= (ecx & 0xff);
            eax=(eax&~0xffff) | d;
        }

D_Decompress16BitC:
        ebx += eax;
        //_mmlog("> DecompressC  : %8x %8x %8x %8x", eax, ebx, ecx, edx);
        *dest = ebx;
        dest++;
        if (--cbcount1) goto D_Decompress16BitData1;
        return;

D_Decompress16BitDepthChange1:
        //_mmlog("> DepthChange1 : %8x %8x %8x %8x",eax, ebx, ecx, edx);
        eax >>= 16;
        eax  &= 0xffffff0f;
        eax++;
        {   unsigned char d = edx&0xff;
            d  += 4;
            edx = (edx&~0xff) | d;
        }

D_Decompress16BitDepthChange3:
        //_mmlog("> DepthChange3 : %8x %8x %8x %8x",eax, ebx, ecx, edx);

        {   unsigned char a = eax&0xff;
            if (a >= ((ecx>>8)&0xff)) a -= 255;
            eax = (eax&~0xff) | a;
        }

        {   unsigned char c;
            c   = 0x10;
            c  -= eax&0xff;
            if ((eax&0xff) > 0x10) c++;
            ecx = ((eax&0xff)<<8) | c;
        }
    goto D_Decompress16BitData1;

D_Decompress16BitA:

        //_mmlog("> DecompressA  : %8x %8x %8x %8x",eax, ebx, ecx, edx);

        if ((ecx&0xff00)>0x1000) goto D_Decompress16BitB;
        edx_save = edx;
        edx      = 0x10000;
        edx    >>= (ecx&0xff);
        edx--;
        eax     &= edx;
        edx    >>= 1;
        edx     += 8;
        if (eax > edx) goto D_Decompress16BitE;
        edx     -= 16;
        if (eax <= edx) goto D_Decompress16BitE;
        eax     -= edx;
        edx      = edx_save;
    goto D_Decompress16BitDepthChange3;

D_Decompress16BitE:
        //_mmlog("> DecompressE  : %8x %8x %8x %8x",eax, ebx, ecx, edx);
        edx      = edx_save;
        eax    <<= (ecx&0xff);
        goto D_Decompress16BitD;

D_Decompress16BitB:
        //_mmlog("> DecompressB  : %8x %8x %8x %8x",eax, ebx, ecx, edx);
        if (!(eax&0x10000)) goto D_Decompress16BitC;

        ecx = (ecx&~0xff)|0x10;
        eax++;
        {   unsigned char c = (ecx&0xff);
            c  -= (eax&0xff);
            ecx = c | ((eax&0xff)<<8);
        }
    goto D_Decompress16BitData1;
}

// ===================================================================
//  Sample Loader API class structure.
//  To make it sound all OO fancy-like!  Woo, because it matters!  yes!
//  Rememeber: Terminology does not make good code.  I do!

static void slfree01(void *block) { sl_buffer    = NULL; }
static void slfree02(void *block) { sl_compress  = NULL; }

// =====================================================================================
    BOOL SL_Init(SAMPLOAD *s, MM_ALLOC *allochandle)        // returns 0 on error!
// =====================================================================================
{    
    if(!mmcs) mmcs = _mmforbid_init();

    _mmforbid_enter(mmcs);

    // A 32k buffer is necessary for decompressing the 32k frames of ImpulseTracker 
    // compressed samples.

    if(!sl_buffer)
         if((sl_buffer=(SWORD *)_mm_mallocx(sl_allochandle = allochandle, 262144, slfree01)) == NULL) return 0;

    // Get the actual byte length of the sample buffer.
    sl_rlength = (s->infmt & SF_16BITS) ? (s->length*2) : s->length;
    if(s->infmt & SF_STEREO) sl_rlength *= 2;

    sl_old = sl_new = 0;
    sl_ditherval = 32;

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
    _mm_free(sl_allochandle, sl_compress);

    _mmforbid_deinit(mmcs);
    mmcs = NULL;
}


// =====================================================================================
    void SL_Reset(void)
// =====================================================================================
{
    sl_old = 0;
}

#include "mminline.h"
//#include "adpcmod.h"

// =====================================================================================
    void SL_Load(void *buffer, SAMPLOAD *smp, int length)
// =====================================================================================
// length  - number of samples to read.  Any unread data will be skipped.
//    This way, the drivers can dictate to only load a portion of the
//    sample, if such behaviour is needed for any reason.
{
    UWORD      infmt = smp->infmt, outfmt = smp->outfmt;
    int        t, u;
    MMSTREAM  *mmfp = smp->mmfp;

    SWORD     *wptr = (SWORD *)buffer;
    SBYTE     *bptr = (SBYTE *)buffer;
            
    int        inlen, outlen;        // length of the input and output (in samples)
    int        todo;

    if(outfmt & SF_STEREO) 
        length *= 2;

    //_mmlog("\nLoading Sample > %d\n", length);

    while(length)
    {
        // Get the # of samples to process.
        todo  = (sl_rlength < 32768) ? sl_rlength : 32768;

        inlen = outlen = todo;

        if(outfmt & SF_16BITS) outlen >>= 1;
        if(outlen > length)    outlen   = length;

        if(infmt & SF_16BITS)  inlen  >>= 1;
        //if(infmt & SF_STEREO)  inlen  >>= 1;

        if(smp->decompress == DECOMPRESS_IT214)
        {   UWORD tlen;

            // create a 32k loading buffer for reading in the compressed frames of ImpulseTracker
            // samples. Air notes: I had to change this to 36000 bytes to make room for very
            // rare samples which are about 32k in length and don't compress - IT tries to com-
            // press them anyway and makes the compressed block > 32k (and larger than the 
            // original sample!).  Woops!

            if(!sl_compress) sl_compress = (UBYTE *)_mm_mallocx(sl_allochandle, 36000, slfree02);

            // load the compressed data into a buffer.  Has to be done
            // because we can't effectively stream this compression algorithm

            tlen = _mm_read_I_UWORD(mmfp);
            _mm_read_UBYTES(sl_compress,tlen,mmfp);

            if(infmt & SF_16BITS)
                Decompress16Bit(sl_compress,sl_buffer, inlen);
            else
                Decompress8Bit(sl_compress,sl_buffer, inlen);

        #ifdef MM_SUPPORT_ADPCM
        } else if(smp->decompress == DECOMPRESS_ADPCM)
        {
            UBYTE  table[16], *input;

            input = _mm_malloc(sl_allochandle, (length+1)/2);
            _mm_read_UBYTES(table,16,mmfp);
            _mm_read_UBYTES(input,(length+1)/2,mmfp);

            DeADPCM(table, input, (length+1)/2, (UBYTE *)sl_buffer);
            _mm_free(sl_allochandle, input);

            todo   = length;
            inlen  = outlen = length;
        #endif
        } else if(infmt & SF_16BITS)
        {   _mm_read_I_SWORDS(sl_buffer,inlen,mmfp);
        } else
        {   SBYTE  *s;
            SWORD  *d;

            _mm_read_SBYTES((SBYTE *)sl_buffer,inlen,mmfp);

            // convert 8 bit data to 16 bit for conversions!
            
            s  = (SBYTE *)sl_buffer;
            d  = sl_buffer;
            s += inlen;
            d += inlen;

            for(t=0; t<inlen; t++)
            {   s--;
                d--;
                *d = (*s) << 8;
            }
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

                for(t=0; t<outlen; t++, s++, d+=2)
                    *s = (*d + *(d+1)) / 2;
            }
        } else
        {   if(outfmt & SF_STEREO)
            {   // Yea, it might seem stupid, but I am sure someone will do
                // it someday - convert a mono sample to stereo!
                SWORD  *s, *d;
                s = d = sl_buffer;
                s += outlen;
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
            while((t < outlen) && length)
            {   scaleval = 0;
                for(u=smp->scalefactor; u && (t < inlen); u--, t++)
                    scaleval += sl_buffer[t];
                sl_buffer[idx++] = scaleval / (smp->scalefactor-u);
                length--;
            }
        }
        
        length     -= outlen;
        sl_rlength -= todo;

        // ---------------------------------------------
        // Normal-to-Delta sample conversion

        if(outfmt & SF_DELTA)
        {   for(t=0; t<outlen; t++)
            {   int   ewoo   = sl_new;
                sl_new       = sl_buffer[t];
                sl_buffer[t] = sl_buffer[t] - ewoo;
            }
        }

        if(outfmt & SF_16BITS)
        {   _mminline_memcpy_word(wptr, sl_buffer, outlen);
            wptr += outlen;
        } else
        {   if(outfmt & SF_SIGNED)
            {   for(t=0; t<outlen; t++, bptr++)
                {   int  eep     = (sl_buffer[t]+sl_ditherval);
                    *bptr        =  _mm_boundscheck(eep,-32768l, 32767l) >> 8;
                    sl_ditherval = sl_buffer[t] - (eep & ~255l);
                }
            } else            
            {   for(t=0; t<outlen; t++, bptr++)
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

    news->infmt         = news->outfmt = infmt;
    news->decompress    = decompress;
    news->seekpos       = seekpos;
    //memcpy(&news->mmfp,fp, sizeof(MMSTREAM));
    news->mmfp          = fp;
    news->handle        = handle;
    news->length        = length;

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


