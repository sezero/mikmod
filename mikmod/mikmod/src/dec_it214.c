/*

 Mikmod Sound System - The Legend Continues

  By Jake Stine and Hour 13 Studios (1996-2002)
  Original code & concepts by Jean-Paul Mikkers (1993-1996)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com
  For additional information and updates, see our website:
    http://www.hour13.com

 ---------------------------------------------------
 dec_it214.c

 Courtesy of Justin Frankel!

 ImpulseTracker 8 and 16 bit compressed sample loaders.
 100% tested and proven!
 These routines recieve src and dest buffers, the dest which must be no more
 and no less than 64K in size (that means equal folks).  The cbcount is the
 number of bytes to be read (never > than 32k).


            // create a 32k loading buffer for reading in the compressed frames of samples. Air
            // notes: I had to change this to 36000 bytes to make room for very rare samples
            // which are about 32k in length and don't compress - IT tries to compress them any-
            // way and makes the compressed block > 32k (and larger than the original sample!).
            //                                 ** Woops! **
*/

#include "mikmod.h"


typedef struct IT214_HANDLE
{
    uint     length;         // length of the compressed block!
    UBYTE   *compress;       // compressed data buffer, because IT can't be streamed.

} IT214_HANDLE;


// =====================================================================================
    static IT214_HANDLE *It214_Init(MMSTREAM *mmfp)
// =====================================================================================
{
    IT214_HANDLE    *ih = _mmobj_array(NULL, 1, IT214_HANDLE);
    ih->compress = (UBYTE *)_mm_malloc(NULL, 36000);

    return ih;
}


// =====================================================================================
    static void It214_Cleanup(IT214_HANDLE *ih)
// =====================================================================================
{
    _mm_free(NULL, ih->compress);
    _mm_free(NULL, ih);
}


// =====================================================================================
    static BOOL Decompress8Bit(const UBYTE *src, SWORD *dest, int cbcount1)
// =====================================================================================
{
    uint ebx = 0x00000900;
    uint ecx = 0;
    uint edx = 0;
    uint eax = 0;

D_Decompress8BitData1:
    eax   = *(int *)src;
    
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
    return TRUE;

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
    static BOOL Decompress16Bit(const UBYTE *src, SWORD *dest, int cbcount1)
// =====================================================================================
{
        uint ecx,edx,ebx,eax;
        uint ecx_save,edx_save;
        ecx = 0x1100;
        edx = ebx = eax = 0;

D_Decompress16BitData1:

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
        {   short d = eax&0xffff;
            d >>= (ecx & 0xff);
            eax=(eax&~0xffff) | d;
        }

D_Decompress16BitC:
        ebx += eax;
        *dest = ebx;
        dest++;
        if (--cbcount1) goto D_Decompress16BitData1;
        return TRUE;

D_Decompress16BitDepthChange1:
        eax >>= 16;
        eax  &= 0xffffff0f;
        eax++;
        {   unsigned char d = edx&0xff;
            d  += 4;
            edx = (edx&~0xff) | d;
        }

D_Decompress16BitDepthChange3:
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
        edx      = edx_save;
        eax    <<= (ecx&0xff);
        goto D_Decompress16BitD;

D_Decompress16BitB:
        if (!(eax&0x10000)) goto D_Decompress16BitC;

        ecx = (ecx&~0xff)|0x10;
        eax++;
        {   unsigned char c = (ecx&0xff);
            c  -= (eax&0xff);
            ecx = c | ((eax&0xff)<<8);
        }
    goto D_Decompress16BitData1;
}


// =====================================================================================
    static BOOL It214_Decompress8Bit(IT214_HANDLE *ih, SWORD *dest, int cbcount, MMSTREAM *mmfp)
// =====================================================================================
{
    uint    tlen;
    tlen = _mm_read_I_UWORD(mmfp);
    _mm_read_UBYTES(ih->compress, tlen, mmfp);
    Decompress8Bit(ih->compress, dest, cbcount);
    return cbcount;
}


// =====================================================================================
    static int It214_Decompress16Bit(IT214_HANDLE *ih, SWORD *dest, int cbcount, MMSTREAM *mmfp)
// =====================================================================================
{
    uint    tlen;
    tlen = _mm_read_I_UWORD(mmfp);
    _mm_read_UBYTES(ih->compress, tlen, mmfp);

    if(cbcount > 16384) cbcount = 16384;
    Decompress16Bit(ih->compress, dest, cbcount);
    return cbcount;
}


SL_DECOMPRESS_API dec_it214 =
{
    NULL,
    SL_COMPRESS_IT214,

    It214_Init,
    It214_Cleanup,
    It214_Decompress16Bit,
    It214_Decompress8Bit,
};
