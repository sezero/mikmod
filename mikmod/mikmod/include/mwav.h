//  Mikmod's built in sample format.
//
//  This is tailored for use with basic sound effects for games and modules.

#ifndef _MWAV_H_
#define _MWAV_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct SAMPLE
{
    uint    format;           // dataformat flags
    UBYTE  *data;             // Data, formatted according to flags

    uint    speed;            // default playback speed (almost anything is legal)
    int     volume;           // default volume.  0 to 128 volume range
    int     panning;          // default panning. PAN_LEFT to PAN_RIGHT range

    // These represent the default (overridable) values for playing the given sample.

    int     length;           // length of the sample, in samples. (no modify)
    int     reppos,           // loopstart position (in samples)
            repend;           // loopend position (in samples)
    int     suspos,           // sustain loopstart (in samples)
            susend;           // sustain loopend (in samples)

    uint    flags;            // sample-looping and other flags.
} SAMPLE;


/**************************************************************************
****** Wavload stuff: *****************************************************
**************************************************************************/

SAMPLE *WAV_LoadFP(MMSTREAM *mmfp);
SAMPLE *WAV_LoadFN(const CHAR *filename);
void WAV_Free(SAMPLE *si);

#ifdef __cplusplus
}
#endif

#endif
