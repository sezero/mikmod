/*

 Name:  MWAV.C

 Description:
 WAV sample loader
 Stereo .WAV files are not yet supported as samples.

 Portability:
 All compilers -- All systems (hopefully)

*/

#include "mikmod.h"
#include "mwav.h"

#include <string.h>

typedef struct WAV
{   
    CHAR  rID[5];
    ULONG rLen;
    CHAR  wID[5];
    CHAR  fID[5];
    ULONG fLen;
    UWORD wFormatTag;
    UWORD nChannels;
    ULONG nSamplesPerSec;
    ULONG nAvgBytesPerSec;
    UWORD nBlockAlign;
    UWORD nFormatSpecific;
} WAV;


// =====================================================================================
    SAMPLE *WAV_LoadFP(MMSTREAM *mmfp)
// =====================================================================================
{
    SAMPLE     *si;
    uint       format;

    WAV        wh;
    CHAR       dID[5];

    // read wav header

    _mm_read_string(wh.rID,4,mmfp);
    wh.rLen = _mm_read_I_ULONG(mmfp);
    _mm_read_string(wh.wID,4,mmfp);

    if( _mm_feof(mmfp) ||
        memcmp(wh.rID,"RIFF",4) ||
        memcmp(wh.wID,"WAVE",4))
    {
	    _mmlog("Mikmod > mwav > Failure: Not a vaild RIFF waveformat!");
        return NULL;
    }

    while(1)
    {   _mm_read_string(wh.fID,4,mmfp);
        wh.fLen = _mm_read_I_ULONG(mmfp);
        wh.fID[4] = 0;
        if(memcmp(wh.fID,"fmt ",4) == 0) break;
        _mm_fseek(mmfp,wh.fLen,SEEK_CUR);
    }

    wh.wFormatTag      = _mm_read_I_UWORD(mmfp);
    wh.nChannels       = _mm_read_I_UWORD(mmfp);
    wh.nSamplesPerSec  = _mm_read_I_ULONG(mmfp);
    wh.nAvgBytesPerSec = _mm_read_I_ULONG(mmfp);
    wh.nBlockAlign     = _mm_read_I_UWORD(mmfp);
    wh.nFormatSpecific = _mm_read_I_UWORD(mmfp);

    // check it

    if(_mm_feof(mmfp))
    {   _mmlog("Mikmod > mwav > Failure: Unexpected end of file loading wavefile.");
        return NULL;
    }

    // skip other crap

    _mm_fseek(mmfp,wh.fLen-16,SEEK_CUR);
    _mm_read_string(dID,4,mmfp);

    if(memcmp(dID,"data",4))
    {   dID[4] = 0;
        _mmlog("Mikmod > mwav > Failure: Unexpected token %s; 'data' not found!",dID);
        return NULL;
    }

    if(wh.nChannels > 2)
    {   _mmlog("Mikmod > mwav > Failure: Unsupported type (more than two channels)");
        return NULL;
    }

    if((si=(SAMPLE *)_mm_calloc(NULL, 1, sizeof(SAMPLE)))==NULL) return NULL;
    
    si->speed  = wh.nSamplesPerSec;
    si->volume = 128;
    si->length = _mm_read_I_ULONG(mmfp);

    format = 0;
    
    si->data = _mm_malloc(NULL, si->length);
    _mm_read_UBYTES(si->data,si->length, mmfp);

    if((wh.nBlockAlign / wh.nChannels) == 2)
    {   format       = SF_16BITS | SF_SIGNED;
        si->length >>= 1;
    }

    if(wh.nChannels == 2)
    {   format       |= SF_STEREO;
        si->length  >>= 1;
    }

    si->format = format;
    
    return si;
}


// =====================================================================================
    SAMPLE *WAV_LoadFN(const CHAR *filename)
// =====================================================================================
{
    MMSTREAM  *mmfp;
    SAMPLE    *si;

    if((mmfp=_mm_fopen(filename,"rb"))==NULL) return NULL;

    si = WAV_LoadFP(mmfp);
    _mm_fclose(mmfp);

    return si;
}


// =====================================================================================
    void WAV_Free(SAMPLE *si)
// =====================================================================================
{
    if(si)
    {   _mm_free(NULL, si->data);
        _mm_free(NULL, si);
    }
}

