// Mikmod Sound System
// Probably Version 3.5 or 4 or something near there.
// Dated sometime in the early 21st century.
//
// Code by Jake Stine of Divine Entertainment.
//
// Description:
//   virtch.c externals defined for use by the mikmod drivers. I see little
//   reason for any non-driver applications to call these functions directly,
//   but feel free to anyway.  Otherwise, don't bother including this thing,
//   you probably don't need it!
//
// DLL Notes:
//  No exporting is done, since this should not be needed outside of mikmod.
//  Take heed, fair sir!

#ifndef __VIRTCH_C__
#define __VIRTCH_C__

//#include "mikmod.h"

typedef struct VIRTCH VIRTCH;
typedef struct VINFO VINFO;

// =====================================================================================
    typedef struct VC_RESFILTER
// =====================================================================================
// The resonant filter information block.  This contains the current running information
// for a resonant filter.  Normally this structure is attached to each of the channels.
// The use of 64 bit integers hiere is a necessity!  Filters require a high level of ac-
// curacy which just can't be fit in 32 bits.
{
    long     resfactor, cofactor;   // these are pregen values based on the resonance and cutoff.
    INT64S   speed, pos;            // running-sum type values, used and modded by thr mixers.

} VC_RESFILTER;


// =====================================================================================
    typedef struct VMIXER
// =====================================================================================
{   
    struct VMIXER *next;

    CHAR  *name;            // name and version of this mixer!

    BOOL  (*Check)(uint channels, uint mixmode, uint format, uint flags);

    BOOL  (*Init)(struct VMIXER *mixer);
    void  (*Exit)(struct VMIXER *mixer);
    void  (*CalculateVolumes)(struct VIRTCH *vc, struct VINFO *vnf);
    void  (*RampVolume)(struct VINFO *vnf, int done);

    void  (__cdecl *NoClick)(void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo);
    void  (__cdecl *NoClickSurround)(void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo);

    void  (__cdecl *Mix)(void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo);
    void  (__cdecl *MixSurround)(void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo);

    MMVOLUME  *vol;
    void      *mixdat;
} VMIXER;


// =====================================================================================
    typedef struct VSAMPLE
// =====================================================================================
{   
    VMIXER *mixer;

    uint  flags;        // looping flags (used for resetting sustain loops only)
    uint  format;       // dataformat flags (16bit, stereo) -- *unchangeable*
    SBYTE *data;
} VSAMPLE;


#define VC_SURROUND   1

#ifndef __GNUC__
#pragma pack (push,2)
#endif

// =====================================================================================
/*    typedef*/ struct VINFO
// =====================================================================================
{   
    // These should be first, since they benefit from 8 byte alignment
    // (or, well, as according to Intel's VTune).

    INT64S    current;        // current index in the sample
    INT64S    increment;      // fixed-point increment value

    VC_RESFILTER resfilter;
    int       cutoff,
              resonance;

    // Local instance sample information

    BOOL      onhold;           // suspended voices, but not yet dead!
    int       samplehandle;     // sample handle by number (indexes vsample[])

    uint      flags;            // sample flags
    uint      size;             // sample size (length) in samples
    int       reppos,           // loop start
              repend;           // loop end
    int       suspos,           // sustain loop start
              susend;           // sustain loop end

    uint      start;            // start index
    BOOL      kick;             // =1 -> sample has to be restarted
    ULONG     frq;              // current frequency

    int       panflg;           // Panning Flags (VC_SURROUND, etc)
    MMVOLUME  volume;

    // micro volume ramping (declicker)
    int       volramp;
    MMVOLUME  vol, oldvol;

    // Callback functionality

    struct VC_CALLBACK
    {   void  (*proc)(SBYTE *dest, void *userinfo);
        long  pos;
        void  *userinfo;
    } callback;

    UBYTE     loopbuf[32];     // 32 byte loop buffer for clickless looping
} /*VINFO*/;

#ifndef __GNUC__
#pragma pack (pop)
#pragma pack (push,4)
#endif

// =====================================================================================
/*    typedef */struct VIRTCH
// =====================================================================================
{
    BOOL       initialized;

    uint       mode;
    uint       mixspeed;
    uint       cpu;
    uint       channels;
    uint       numchn;
    uint       memory;

    uint       clickcnt;               // number of channels in the vold decliker buffer.

    MMVOLUME   volume;

    uint       samplehandles;

    VINFO     *vinf, *vold;
    VSAMPLE   *sample;

    long       TICK, TICKLEFT, TICKREMAIN, samplesthatfit;
    SLONG     *TICKBUF;
    volatile BOOL preemption;         // see explaination of preemption in the mikmod docs.

    VMIXER    *mixerlist;
    MM_ALLOC  *allochandle;
} /*VIRTCH*/;

#ifndef __GNUC__
#pragma pack (pop)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
****** Virtual channel stuff: *********************************************
**************************************************************************/

#define VC_MAXVOICES 0xfffffful

extern VIRTCH *VC_Init(void);
extern void    VC_Exit(VIRTCH *vc);
extern void    VC_Preempt(MD_DEVICE *md);
extern BOOL    VC_SetSoftVoices(VIRTCH *vc, uint num);
extern BOOL    VC_SetMode(VIRTCH *vc, uint mixspeed, uint mode, uint channels, uint cpumode);
extern void    VC_GetMode(VIRTCH *vc, uint *mixspeed, uint *mode, uint *channels, uint *cpumode);
extern void    VC_RegisterMixer(VIRTCH *vc, VMIXER *mixer);
extern void    VC_RegisterStereoMixers(VIRTCH *vc);

extern void    VC_SetVolume(MD_DEVICE *md, const MMVOLUME *volume);
extern void    VC_GetVolume(MD_DEVICE *md, MMVOLUME *volume);

extern ULONG   VC_SampleSpace(MD_DEVICE *md, int type);
extern ULONG   VC_SampleLength(MD_DEVICE *md, int type, SAMPLOAD *s);
extern int     VC_SampleAlloc(MD_DEVICE *md, uint length, uint *flags);
extern void   *VC_SampleGetPtr(MD_DEVICE *md, uint handle);
extern int     VC_SampleLoad(MD_DEVICE *md, SAMPLOAD *sload, int type);
extern void    VC_SampleUnload(MD_DEVICE *md, uint handle);

extern int     VC_GetActiveVoices(MD_DEVICE *md);

extern void    VC_VoiceSetVolume(MD_DEVICE *md, uint voice, const MMVOLUME *volume);
extern void    VC_VoiceGetVolume(MD_DEVICE *md, uint voice, MMVOLUME *volume);
extern void    VC_VoiceSetFrequency(MD_DEVICE *md, uint voice, ulong frq);
extern ulong   VC_VoiceGetFrequency(MD_DEVICE *md, uint voice);
extern void    VC_VoiceSetPosition(MD_DEVICE *md, uint voice, ulong pos);
extern ulong   VC_VoiceGetPosition(MD_DEVICE *md, uint voice);

extern void    VC_VoiceSetResonance(MD_DEVICE *md, uint voice, int cutoff, int resonance);
extern void    VC_VoiceSetSurround(MD_DEVICE *md, uint voice, int flags);

extern void    VC_VoicePlay(MD_DEVICE *md, uint voice, uint handle, long start, uint length, int reppos, int repend, int suspos, int susend, uint flags);
extern void    VC_VoiceResume(MD_DEVICE *md, uint voice);

extern void    VC_VoiceStop(MD_DEVICE *md, uint voice);
extern BOOL    VC_VoiceStopped(MD_DEVICE *md, uint voice);
extern void    VC_VoiceReleaseSustain(MD_DEVICE *md, uint voice);
extern ULONG   VC_VoiceRealVolume(MD_DEVICE *md, uint voice);

// These are functions the drivers use to update the mixing buffers.

extern void    VC_WriteSamples(MDRIVER *md, SBYTE *buf, long todo);
extern ULONG   VC_WriteBytes(MDRIVER *md, SBYTE *buf, long todo);
extern void    VC_SilenceBytes(MDRIVER *md, SBYTE *buf, long todo);

//extern void    VC_EnableInterpolation(VIRTCH *vc, int handle);
//extern void    VC_DisableInterpolation(VIRTCH *vc, int handle);


// =====================================
//    Mikmod Dynamic Pluggable Mixers
// =====================================

extern VMIXER VC_MIXER_PLACEBO;             // the do-nothing mixer, (c) Creative Silence

// Default 'C' Mixers.
// -------------------

extern VMIXER M8_MONO_INTERP, M8_STEREO_INTERP,
              S8_MONO_INTERP, S8_STEREO_INTERP,
              M8_MONO, M8_STEREO,
              S8_MONO, S8_STEREO;

extern VMIXER M16_MONO_INTERP, M16_STEREO_INTERP,
              S16_MONO_INTERP, S16_STEREO_INTERP,
              M16_MONO, M16_STEREO,
              S16_MONO, S16_STEREO;

// With resonant filters!

extern VMIXER RF_M8_MONO_INTERP, RF_M8_STEREO_INTERP,
              RF_S8_MONO_INTERP, RF_S8_STEREO_INTERP,
              RF_M8_MONO, RF_M8_STEREO,
              RF_S8_MONO, RF_S8_STEREO;

extern VMIXER RF_M16_MONO_INTERP, RF_M16_STEREO_INTERP,
              RF_S16_MONO_INTERP, RF_S16_STEREO_INTERP,
              RF_M16_MONO, RF_M16_STEREO,
              RF_S16_MONO, RF_S16_STEREO;


// Intel Assembly Mixers.

extern VMIXER ASM_M8_MONO_INTERP, ASM_M8_STEREO_INTERP,
              ASM_S8_MONO_INTERP, ASM_S8_STEREO_INTERP,
              ASM_M8_MONO, ASM_M8_STEREO,
              ASM_S8_MONO, ASM_S8_STEREO;

extern VMIXER ASM_M16_MONO_INTERP, ASM_M16_STEREO_INTERP,
              ASM_S16_MONO_INTERP, ASM_S16_STEREO_INTERP,
              ASM_M16_MONO, ASM_M16_STEREO,
              ASM_S16_MONO, ASM_S16_STEREO;

#ifdef __cplusplus
}
#endif

#endif
