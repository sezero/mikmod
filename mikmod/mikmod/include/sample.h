/*
  sample.h
  Sample structures and Sample loading APIs.
  Traditioanl comment-lacking header file, courtesy of Air.
*/

#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include "mikmod.h"


#ifdef __cplusplus
extern "C" {
#endif


// -------------------------------------------------------------------------------------
    typedef struct SAMPLE
// -------------------------------------------------------------------------------------
{
    MM_ALLOC *allochandle;
    uint      format;           // dataformat flags
    UBYTE    *data;             // Data, formatted according to flags

    uint      speed;            // default playback speed (almost anything is legal)
    int       volume;           // default volume.  0 to 128 volume range
    int       panning;          // default panning. PAN_LEFT to PAN_RIGHT range

    // These represent the default (overridable) values for playing the given sample.

    int       length;           // length of the sample, in samples. (no modify)
    int       reppos,           // loopstart position (in samples)
              repend;           // loopend position (in samples)
    int       suspos,           // sustain loopstart (in samples)
              susend;           // sustain loopend (in samples)

    uint      flags;            // sample-looping and other flags.
    int       userint;          // user-value thingie.  for your own use.

} SAMPLE;


// -------------------------------------------------------------------------------------
    typedef struct SAMPLE_LOAD_API
// -------------------------------------------------------------------------------------
// API interface for various sample loaders!
{
    struct SAMPLE_LOAD_API *next;

    UBYTE       *type;
    UBYTE       *version;

    BOOL        (*Test)(MMSTREAM *fp);
    void       *(*Init)(void);
    BOOL        (*LoadHeader)(void *handle, MMSTREAM *fp);
    BOOL        (*Load)(void *handle, SAMPLE *si, MMSTREAM *fp);
    void        (*Cleanup)(void *handle);

} SAMPLE_LOAD_API;


MMEXPORT void    Sample_RegisterLoader(SAMPLE_LOAD_API *ldr);
MMEXPORT SAMPLE *Sample_Load(const CHAR *filename, MM_ALLOC *allochandle);
MMEXPORT BOOL    Sample_Test(const CHAR *filename);
MMEXPORT SAMPLE_LOAD_API *Sample_TestFP(MMSTREAM *fp);
MMEXPORT SAMPLE *Sample_LoadFP(MMSTREAM *fp, MM_ALLOC *allochandle);
MMEXPORT void    Sample_Free(SAMPLE *sample);

// Various Mikmod-Packaged Sample Loaders
// --------------------------------------

extern SAMPLE_LOAD_API load_wav;
extern SAMPLE_LOAD_API load_ogg;


#ifdef __cplusplus
}
#endif


#endif      // _SAMPLE_H_
