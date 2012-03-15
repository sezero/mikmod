/*

 Mikmod Sound System -- The Legend Continues!

  By Jake Stine of Hour 13 Studios (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com

 -----------------------------------------
 mikmod.h

  General stuffs needed to use or compile mikmod.  Of course.
  It is easier to list what things are NOT containted within this header
  file:

  a) module player (or any other player) stuffs.  See mplayer.h.
  b) unimod format/structure/object stuffs (unimod.h)
  c) mikmod streaming input/output and memory management (mmio.h,
     automatically included below).
  d) Mikmod typedefs, needed for compiling mikmod only (mmtypes.h)

*/

#ifndef MIKMOD_H
#define MIKMOD_H

#include "mmio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GLOBVOL_FULL  128
#define VOLUME_FULL   128

#define PAN_LEFT         (0-128)
#define PAN_RIGHT        128
#define PAN_FRONT        (0-128)
#define PAN_REAR         128
#define PAN_CENTER       0

#define PAN_SURROUND     512         // panning value for Dolby Surround


#define Mikmod_RegisterDriver(x) MD_RegisterDriver(&x)
#define Mikmod_RegisterLoader(x) ML_RegisterLoader(&x)
#define Mikmod_RegisterErrorHandler(x) _mmerr_sethandler(x)
#define Mikmod_PrecacheSamples() SL_LoadSamples()

/* typedefs moved up here to satisfy GCC. */

typedef struct MDRIVER     MDRIVER;
typedef struct MD_VOICESET MD_VOICESET;

// ==========================================================================
// Sample format [in-memory] flags:
// These match up to the bit format passed to the drivers.

// Notes:
//   SF Prefixes - Driver flags.  These describe the sample format to the driver
//     for both loading and playback.
//   SL Prefixes - Looping flags.

#define SL_LOOP             (1ul<<0)
#define SL_BIDI             (1ul<<1)
#define SL_REVERSE          (1ul<<2)
#define SL_SUSTAIN_LOOP     (1ul<<3)   // Sustain looping, as used by ImpulseTracker
#define SL_SUSTAIN_BIDI     (1ul<<4)   //   (I hope someone finds a use for this someday
#define SL_SUSTAIN_REVERSE  (1ul<<5)   //    considering how much work it was!)
#define SL_DECLICK          (1ul<<5)   // enable aggressive declicking - generally unneeded.

#define SF_16BITS           (1ul<<0)
#define SF_STEREO           (1ul<<1)
#define SF_SIGNED           (1ul<<2)
#define SF_DELTA            (1ul<<3)
#define SF_BIG_ENDIAN       (1ul<<4)

#define SL_RESONANCE_FILTER (1ul<<8)   // Impulsetracker-style resonance filters


// Used to specify special values for start in Voice_Play().
// Note that 'SF_START_BEGIN' starts the sample at the END if SL_REVERSE is set!
#define SF_START_BEGIN       -2
#define SF_START_CURRENT     -1

enum SL_COMPRESS
{   
    SL_COMPRESS_RAW   = 0,
    SL_COMPRESS_IT214 = 1,
    SL_COMPRESS_IT215,
    SL_COMPRESS_MPEG3,
    SL_COMPRESS_ADPCM,
    SL_COMPRESS_VORBIS,
    SL_COMPRESS_COUNT,
};


// -------------------------------------------------------------------------------------
    typedef struct SL_DECOMPRESS_API
// -------------------------------------------------------------------------------------
// decompress16/8    - returns the number of samples actually loaded.
{
    struct SL_DECOMPRESS_API     *next;

    enum SL_COMPRESS  type;    // compression type!

    void   *(*init)(MMSTREAM *mmfp);
    void    (*cleanup)(void *handle);
    int     (*decompress16)(void *handle, SWORD *dest, int cbcount1, MMSTREAM *mmfp);
    int     (*decompress8)(void *handle, SWORD *dest, int cbcount1, MMSTREAM *mmfp);

} SL_DECOMPRESS_API;


// -------------------------------------------------------------------------------------
    typedef struct SAMPLOAD
// -------------------------------------------------------------------------------------
// This structure contains all pertinent information to loading and properly
// converting a sample.
{
    uint      length;           // length of the sample to load

    uint      infmt,            // format of the sample on disk (never changes)
              outfmt;           // format it will be in when loaded

    MMSTREAM  *mmfp;            // file and seekpos of module
    long      seekpos;          // seek position within the file (-1 = current position)
    int       *handle;          // place to write the handle to
    int        scalefactor;

    struct
    {
        enum SL_COMPRESS    type;  // compression type!
        SL_DECOMPRESS_API  *api;   // decompression api!
        void               *handle;
    } decompress;

    struct SAMPLOAD *next;      // linked list hoopla.

} SAMPLOAD;


MMEXPORT void      SL_RegisterDecompressor(SL_DECOMPRESS_API *ldr);

MMEXPORT void      SL_HalveSample(SAMPLOAD *s);
MMEXPORT void      SL_Sample8to16(SAMPLOAD *s);
MMEXPORT void      SL_Sample16to8(SAMPLOAD *s);
MMEXPORT void      SL_SampleSigned(SAMPLOAD *s);
MMEXPORT void      SL_SampleDelta(SAMPLOAD *s, BOOL yesno);
MMEXPORT void      SL_SampleUnsigned(SAMPLOAD *s);
MMEXPORT BOOL      SL_LoadSamples(struct MDRIVER *md);
MMEXPORT BOOL      SL_LoadNextSample(struct MDRIVER *md);
MMEXPORT void      SL_Load(void *buffer, SAMPLOAD *smp, int length);
MMEXPORT BOOL      SL_Init(SAMPLOAD *s, MM_ALLOC *allochandle);
MMEXPORT void      SL_Exit(SAMPLOAD *s);
MMEXPORT void      SL_Cleanup(void);

MMEXPORT SAMPLOAD *SL_RegisterSample(struct MDRIVER *md, int *handle, uint infmt, uint length, int decompress, MMSTREAM *fp, long seekpos);

extern SL_DECOMPRESS_API  dec_raw;
extern SL_DECOMPRESS_API  dec_it214;
extern SL_DECOMPRESS_API  dec_vorbis;
extern SL_DECOMPRESS_API  dec_adpcm;


/**************************************************************************
****** Driver stuff: ******************************************************
**************************************************************************/

#define  MM_STATIC    1
#define  MM_DYNAMIC   2

#define  MD_DISABLE_SURROUND  0
#define  MD_ENABLE_SURROUND   1

enum
{   MD_MONO = 1,
    MD_STEREO,
    MD_QUADSOUND
};

// possible mixing mode bits:
// --------------------------
// These take effect only after Mikmod_Init or Mikmod_Reset.

#define DMODE_DEFAULT        (1ul<<16)   // use default/current settings.  Ignore all other flags

#define DMODE_16BITS         (1ul<<0)    // enable 16 bit output
#define DMODE_NOCLICK        (1ul<<1)    // enable declicker - uses volume micro ramps to remove clicks.
#define DMODE_REVERSE        (1ul<<2)    // reverse stereo
#define DMODE_INTERP         (1ul<<3)    // enable interpolation
#define DMODE_EXCLUSIVE      (1ul<<4)    // enable exclusive (non-cooperative) mode.
#define DMODE_SAMPLE_8BIT    (1ul<<5)    // force use of 8 bit samples only.
#define DMODE_SAMPLE_DYNAMIC (1ul<<6)    // force dynamic sample support
#define DMODE_SURROUND       (1ul<<7)    // Enable support for dolby surround (inverse waveforms left/right)
#define DMODE_RESONANCE      (1ul<<8)    // Enable support for resonance filters


// -------------------------------------------------------------------------------------
    typedef struct _MMVOLUME
// -------------------------------------------------------------------------------------
// Used by mdriver to communicate quadsound volumes between itself and the drivers in a
// fast, efficient, and clean manner.  You can use it too, if are elite and don't mind 
// using structs for things.  Or you can just use 'flvol, frvol, rlvol, rrvol' for 
// everything like a dork!
{
    struct
    {   int   left, right;
    } front;
    
    struct
    {   int   left, right;
    } rear;
} MMVOLUME;


// -------------------------------------------------------------------------------------
    typedef struct MD_DEVICE
// -------------------------------------------------------------------------------------
// driver structure:
// Each driver must have a structure of this type.
{   
    CHAR    *Name;
    CHAR    *Version;
    uint    HardVoiceLimit,       // Limit of hardware mixer voices for this driver
            SoftVoiceLimit;       // Limit of software mixer voices for this driver

    // Below is the 'private' stuff, to be used by the MDRIVER and the
    // driver modules themselves [commenting is my version of C++ private]

    struct MD_DEVICE  *next;

    struct VIRTCH     *vc;        // optional software mixer handle --initialized by the driver if/when needed.
    void              *local;     // local data storage unit.  unit or loose it.  hahaha.. hr.. ugh.

    // sample loading
    int     (*SampleAlloc)     (struct MD_DEVICE *md, uint length, uint *flags);
    void   *(*SampleGetPtr)    (struct MD_DEVICE *md, uint handle);
    int     (*SampleLoad)      (struct MD_DEVICE *md, SAMPLOAD *s, int type);
    void    (*SampleUnLoad)    (struct MD_DEVICE *md, uint handle);
    ULONG   (*FreeSampleSpace) (struct MD_DEVICE *md, int type);
    ULONG   (*RealSampleLength)(struct MD_DEVICE *md, int type, SAMPLOAD *s);

    // detection and initialization
    BOOL    (*IsPresent)       (void);
    BOOL    (*Init)            (struct MDRIVER *md, uint latency, void *optstr);  // init driver using curent mode settings.
    void    (*Exit)            (struct MDRIVER *md);
    void    (*Update)          (struct MDRIVER *md);      // update driver (for polling-based updates).
    void    (*Preempt)         (struct MD_DEVICE *md);    // request update preemption and player resync

    BOOL    (*SetHardVoices)   (struct MDRIVER *md, uint num);
    BOOL    (*SetSoftVoices)   (struct MDRIVER *md, uint num);

    // GetMode and Setmode.  They don't use structures because Zero and Vecna
    // are sick of me making structs for everything.
    // Notes:
    //   SetMode can be used at any time to re-configure the driver settings.
    //   Some drivers will reconfigure, others may just ignore it.  Depends
    //   on the driver.

    BOOL    (*SetMode)         (struct MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode);
    void    (*GetMode)         (struct MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode);

    // Driver volume system (quadsound support!)
    //  (There is no direct panning function because all panning is done 
    //  through manipulation of the four volumes).

    void    (*SetVolume)       (struct MD_DEVICE *md, const MMVOLUME *volume);
    void    (*GetVolume)       (struct MD_DEVICE *md, MMVOLUME *volume);

    // Voice control and Voice information
    
    int     (*GetActiveVoices)    (struct MD_DEVICE *md);

    void    (*VoiceSetVolume)     (struct MD_DEVICE *md, uint voice, const MMVOLUME *volume);
    void    (*VoiceGetVolume)     (struct MD_DEVICE *md, uint voice, MMVOLUME *volume);
    void    (*VoiceSetFrequency)  (struct MD_DEVICE *md, uint voice, ulong frq);
    ulong   (*VoiceGetFrequency)  (struct MD_DEVICE *md, uint voice);
    void    (*VoiceSetPosition)   (struct MD_DEVICE *md, uint voice, ulong pos);
    ulong   (*VoiceGetPosition)   (struct MD_DEVICE *md, uint voice);
    void    (*VoiceSetSurround)   (struct MD_DEVICE *md, uint voice, int flags);
    void    (*VoiceSetResonance)  (struct MD_DEVICE *md, uint voice, int cutoff, int resonance);

    void    (*VoicePlay)          (struct MD_DEVICE *md, uint voice, uint handle, long start, uint length, int reppos, int repend, int suspos, int susend, uint flags);
    void    (*VoiceResume)        (struct MD_DEVICE *md, uint voice);
    void    (*VoiceStop)          (struct MD_DEVICE *md, uint voice);
    BOOL    (*VoiceStopped)       (struct MD_DEVICE *md, uint voice);
    void    (*VoiceReleaseSustain)(struct MD_DEVICE *md, uint voice);
    ulong   (*VoiceRealVolume)    (struct MD_DEVICE *md, uint voice);

    void    (*WipeBuffers)        (struct MDRIVER *md);
} MD_DEVICE;


// ==========================================================================
// VoiceSets!  The Magic of Modular Design!
//
// Each voiceset has set characteristics, including a couple flags (most
// specifically, the flag for hardware or software mixing, if available), and
// volume/voice descriptors.
//
// The voicesets must all be registered with the driver prior to initial-
// ization, and are then referenced via an integer handle.

#define MDVS_DYNAMIC            (1ul<<0)      // software mixing enable
#define MDVS_STATIC             (1ul<<1)      // preferred hardware mixing
#define MDVS_PLAYER             (1ul<<2)      // enable the player (if (*player) is not NULL)
#define MDVS_FROZEN             (1ul<<3)      // Freeze all voice activity on the voiceset.
#define MDVS_STREAM             (1ul<<4)      // streaming voice set
#define MDVS_INHERITED_VOICES   (1ul<<5)      // inherits the parent's voice pool

#define MDVD_CRITICAL           (1ul<<0)      // Voice is marked as critical
#define MDVD_PAUSED             (1ul<<1)      // voice is suspended (paused) rather than off.


// -------------------------------------------------------------------------------------
    typedef struct _VS_STREAM
// -------------------------------------------------------------------------------------
{   
    void    (*callback)(struct MD_VOICESET *voiceset, uint voice, void *dest, uint len);
    void    *streaminfo;        // information set by and used by the streaming device.

    uint    blocksize;          // buffer size...
    uint    numblocks;
    uint    handle;             // sample handle allocated for streaming
    ulong   oldpos;             // used for hard-headed streaming mode.
} _VS_STREAM;


// -------------------------------------------------------------------------------------
    typedef struct MD_VOICE_DESCRIPTOR
// -------------------------------------------------------------------------------------
{   
    int         voice;             // reference/index to the 'real' voice
    int         volume;            // mdriver's pre-vs->volume modified volumes!
    int         pan_lr, pan_fb;    // panning values.  left/right and front/back.

    uint        flags;             // describes each voice

    // Streaming audio options:
    // Allows the user to put their own user-defineable streaming data into the
    // sample buffer, rather than mikmod filling it itself from precached
    // SAMPLEs.

    _VS_STREAM *stream;

} MD_VDESC;


// -------------------------------------------------------------------------------------
    struct MD_VOICESET
// -------------------------------------------------------------------------------------
// Uh Oh! I used my stupid-like object oriented coding approach for this structure... and
// I'm just sure all your GCC/unix people are going to hate it. :)
{
    // premix modification options:
    // Procedures are available for modifying the sample data before the
    // mixer adds it into the mixing buffer.  This is intended for streaming
    // audio and the option to add additional effects, such as flange or reverb.

    void     (*premix8) (struct MD_VOICESET *voiceset, uint voice, UBYTE *dest, uint len);
    void     (*premix16)(struct MD_VOICESET *voiceset, uint voice, SWORD *dest, uint len);

    // Streaming audio options:
    // Allows the user to put their own user-defineable streaming data into the
    // sample buffer, rather than mikmod filling it itself from precached
    // SAMPLEs.  This information is used for any voiceset that has the MDVS_STREAM
    // flag set.


    // Music options:
    // Mikmod includes support for playing music in an accurate and efficient
    // manner.  The mixer will ensure that all players are updated in appro-
    // priate fashion.

    uint     (*player)(struct MD_VOICESET *voiceset, void *playerinfo);
    void      *playerinfo;      // info block allocated for and used by the player.

    // MDRIVER Private stuff:
    //  Changing this stuff will either cause very bad, or simply unpredictable
    //  behaviour.  Please use the provided VoiceSet_* class of API functions
    //  to modify this jazz.

    UBYTE      flags;            // voiceset flags
                
    uint       voices;           // number of voices allocated to this voiceset (and all it's children)
    MD_VDESC  *vdesc;            // voice descriptors + voice lookup table.

    int        countdown;        // milliseconds until the next call to this player.
    uint       sfxpool;          // sound effects voice-search

    int        volume;           // voiceset 'natural' volume
	int        absvol;           // internal volume (after parents suppress it)
    int        pan_lr, pan_fb;

    struct MD_VOICESET
              *next, *nextchild,
              *owner,
              *children;

    struct MDRIVER *md;          // the driver this voiceset is attached to.
};


// -------------------------------------------------------------------------------------
    struct MDRIVER
// -------------------------------------------------------------------------------------
// The info in this structure, after it has been returned from Mikmod_Init, will be
// properly updated whenever the status of the driver changes (for whatever reason
// that may be).  Note that none of these should ever be changed manually.  Use
// the appropriate Mikmod_* functions to alter the driver state.
{
    MD_DEVICE  device;      // device info / function pointers.
    MM_ALLOC  *allochandle;

    // hardvoices and softvoices:
    //  Marked volatile because, well.. they are!  A player thread could change them at any time,
    //  for example.
    
    volatile
    int       hardvoices,    // total hardware voices allocated (-1 means hardware is not available!)
              softvoices;    // total software voices allocated (-1 means software is not available!)

    int       volume;        // Global sound volume (0-128)
    int       pan;           // panning position (PAN_LEFT to PAN_RIGHT)
    int       pansep;        // 0 == mono, 128 == 100% (full left/right)

    // Everything from here on down is generally static, unless the user
    // requests a driver mode chnage manually.

    CHAR     *name;          // name of this device
    CHAR     *optstr;        // options string, used by some drivers.

    uint      mode;          // Mode.  See DMODE_? flags above
    uint      channels;      // number of channels.
    uint      cpu;           // cpu mode.
    uint      mixspeed;      // mixing/sampling rate.
    uint      latency;       // set latency, in milliseconds

    // Voiceset Information

    MD_VOICESET  *voiceset;

    // Mdriver Voice Management:
    //  These indicate which voices of the two voice types have been assigned to
    //  voice sets, and which voice sets they are allocated to.  A value of -1 is
    //  unassigned.  Any other value is the index of the voiceset in use.

    MD_VOICESET **vmh,      // voice management, hardware (NULL if no hardware supported)
                **vms;      // voice management, software (NULL if no software supported)

    BOOL     isplaying;
};


// -------------------------------------------------------------------------------------
    typedef struct MD_STREAM
// -------------------------------------------------------------------------------------
// This is a streaming audio template structure, which can be attached to any
// SAMPLE struct to turn that struct into an automatic streaming audio sample.
{
    void    (*callback)(struct MD_VOICESET *voiceset, uint voice, void *dest, uint len);
    void    *streaminfo;        // information set by and used by the streaming device.
    uint    blocksize;          // buffer size... (in samples)
    uint    numblocks;          // number of blocks (> 1 means block mode!)

} MD_STREAM;


// main driver prototypes:
// =======================

MMEXPORT void       Mikmod_RegisterAllDrivers(void);
MMEXPORT int        Mikmod_GetNumDrivers(void);
MMEXPORT MD_DEVICE *Mikmod_DriverInfo(int driver);

MMEXPORT void     Mikmod_SamplePrecacheFP(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, FILE *fp, long seekpos);
MMEXPORT void     Mikmod_SamplePrecacheMem(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, void *data);

//MMEXPORT int      Mikmod_PlaySound(MD_VOICESET *vs, SAMPLE *s, uint start, uint flags);

MMEXPORT MDRIVER *Mikmod_Init(uint mixspeed, uint latency, void *optstr, uint chanmode, uint cpumode, uint drvmode);
MMEXPORT void     Mikmod_Exit(MDRIVER *md);
MMEXPORT BOOL     Mikmod_Reset(MDRIVER *md);
MMEXPORT BOOL     Mikmod_Active(MDRIVER *md);
MMEXPORT void     Mikmod_RegisterPlayer(MDRIVER *md, void (*plr)(void));
MMEXPORT void     Mikmod_Update(MDRIVER *md);
MMEXPORT int      Mikmod_GetActiveVoices(MDRIVER *md);
MMEXPORT void     Mikmod_WipeBuffers(MDRIVER *md);

MMEXPORT BOOL     Mikmod_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode);
MMEXPORT void     Mikmod_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode);


// Voiceset crapola -- from -- VOICESET.C --
// =========================================

MMEXPORT MD_VOICESET *Voiceset_Create(MDRIVER *drv, MD_VOICESET *owner, uint voices, uint flags);
MMEXPORT void         Voiceset_SetPlayer(MD_VOICESET *vs, uint (*player)(struct MD_VOICESET *voiceset, void *playerinfo), void *playerinfo);
MMEXPORT MD_VOICESET *Voiceset_CreatePlayer(MDRIVER *md, MD_VOICESET *owner, uint voices, uint (*player)(struct MD_VOICESET *voiceset, void *playerinfo), void *playerinfo, BOOL hardware);
MMEXPORT void         Voiceset_Free(MD_VOICESET *vs);
MMEXPORT int          Voiceset_SetNumVoices(MD_VOICESET *vs, uint voices);
MMEXPORT int          Voiceset_GetVolume(MD_VOICESET *vs);
MMEXPORT void         Voiceset_SetVolume(MD_VOICESET *vs, int volume);

MMEXPORT void     Voiceset_PlayStart(MD_VOICESET *vs);
MMEXPORT void     Voiceset_PlayStop(MD_VOICESET *vs);
MMEXPORT void     Voiceset_Reset(MD_VOICESET *vs);
MMEXPORT void     Voiceset_EnableOutput(MD_VOICESET *vs);
MMEXPORT void     Voiceset_DisableOutput(MD_VOICESET *vs);

// MD_Player - Voiceset Player Update Procedure
extern   ulong    MD_Player(MDRIVER *drv, ulong timepass);

MMEXPORT void     Voice_SetVolume(MD_VOICESET *vs, uint voice, int ivol);
MMEXPORT int      Voice_GetVolume(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_SetPosition(MD_VOICESET *vs, uint voice, long pos);
MMEXPORT long     Voice_GetPosition(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_SetFrequency(MD_VOICESET *vs, uint voice, long frq);
MMEXPORT long     Voice_GetFrequency(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_SetPanning(MD_VOICESET *vs, uint voice, int pan_lr, int pan_fb);
MMEXPORT void     Voice_GetPanning(MD_VOICESET *vs, uint voice, int *pan_lr, int *pan_fb);
MMEXPORT void     Voice_SetResonance(MD_VOICESET *vs, uint voice, int cutoff, int resonance);

MMEXPORT void     Voice_Play(MD_VOICESET *vs, uint voice, int handle, long start, int length, int reppos, int repend, int suspos, int susend, uint flags);
MMEXPORT void     Voice_Pause(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_Resume(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_Stop(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_ReleaseSustain(MD_VOICESET *vs, uint voice);
MMEXPORT BOOL     Voice_Stopped(MD_VOICESET *vs, uint voice);
MMEXPORT BOOL     Voice_Paused(MD_VOICESET *vs, uint voice);

MMEXPORT ulong    Voice_RealVolume(MD_VOICESET *vs, uint voice);

MMEXPORT int      Voice_Find(MD_VOICESET *vs, uint flags);


// Lower level 'stuff'
// -------------------

// use macro Mikmod_RegisterDriver instead
MMEXPORT void     MD_RegisterDriver(MD_DEVICE *drv);

// loads a sample based on the give SAMPLOAD structure into the driver
MMEXPORT int      MD_SampleLoad(MDRIVER *md, int type, SAMPLOAD *s);

// allocates a chunk of memory without loading anything into it.
MMEXPORT int      MD_SampleAlloc(MDRIVER *md, int type);
MMEXPORT void     MD_SampleUnload(MDRIVER *md, int handle);

// returns the actual amount of memory a sample will use.  Necessary because
// some drivers may only support 8 or 16 bit data, or may not support stereo.
MMEXPORT ulong    MD_SampleLength(MDRIVER *md, int type, SAMPLOAD *s);
MMEXPORT ulong    MD_SampleSpace(MDRIVER *md, int type);  // free sample memory in driver.


// ================================================================
// Declare external drivers
// (these all come with various mikmod packages):
// ================================================================

// Multi-platform jazz...
MMEXPORT MD_DEVICE drv_nos;      // nosound driver, REQUIRED!
MMEXPORT MD_DEVICE drv_raw;      // raw file output driver [music.raw]
MMEXPORT MD_DEVICE drv_wav;      // RIFF WAVE file output driver [music.wav]

// Windows95 drivers:
MMEXPORT MD_DEVICE drv_win;      // windows media (waveout) driver
MMEXPORT MD_DEVICE drv_ds;       // Directsound driver
MMEXPORT MD_DEVICE drv_dsaccel;  // accelerated directsound driver (sucks!)

// MS_DOS Drivers:
MMEXPORT MD_DEVICE drv_awe;      // experimental SB-AWE driver
MMEXPORT MD_DEVICE drv_gus;      // gravis ultrasound driver [hardware / software mixing]
MMEXPORT MD_DEVICE drv_gus2;     // gravis ultrasound driver [hardware mixing only]
MMEXPORT MD_DEVICE drv_sb;       // soundblaster 1.5 / 2.0 DSP driver
MMEXPORT MD_DEVICE drv_sbpro;    // soundblaster Pro DSP driver
MMEXPORT MD_DEVICE drv_sb16;     // soundblaster 16 DSP driver
MMEXPORT MD_DEVICE drv_ss;       // ensoniq soundscape driver
MMEXPORT MD_DEVICE drv_pas;      // PAS16 driver
MMEXPORT MD_DEVICE drv_wss;      // Windows Sound System driver

// Various UNIX/Linux drivers:
MMEXPORT MD_DEVICE drv_vox;      // linux voxware driver
MMEXPORT MD_DEVICE drv_AF;       // Dec Alpha AudioFile driver
MMEXPORT MD_DEVICE drv_sun;      // Sun driver
MMEXPORT MD_DEVICE drv_os2;      // Os2 driver
MMEXPORT MD_DEVICE drv_hp;       // HP-UX /dev/audio driver
MMEXPORT MD_DEVICE drv_aix;      // AIX audio-device driver
MMEXPORT MD_DEVICE drv_sgi;      // SGI audio-device driver
MMEXPORT MD_DEVICE drv_tim;      // timing driver
MMEXPORT MD_DEVICE drv_ultra;    // ultra driver for linux

// Very portable SDL driver:
MMEXPORT MD_DEVICE drv_sdl;	// Simple DirectMedia Layer output driver

#ifdef __cplusplus
}
#endif

#include "virtch.h"    // needed because mdriver has a virtch handle now.

#ifdef __cplusplus
extern "C" {
#endif
MMEXPORT void VC_EnableInterpolation(VIRTCH *vc, int handle);
MMEXPORT void VC_DisableInterpolation(VIRTCH *vc, int handle);
#ifdef __cplusplus
}
#endif

#endif

