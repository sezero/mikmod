/*

 MikMod Sound System

  By Jake Stine of Divine Entertainment (1996-1999)

 Support:
  If you find problems with this code, send mail to:
    air@divent.simplenet.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Header: uniform.h

  Uniformat structures and procedure prototypes.

*/
  
#ifndef _UNIFORM_H_
#define _UNIFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

// --------------------
// -/- UniMod flags -/-

#define UF_XMPERIODS        (1ul<<0)     // XM periods / finetuning / behavior
#define UF_LINEAR           (1ul<<1)     // LINEAR periods
#define UF_LINEAR_PITENV    (1ul<<2)     // linear slides on envelopes?
#define UF_INST             (1ul<<3)     // Instruments are used
#define UF_NNA              (1ul<<4)     // New Note Actions used (set numvoices rather than numchn)
#define UF_LINEAR_FREQ      (1ul<<5)     // Tran's frequency-linear mode (instead of periods)

#define UF_NO_PANNING       (1ul<<8)     // Disable 8xx panning effects
#define UF_NO_EXTSPEED      (1ul<<9)     // Disable PT extended speed.
#define UF_NO_RESONANCE     (1ul<<10)    // Disable 8xx panning effects

#define UF_EXTSAMPLES       (1ul<<12)    // uses extanded samples?

#define UFF_LOOP_PATTERNSCOPE (1ul<<20)  // used on UNI_LOOP global effect!


// Instrument format flags
// -----------------------

#define IF_OWNPAN          (1ul<<0)
#define IF_USE_CUTOFF      (1ul<<1)
#define IF_USE_RESONANCE   (1ul<<2)


// New Note Action Flags (NNAs) 
// ----------------------------
// (used by samps and insts)

#define NNA_CUT         0
#define NNA_CONTINUE    1
#define NNA_OFF         2
#define NNA_FADE        3


// Duplicate Note Check Flags
// --------------------------
//  - Duplicate Check Type (DCT)
//  - Duplicate Check Action (DCA)

#define DCT_OFF         0
#define DCT_NOTE        1
#define DCT_SAMPLE      2
#define DCT_INST        3

#define DCA_CUT         0
#define DCA_OFF         1
#define DCA_FADE        2


// Envelope flags
// --------------
// Note on EF_VOLENV:  That specifies a 'volume envelope' stype of handling,
// where the fadeout is activated when the end of the envelope is reached.
// Used by ITs, but not by XMs.

#define EF_ON          (1ul<<0)
#define EF_SUSTAIN     (1ul<<1)
#define EF_LOOP        (1ul<<2)
#define EF_VOLENV      (1ul<<3)
#define EF_INCLUSIVE   (1ul<<4)      // ITs are inclusive, XMs are exclusive.
#define EF_CARRY       (1ul<<5)      // carries the volume envelope info over.


// Miscellaneous flags
// -------------------
//  - Keyoff control (Kick, Fade, Kill)
//  - Auto-vibrato (IT/XM mode)

#define KEY_KICK        0
#define KEY_OFF         1
#define KEY_FADE        2
#define KEY_KILL        3

#define AV_IT           1   // IT vs. XM vibrato info


// UNITRK : Repeat / Length flags and masks
// ----------------------------------------

#define TFLG_LENGTH_MASK       63
#define TFLG_NOTE              64
#define TFLG_EFFECT           128


// UNITRK : Effect Flags
// ---------------------

#define TFLG_EFFECT_MEMORY       1
#define TFLG_EFFECT_INFO         2
#define TFLG_PARAM_POSITIVE      4
#define TFLG_PARAM_NEGATIVE      8
#define TFLG_OVERRIDE_EFFECT    16
#define TFLG_OVERRIDE_FRAMEDLY  32

// UNITRK :  Framedelay bitwise constants
// --------------------------------------

#define UFD_TICKMASK  127
#define UFD_RUNONCE   128


// UNITRK : Enumerated UniMod Commands
// -----------------------------------
//  Commands are divided into two segments:  Global commands and
//  channel commands.

enum      // Global UniMod commands.
{   
    UNI_GLOB_VOLUME = 1,
    UNI_GLOB_VOLSLIDE,
    UNI_GLOB_FINEVOLSLIDE,
    UNI_GLOB_TEMPO,
    UNI_GLOB_TEMPOSLIDE,
    UNI_GLOB_SPEED,
    UNI_GLOB_LOOPSET,
    UNI_GLOB_LOOP,
    UNI_GLOB_DELAY,
    UNI_GLOB_PATJUMP,
    UNI_GLOB_PATBREAK,
    UNI_GLOB_LAST
};

enum
{   UNI_ARPEGGIO = 1,
    UNI_VOLUME,
    UNI_CHANVOLUME,
    UNI_PANNING,
    UNI_VOLSLIDE,
    UNI_CHANVOLSLIDE,
    UNI_TREMOLO_SPEED,
    UNI_TREMOLO_DEPTH,
    UNI_TREMOR,

    UNI_PITCHSLIDE,
    UNI_VIBRATO_SPEED,
    UNI_VIBRATO_DEPTH,
    UNI_PORTAMENTO_LEGACY,
    UNI_PORTAMENTO,
    UNI_PORTAMENTO_BUGGY,
    UNI_PORTAMENTO_TRAN,

    UNI_PANSLIDE,
    UNI_PANBRELLO_SPEED,
    UNI_PANBRELLO_DEPTH,

    UNI_VIBRATO_WAVEFORM,
    UNI_TREMOLO_WAVEFORM,
    UNI_PANBRELLO_WAVEFORM,

    UNI_NOTEKILL,
    UNI_NOTEDELAY,
    UNI_RETRIG,
    UNI_OFFSET_LEGACY,
    UNI_OFFSET,
    UNI_KEYOFF,
    UNI_KEYFADE,

    UNI_ENVELOPE_CONTROL,
    UNI_NNA_CONTROL,
    UNI_NNA_CHILDREN,

    UNI_FILTER_CUTOFF,
    UNI_FILTER_RESONANCE,

    UNI_SETSPEED,
    UNI_SETSPEED_TRAN,

    UNI_LAST
};

enum
{   UNIMEM_NONE = 0,
    PTMEM_PITCHSLIDEUP,
    PTMEM_PITCHSLIDEDN,
    PTMEM_VIBRATO_SPEED,
    PTMEM_VIBRATO_DEPTH,
    PTMEM_TREMOLO_SPEED,
    PTMEM_TREMOLO_DEPTH,
    PTMEM_TEMPO,
    PTMEM_PORTAMENTO,
    PTMEM_OFFSET,
    PTMEM_LAST
};


// Module-only Playback Flags
// --------------------------
// PSF flags (Player Sample Format): 
//
// PSF_OWNPAN    -  Modules can optional enable or disable a sample from using
//   its set default panning positon, in which case the current channel panning
//   position is used.
// PSF_UST_LOOP  -  UST modules treat the sample loopstart/loop begin differently
//   from normal loops.  The sample only plays the looped portion, ignoring the
//   start of the sample completely.

#define PSF_OWNPAN          (1ul<<16)
#define PSF_UST_LOOP        (1ul<<17)

// =====================================================================================
    typedef struct UNISAMPLE
// =====================================================================================
// Mikmod's custom module player sample structure, used in the place of the MD_SAMPLE,
// which I normally use in applications for sound effects.  Reason: this has a lot of
// extra little tidbits which would be otherwise worthless in a sound effects situaiton.
{
    uint   flags;           // looping and player flags!
    int    handle;          // handle to the sample loaded into the driver

    uint   speed;           // default playback speed (almost anything is legal)
    int    volume;          // 0 to 128 volume range
    int    panning;         // PAN_LEFT to PAN_RIGHT range

    uint   length;          // Length of the sample (samples, not bytes!)
    uint   loopstart,       // Sample looping smackdown!
           loopend,
           susbegin,        // sustain loop begin (in samples)
           susend;          // sustain loop end

    int    cutoff,          // the cutoff frequency (range 0 to 16384)
           resonance;       // the resonance factor (range -128 to 128)

    CHAR  *samplename;      // name of the sample    

    BOOL   used;

    // Sample Loading information - relay info between the loaders and
    // unimod.c - like wow dude!

    uint   format;          // diskformat flags, describe sample prior to loading!
    uint   compress;        // compression status of the sample.
    long   seekpos;         // seekpos within the module.

} UNISAMPLE;


// UNITRK : Structures and Unions
// ------------------------------
// These structures have 1 byte packing else they eat LOTS of memory (since we
// will be using a possible several thousand of these babies per module).

#ifndef __GNUC__
#pragma pack (push,1)
#endif

typedef union _BYTE_MOB
{   UBYTE      u;
    SBYTE      s;
} BYTE_MOB;

typedef union _WORD_MOB
{   UWORD      u;
    SWORD      s;
} WORD_MOB;

typedef union _INT_MOB
{   ULONG      u;
    SLONG      s;

    struct _WORD_PAIR
    {   WORD_MOB  loword;
        WORD_MOB  hiword;
    };

    struct _BYTE_BUNCH
    {   UBYTE     byte_a, byte_b,
                  byte_c, byte_d;
    };
} INT_MOB;


typedef struct _UNITRK_NOTE
{   UBYTE    note, inst;
} UNITRK_NOTE;


// This is mighty creul of me.  Two structures of almost the same name.
// The first one (UNITRK_EFFECT) is used by the player and the loaders.  It is a memory-less
// effect structure.  The second (UE_EFFECT) has memory info in it, and is for use
// by trackers and info viewers.

typedef struct _UNITRK_EFFECT
{   INT_MOB  param;             // Parameter
    UBYTE    effect;            // Effect 
    UBYTE    framedly;          // framedelay for effect.  If UFD_RUNONCE is set,
                                // command is run once on the given tick.
} UNITRK_EFFECT;


//--------------
// UE_EFFECT - Unitrk Extended Effect Structure.
//  This structure contains the memory information in addition to the
//  standard effect information.
//
// Flags:
//  UEF_GLOBAL  - Is a global effect, which means the chnslot var is valid.
//  UEF_MEMORY  - Set if it is a memory (ie, no effect data).  Effect data will
//                be invaild if this flag is set.

#define UEF_GLOBAL  1
#define UEF_MEMORY  2

typedef struct _UE_EFFECT
{   int           flags;
    UNITRK_EFFECT effect;
    int           memslot,      // Memory slot effect uses.
                  memchan;      // Only valid if UEF_GLOBAL flag is set.
} UE_EFFECT;

typedef struct _UNITRK_ROW
{   int    pos;
    UBYTE *row;
} UNITRK_ROW;


#ifndef __GNUC__
#pragma pack (pop)
#endif

// =====================================================================================
    typedef struct _UNI_EFFTRK
// =====================================================================================
// inuse member:
// The inuse is used to determine if a track is blank or not.  utrk_dup has no easy way
// to tell if the track has meaningful content or is simply filled with a series of 0's.
// So, inuse is set to 1 by utrk_newline whenever a row actually has any meaningful content.
// reallen:
// This has been added more recently, and gives the actual length of the track (minus all
// the blank rows at the end of the track).  This member effectively makes inuse obsolete.
// I might remove it later, but for now, it ain't broke and I ain't fixing it!
{
    UBYTE    *buffer;          // unitrk buffer allocation!
    int      writepos;         // current writepos in the buffer (and length!)

    UBYTE    *output;
    int      outsize;          // current buffer size (increases as needed).
    int      outpos;

    int      note, inst;

    BOOL     inuse;            // indicates if track is in use or just blank
    int      reallen;          // the length of the 'used' stream (minus trailing empty rows)
} UNI_EFFTRK;


// =====================================================================================
    typedef struct _UNI_GLOBTRK
// =====================================================================================
{
    UBYTE    *buffer;          // unitrk buffer allocation!
    int      writepos;         // current writepos in the buffer (and length!)

    UBYTE    *output;
    int      outsize;          // current buffer size (increases as needed).
    int      outpos;

    BOOL     inuse;            // indicates if track is in use or just blank
    int      reallen;          // the length of the 'used' stream (minus trailing empty rows)
} UNI_GLOBTRK;


// =====================================================================================
    typedef struct _UTRK_WRITER
// =====================================================================================
{
    UNI_EFFTRK   *curtrk, *unitrk;
    UNI_GLOBTRK   globtrk;
    int           curchn, unichn;

    int           trkwrite;
    int           patwrite;

    UBYTE         lmem_flag[64];
    MM_ALLOC     *allochandle;

} UTRK_WRITER;


#ifndef __GNUC__
#pragma pack (push,2)
#endif

typedef struct ENVPT
{   UWORD pos;
    SWORD val;
} ENVPT;


// =====================================================================================
    typedef struct EXTSAMPLE
// =====================================================================================
// Each extanded sample info block extends the information for each corresponding core
// SAMPLE structure in the samples[] array in the UNIMOD struct (ie same index for ext-
// samples and samples arrays - info for same sample).  
{   
    UBYTE  globvol;          // global volume
    UBYTE  nnatype;

    SBYTE  pitpansep;        // pitch pan separation (-64 to 64)
    UBYTE  pitpancenter;     // pitch pan center (0 to 119)
    UBYTE  rvolvar;          // random volume varations (0 - 100%)
    UBYTE  rpanvar;          // random panning varations (0 - 100%)
    UWORD  volfade;

    UBYTE  volflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  volpts;
    UBYTE  volsusbeg;
    UBYTE  volsusend;
    UBYTE  volbeg;
    UBYTE  volend;
    ENVPT  *volenv;

    UBYTE  panflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  panpts;
    UBYTE  pansusbeg;
    UBYTE  pansusend;
    UBYTE  panbeg;
    UBYTE  panend;
    ENVPT  *panenv;

    UBYTE  pitflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  pitpts;
    UBYTE  pitsusbeg;
    UBYTE  pitsusend;
    UBYTE  pitbeg;
    UBYTE  pitend;
    ENVPT  *pitenv;

    UBYTE  vibflags;         // set = IT vibrato type, clear = XM type
    UBYTE  vibtype;
    UBYTE  vibsweep;
    UBYTE  vibdepth;
    UBYTE  vibrate;

    UWORD  avibpos;          // autovibrato pos [internal use only]
} EXTSAMPLE;


// =====================================================================================
    typedef struct INSTRUMENT
// =====================================================================================
{   
    UBYTE  flags;

    UBYTE  samplenumber[120];
    UBYTE  samplenote[120];

    UBYTE  dct;              // duplicate check type
    UBYTE  dca;              // duplicate check action

    UBYTE  globvol;          // global volume
    SWORD  panning;          // instrument-based panning var
    UBYTE  nnatype;
    
    SBYTE  pitpansep;        // pitch pan separation (-64 to 64)
    UBYTE  pitpancenter;     // pitch pan center (0 to 119)
    UBYTE  rvolvar;          // random volume varations (0 - 100%)
    UBYTE  rpanvar;          // random panning varations (0 - 100%)

    UWORD  volfade;          // fadeout rate

    UBYTE  volflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  volpts;
    UBYTE  volsusbeg;
    UBYTE  volsusend;
    UBYTE  volbeg;
    UBYTE  volend;
    ENVPT  volenv[32];

    UBYTE  panflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  panpts;
    UBYTE  pansusbeg;
    UBYTE  pansusend;
    UBYTE  panbeg;
    UBYTE  panend;
    ENVPT  panenv[32];

    UBYTE  pitflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  pitpts;
    UBYTE  pitsusbeg;
    UBYTE  pitsusend;
    UBYTE  pitbeg;
    UBYTE  pitend;
    ENVPT  pitenv[32];

    UBYTE  resflg;           // bit 0: on 1: sustain 2: loop
    UBYTE  respts;
    UBYTE  ressusbeg;
    UBYTE  ressusend;
    UBYTE  resbeg;
    UBYTE  resend;
    ENVPT  resenv[32];

    UBYTE  vibflags;
    UBYTE  vibtype;
    UBYTE  vibsweep;
    UBYTE  vibdepth;
    UBYTE  vibrate;

    int    cutoff,           // the cutoff frequency (range 0 to 16384)
           resonance;        // the resonance factor (range -128 to 128)

    CHAR   *insname;

    UWORD  avibpos;          // autovibrato pos [internal use only]
} INSTRUMENT;

#ifndef __GNUC__
#pragma pack (pop)
#endif

// =====================================================================================
    typedef struct UNIMOD
// =====================================================================================
{
    // This section of elements are all file-storage related.
    // all of this information can be found in the UNIMOD disk format.
    // For further details about there variables, see the MikMod Docs.

    uint        flags;          // See UniMod Flags above
    uint        numchn;         // number of module channels
    uint        numvoices;      // voices allocated to NNA playback
    long        songlen;        // length of the song as predicted by PredictSongLength (1/1000th of a second).
    long        filesize;       // size of the module file, in bytes
    uint        memsize;        // number of per-channel effect memory entires (a->memory) to allocate.

    uint        numpos;         // number of positions in this song
    uint        numpat;         // number of patterns in this song
    uint        numtrk;         // number of tracks
    uint        numins;         // number of instruments
    uint        numsmp;         // number of samples
    int         reppos;         // restart position
    uint        initspeed;      // initial song speed
    uint        inittempo;      // initial song tempo
    uint        initvolume;     // initial global volume (0 - 128)

    int         panning[64];    // 64 initial panning positions
    uint        chanvol[64];    // 64 initial channel volumes
    BOOL        muted[64];      // 64 muting flags (I really should change this to a struct)

    int		pansep;		// Panning separation (0=mono, 128=full stereo).

    CHAR       *songname;       // name of the song
    CHAR       *filename;       // name of the file this song came from (optional)
    CHAR       *composer;       // name of the composer
    CHAR       *comment;        // module comments
    CHAR       *modtype;        // string type of module loaded

    // Instruments and Samples!

    INSTRUMENT *instruments;    // all instruments
    UNISAMPLE  *samples;        // all samples
    EXTSAMPLE  *extsamples;     // all extended sample-info (corresponds to each sample)

    // UniTrack related -> Tracks and reading them!
        
    UBYTE     **tracks;         // array of numtrk pointers to tracks
    UBYTE     **globtracks;     // array of numpat pointers to global tracks
    UWORD      *patterns;       // array of Patterns [index to tracks for each channel].
    UWORD      *pattrows;       // array of number of rows for each pattern
    UWORD      *positions;      // all positions

    UBYTE       memflag[64];    // flags for each memory slot (alloc'd to mf->memsize)

    //void      (*localeffects)(int effect, INT_MOB dat);
    //void      (*globaleffects)(int effect, INT_MOB dat);

    UTRK_WRITER *ut;
    MDRIVER     *md;             // the driver this module is bound to.
    MM_ALLOC    *allochandle;

    long         strip_threshold;  // time, in milliseconds, before shortening song.
    int          sngpos_silence;   // number of positions in this song
    int          patpos_silence;   // number of patterns in this song
} UNIMOD;


// ====================
// Unitrack prototypes:
// ====================

MMEXPORT UBYTE utrk_blanktrack[2];

MMEXPORT void    pt_write_effect(UTRK_WRITER *ut, uint eff, uint dat);
MMEXPORT void    pt_write_exx(UTRK_WRITER *ut, uint eff, uint dat);
MMEXPORT void    pt_global_consolidate(UNIMOD *of, UBYTE **globtrack);

MMEXPORT UBYTE  *utrk_global_copy(UTRK_WRITER *ut, UBYTE *track, int chn);


// These should be called by the player only (special coded to work
// directly with the unimod structure in a simple and efficient manner).

MMEXPORT uint    utrk_global_getlength(UBYTE *track);
MMEXPORT void    utrk_global_seek(UNITRK_ROW *urow, UBYTE *track, int row);
MMEXPORT void    utrk_global_nextrow(UNITRK_ROW *urow);
MMEXPORT BOOL    utrk_global_geteffect(UE_EFFECT *eff, UNITRK_ROW *urow);

MMEXPORT uint    utrk_local_getlength(UBYTE *track);
MMEXPORT void    utrk_local_seek(UNITRK_ROW *urow, UBYTE *track, int row);
MMEXPORT void    utrk_local_nextrow(UNITRK_ROW *urow);
MMEXPORT BOOL    utrk_local_geteffect(UE_EFFECT *eff, UNITRK_ROW *urow);
MMEXPORT void    utrk_getnote(UNITRK_NOTE *note, UNITRK_ROW *urow);

MMEXPORT UTRK_WRITER *utrk_init(int nc, MM_ALLOC *ahparent);
MMEXPORT uint    utrk_cleanup(UTRK_WRITER *ut);
MMEXPORT void    utrk_local_memflag(UTRK_WRITER *ut, int memslot, int eff, int fdly);
MMEXPORT void    utrk_memory_reset(UTRK_WRITER *ut);

MMEXPORT void    utrk_reset(UTRK_WRITER *ut);
MMEXPORT void    utrk_settrack(UTRK_WRITER *ut, int trk);
MMEXPORT void    utrk_write_global(UTRK_WRITER *ut, const UNITRK_EFFECT *data, int memslot);
MMEXPORT void    utrk_write_local(UTRK_WRITER *ut, const UNITRK_EFFECT *data, int memslot);
MMEXPORT void    utrk_memory_global(UTRK_WRITER *ut, const UNITRK_EFFECT *data, int memslot);
MMEXPORT void    utrk_memory_local(UTRK_WRITER *ut, const UNITRK_EFFECT *data, int memslot, int signs);
MMEXPORT void    utrk_write_inst(UTRK_WRITER *ut, unsigned int ins);
MMEXPORT void    utrk_write_note(UTRK_WRITER *ut, unsigned int note);
MMEXPORT void    utrk_newline(UTRK_WRITER *ut);

MMEXPORT UBYTE   *utrk_dup_track(UTRK_WRITER *ut, int chn, MM_ALLOC *allochandle);
MMEXPORT UBYTE   *utrk_dup_global(UTRK_WRITER *ut, MM_ALLOC *allochandle);
MMEXPORT BOOL    utrk_dup_pattern(UTRK_WRITER *ut, UNIMOD *mf);

MMEXPORT void    utrk_optimizetracks(UNIMOD *mf);

//extern UWORD   TrkLen(UBYTE *trk);
//extern BOOL    MyCmp(UBYTE *a, UBYTE *b, UWORD l);


/***************************************************
****** Loader stuff: *******************************
****************************************************/

typedef void ML_HANDLE;

// loader structure:

// =====================================================================================
    typedef struct MLOADER
// =====================================================================================
{   
    CHAR    *Type;
    CHAR    *Version;

    int      defpan;                         // default channel panning for this module.

    // Below is the 'private' stuff, to be used by the MDRIVER and the
    // driver modules themselves [commenting is my version of C++]

    struct MLOADER *next;

    BOOL       (*Test)(MMSTREAM *modfp);
    ML_HANDLE *(*Init)(void);                // creates an instance handle for loading songs.
    void       (*Cleanup)(ML_HANDLE *inst);  // clean up (free) instance

    BOOL       (*Load)(ML_HANDLE *handle, UNIMOD *of, MMSTREAM *mmfile);
    CHAR      *(*LoadTitle)(MMSTREAM *mmfile);

    BOOL       enabled;                      // enabled at registration.
    BOOL       nopaneff;                     // disable panning effects (sets UF_NO_PANNING)
    BOOL       noreseff;                     // disable resonance (midi macro)
} MLOADER;

// public loader variables:

extern UWORD  finetune[16];
extern UWORD  npertab[60];          // used by the original MOD loaders

// main loader prototypes:
// -----------------------
// These are defined all over the place, I really should reorganize them.

MMEXPORT void     MikMod_RegisterAllLoaders(void);
MMEXPORT MLOADER *MikMod_LoaderInfo(int loader);
MMEXPORT int      MikMod_GetNumDrivers(void);

MMEXPORT UNIMOD  *Unimod_LoadFP(MDRIVER *md, MMSTREAM *modfp, MMSTREAM *smpfp, int mode);
MMEXPORT UNIMOD  *Unimod_Load(MDRIVER *md, const CHAR *filename);
MMEXPORT UNIMOD  *Unimod_LoadInfo(const CHAR *filename);
MMEXPORT void     Unimod_Free(UNIMOD *mf);

MMEXPORT void     Unimod_SetDefaultPan(int defpan);
MMEXPORT BOOL     Unimod_LoadSamples(UNIMOD *mf, MDRIVER *md, MMSTREAM *smpfp);
MMEXPORT void     Unimod_UnloadSamples(UNIMOD *mf);

MMEXPORT void     Unimod_StripSilence(UNIMOD *mf, long threshold);

MMEXPORT void     ML_RegisterLoader(MLOADER *ldr);  // use the macro MikMod_RegisterLoader instead


// other loader prototypes: (used by the loader modules)

//extern BOOL    InitTracks(void);
extern void     AddTrack(UBYTE *tr);
extern BOOL     AllocPositions(UNIMOD *of,int total);
extern BOOL     AllocPatterns(UNIMOD *of);
extern BOOL     AllocTracks(UNIMOD *of);
extern BOOL     AllocInstruments(UNIMOD *of);
extern BOOL     AllocSamples(UNIMOD *of, BOOL ext);
extern CHAR    *DupStr(MM_ALLOC *allochandle, CHAR *s, UWORD len);


// Declare external loaders:

MMEXPORT MLOADER  load_uni;        // Internal UniMod Loader (Current version of UniMod only)
MMEXPORT MLOADER  load_mod;        // Standard 31-instrument Module loader (Protracker, StarTracker, FastTracker, etc)
MMEXPORT MLOADER  load_m15;        // 15-instrument (SoundTracker and Ultimate SoundTracker)
MMEXPORT MLOADER  load_mtm;        // Multi-Tracker Module (by Renaissance)
MMEXPORT MLOADER  load_s3m;        // ScreamTracker 3 (by Future Crew)
MMEXPORT MLOADER  load_stm;        // ScreamTracker 2 (by Future Crew)
MMEXPORT MLOADER  load_ult;        // UltraTracker 
MMEXPORT MLOADER  load_xm;         // FastTracker 2 (by Trition)
MMEXPORT MLOADER  load_it;         // Impulse Tracker (by Jeffrey Lim)
MMEXPORT MLOADER  load_669;        // 669 and Extended-669 (by Tran / Renaissance)
MMEXPORT MLOADER  load_dsm;        // DSIK internal module format
MMEXPORT MLOADER  load_med;        // MMD0 and MMD1 Amiga MED modules (by OctaMED)
MMEXPORT MLOADER  load_far;        // Farandole Composer Module

#ifdef __cplusplus
}
#endif

#endif

