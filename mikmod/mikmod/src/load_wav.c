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
 load_wav.c

 Microsoft's RIFF WAV sample loader.  Now restructured to interface with the
 Sample_Load API (which is not to be confused with the module sample loader
 in sloader.c).

*/

#include "sample.h"

#include <string.h>

// -------------------------------------------------------------------------------------
    typedef struct WAV
// -------------------------------------------------------------------------------------
// Internal wav header stuff, used during the loading and SAMPLE conversion process.
{   
    CHAR    rID[5];
    ULONG   rLen;
    CHAR    wID[5];
    CHAR    fID[5];
    ULONG   fLen;
    UWORD   wFormatTag;
    UWORD   nChannels;
    ULONG   nSamplesPerSec;
    ULONG   nAvgBytesPerSec;
    UWORD   nBlockAlign;
    UWORD   nFormatSpecific;

} WAV;


// =====================================================================================
    static BOOL WAV_Test(MMSTREAM *mmfp)
// =====================================================================================
{
    CHAR       rID[5];
    CHAR       wID[5];
    ULONG      rLen;

    // read wav header

    _mm_read_string(rID,4,mmfp);
    rLen = _mm_read_I_ULONG(mmfp);
    _mm_read_string(wID,4,mmfp);

    if(_mm_feof(mmfp) ||
        memcmp(rID,"RIFF",4) ||
        memcmp(wID,"WAVE",4))
    {
        return FALSE;
    }

    return TRUE;
}


// =====================================================================================
    static void *WAV_Init(void)
// =====================================================================================
{
    return _mm_malloc(NULL, sizeof(WAV));
}


// =====================================================================================
    static void WAV_Cleanup(WAV *wav)
// =====================================================================================
{
    _mm_free(NULL, wav);
}


// =====================================================================================
    static BOOL WAV_LoadHeader(WAV *wh, MMSTREAM *mmfp)
// =====================================================================================
{
    // read wav header

    _mm_read_string(wh->rID,4,mmfp);
    wh->rLen = _mm_read_I_ULONG(mmfp);
    _mm_read_string(wh->wID,4,mmfp);

    while(1)
    {   _mm_read_string(wh->fID,4,mmfp);
        wh->fLen = _mm_read_I_ULONG(mmfp);
        wh->fID[4] = 0;
        if(memcmp(wh->fID,"fmt ",4) == 0) break;
        _mm_fseek(mmfp,wh->fLen,SEEK_CUR);
    }

    wh->wFormatTag      = _mm_read_I_UWORD(mmfp);
    wh->nChannels       = _mm_read_I_UWORD(mmfp);
    wh->nSamplesPerSec  = _mm_read_I_ULONG(mmfp);
    wh->nAvgBytesPerSec = _mm_read_I_ULONG(mmfp);
    wh->nBlockAlign     = _mm_read_I_UWORD(mmfp);
    wh->nFormatSpecific = _mm_read_I_UWORD(mmfp);

    // check it

    if(_mm_feof(mmfp))
    {   _mmlog("Mikmod > mwav > Failure: Unexpected end of file loading wavefile header.");
        return FALSE;
    }

    // skip other crap

    _mm_fseek(mmfp,wh->fLen-16,SEEK_CUR);

    return TRUE;
}


// =====================================================================================
    BOOL WAV_Load(WAV *wh, SAMPLE *si, MMSTREAM *mmfp)
// =====================================================================================
{
    uint        format;
    CHAR        dID[5];

    _mm_read_string(dID,4,mmfp);

    if(memcmp(dID,"data",4))
    {   dID[4] = 0;
        _mmlog("Mikmod > mwav > Failure: Unexpected token %s; 'data' not found!",dID);
        return FALSE;
    }

    if(wh->nChannels > 2)
    {   _mmlog("Mikmod > mwav > Failure: Unsupported type (more than two channels)");
        return FALSE;
    }

    si->speed  = wh->nSamplesPerSec;
    si->volume = 128;
    si->length = _mm_read_I_ULONG(mmfp);

    format = 0;

    si->data = _mm_malloc(si->allochandle, si->length);
    _mm_read_UBYTES(si->data,si->length, mmfp);

    if((wh->nBlockAlign / wh->nChannels) == 2)
    {   format       = SF_16BITS | SF_SIGNED;
        si->length >>= 1;
    }

    if(wh->nChannels == 2)
    {   format       |= SF_STEREO;
        si->length  >>= 1;
    }

    si->format = format;

    return TRUE;
}


SAMPLE_LOAD_API load_wav = 
{
    NULL,           // linked list

    "WAV",
    "RIFF WAV Loader 1.0",
    WAV_Test,
    WAV_Init,
    WAV_LoadHeader,
    WAV_Load,
    WAV_Cleanup,
};
