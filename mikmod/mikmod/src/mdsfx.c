/*
  Mikmod mdsfx Management System

  By Jake Stine of Divine Entertainment

  Not done code.  Beware.  muhahahaha
*/


#include "mikmod.h"
#include "mdsfx.h"

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
    MD_SAMPLE *mdsfx_loadwavfp(MDRIVER *md, MMSTREAM *mmfp)
// =====================================================================================
{
    MD_SAMPLE *si;
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

    if((si=(MD_SAMPLE *)_mm_calloc(md->allochandle, 1, sizeof(MD_SAMPLE)))==NULL) return NULL;
    
    si->speed  = wh.nSamplesPerSec;
    si->volume = 128;
    si->length = _mm_read_I_ULONG(mmfp);

    format = 0;
    
    //si->data = _mm_malloc(si->length);
    //_mm_read_UBYTES(si->data,si->length, mmfp);

    if((wh.nBlockAlign / wh.nChannels) == 2)
    {   format       = SF_16BITS | SF_SIGNED;
        si->length >>= 1;
    }

    if(wh.nChannels == 2)
    {   format       |= SF_STEREO;
        si->length  >>= 1;
    }

    //si->format = format;

    SL_RegisterSample(md, &si->handle, format, si->length, 0, mmfp, _mm_ftell(mmfp));

    return si;
}


// =====================================================================================
    MD_SAMPLE *mdsfx_loadwav(MDRIVER *md, const CHAR *fn)
// =====================================================================================
// Hack procedure to load a Microsoft RIFF WAV file.  I should have a unimod-style
// layer so that I can more easily support multiple disk formats.  However!  Since nobody
// ever seems to use *anything but WAV* maybe it doesn't really matter.
{
    MMSTREAM   *mmfp;
    MD_SAMPLE  *si;

    if((mmfp=_mm_fopen(fn,"rb"))==NULL) return NULL;

    si = mdsfx_loadwavfp(md, mmfp);
    SL_LoadSamples(md);
    _mm_fclose(mmfp);

    return si;
}


// =====================================================================================
    MD_SAMPLE *mdsfx_create(MDRIVER *md)
// =====================================================================================
{
    MD_SAMPLE *si;

    si = (MD_SAMPLE *)_mm_calloc(md->allochandle, 1,sizeof(MD_SAMPLE));
    si->md = md;
    return si;
}


// =====================================================================================
    MD_SAMPLE *mdsfx_duplicate(MD_SAMPLE *src)
// =====================================================================================
{
    MD_SAMPLE  *newug;

    newug = (MD_SAMPLE *)_mm_malloc(src->md ? src->md->allochandle : NULL, sizeof(MD_SAMPLE));
    
    // duplicate all information

    *newug = *src;
    if(src->owner)
        newug->owner = src->owner;
    else
        newug->owner = src;

    return newug;
}


// =====================================================================================
    void mdsfx_free(MD_SAMPLE *samp)
// =====================================================================================
{
    if(samp->owner)
    {   // deallocate sample from audio drivers.
        MD_SampleUnload(samp->md, samp->handle);
    }

    _mm_free(samp->md ? samp->md->allochandle : NULL, samp);
}


// =====================================================================================
    void mdsfx_play(MD_SAMPLE *s, MD_VOICESET *vs, uint voice, int start)
// =====================================================================================
{
    Voice_Play(vs, vs->vdesc[voice].voice, s->handle, start, s->length, s->reppos, s->repend, s->suspos, s->susend, s->flags);
    Voice_SetVolume(vs,voice, 128);
}


// =====================================================================================
    int mdsfx_playeffect(MD_SAMPLE *s, MD_VOICESET *vs, uint start, uint flags)
// =====================================================================================
// Mikmod's automated sound effects sample player. Picks a voice from the given voiceset,
// based upon the voice either being empty (silent), or being the oldest sound in the 
// voiceset.  Any sound flagged as critical will not be replaced.
//
// Returns the voice that the sound is being played on.  If no voice was available (usually
// by fault of criticals) then -1 is returned.
//
// flags == MDVD_* flags in mdsfx.h
{
    uint     orig;     // for cases where all channels are critical
    int      voice;

    // check for invalid parameters
    if(!vs || !vs->voices || !s) return -1;

    orig = vs->sfxpool;

    // Find a suitable voice for this sample to be played in.
    // Use the user-definable callback procedure to do so!

    voice = (s->findvoice_proc) ? s->findvoice_proc(vs, flags) : Voice_Find(vs, flags);
    if(voice == -1) return -1;

    //volume  = _mm_boundscheck(s->volume, 0, VOLUME_FULL);
    //panning = (s->panning == PAN_SURROUND) ? PAN_SURROUND : _mm_boundscheck(s->panning, PAN_LEFT, PAN_RIGHT);
    
    Voice_Play(vs, voice, s->handle, start, s->length, s->reppos, s->repend, s->suspos, s->susend, s->flags);
    Voice_SetVolume(vs, voice, s->volume);
    Voice_SetPanning(vs, voice, s->panning, 0);
    Voice_SetFrequency(vs, voice, s->speed);

    return voice;
}

