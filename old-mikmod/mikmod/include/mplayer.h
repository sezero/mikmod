#ifndef _MPLAYER_H_
#define _MPLAYER_H_

#include "uniform.h"
#include "mmforbid.h"

#define MUTE_EXCLUSIVE  32000
#define MUTE_INCLUSIVE  32001

// -------------------------------------------
// -/- Master Player Flags (player->flags) -/-

#define PF_FORBID    (1ul<<0)
#define PF_LOOP      (1ul<<1)
#define PF_NNA       (1ul<<2)
#define PF_TIMESEEK  (1ul<<3)


// ------------------------------------
// -/- VolumeFade Flags and Defines -/-

#define MP_VOLUME_CUR    -1

enum
{
    MP_SEEK_SET = 0,
    MP_SEEK_CUR,
    MP_SEEK_END
};


#define MEMFLAG_LOCAL   1
#define MEMFLAG_GLOBAL  2

#ifndef __GNUC__
#pragma pack (push,1)
#endif

// -------------------------------------------------------------------------------------
    typedef struct MP_EFFMEM
// -------------------------------------------------------------------------------------
// Effect memory structure.  Flag indicates whether this is a global or local effect. 
// Sloppy, but works.
{   
    UBYTE          flag;
    UNITRK_EFFECT  effect;

} MP_EFFMEM;

#ifndef __GNUC__
#pragma pack (pop)
#pragma pack (push,4)
#endif

// -------------------------------------------------------------------------------------
    typedef struct _ENVPR
// -------------------------------------------------------------------------------------
{   
    UBYTE pts;          // number of envelope points
    UBYTE susbeg;       // envelope sustain index begin
    UBYTE susend;       // envelope sustain index end
    UBYTE beg;          // envelope loop begin
    UBYTE end;          // envelope loop end
    ENVPT *env;         // envelope points

    // Dyamic data modified by the envelope handler
    
    UWORD p;            // current envelope counter
    int   a;            // envelope index a
    int   b;            // envelope index b

} ENVPR;


// -------------------------------------------------------------------------------------
    typedef struct _MP_SIZEOF
// -------------------------------------------------------------------------------------
{
    INSTRUMENT  *i;
    UNISAMPLE   *s;
    EXTSAMPLE   *es;

    uint    sample;       // which instrument number
    int     volume;       // output volume (vol + sampcol + instvol)
    int     panning;      // panning position
    int     chanvol;      // channel's "global" volume
    uint    fadevol;      // fading volume rate
    uint    period;       // period to play the sample at

    SWORD   handle;       // which sample-handle
    long    start;        // The start byte index in the sample
    long    setpos;       // new setposition index for the sample.

    UBYTE   volflg;       // volume envelope settings
    UBYTE   panflg;       // panning envelope settings
    UBYTE   pitflg;       // pitch envelope settings
    UBYTE   resflg;       // resonance filter envelope settings

    UBYTE   note;         // the audible note (as heard, direct rep of period)
    UBYTE   nna;          // New note action type + master/slave flags
    UBYTE   kick;         // if true = sample has to be restarted

    int     cutoff;       // Resonance filter cutoff (harmonic scale)
    uint    resonance;    // Resonance filter resonance.

} MP_SIZEOF;


// -------------------------------------------------------------------------------------
    typedef struct _MP_VOICE
// -------------------------------------------------------------------------------------
{   
    MP_SIZEOF  shared;

    UBYTE  keyoff;        // if true = fade out and stuff

    ENVPR  venv;
    ENVPR  penv;
    ENVPR  cenv;
    ENVPR  renv;

    UWORD  avibpos;      // autovibrato pos
    UWORD  aswppos;      // autovibrato sweep pos

    ULONG  totalvol;     // total volume of channel (before global mixings)

    BOOL   mflag;
    SWORD  masterchn;
    struct _MP_CONTROL *master;// index of "master" effects channel

} MP_VOICE;


// -------------------------------------------------------------------------------------
    typedef struct _MP_CONTROL
// -------------------------------------------------------------------------------------
{
    MP_SIZEOF  shared;

    int    volume;        // output volume (vol + sampcol + instvol)
    int    tmpvolume;     // tmp volume
    uint   tmpperiod;     // tmp period
    uint   wantedperiod;  // period to slide to (with effect 3 or 5)

    MP_VOICE *slave;      // Audio Slave of current effects control channel

    UBYTE  keyoff;        // if true = fade out and stuff
    UBYTE  muted;         // if set, channel not played
    UBYTE  notedelay;     // (used for note delay)

    UWORD  slavechn;      // Audio Slave of current effects control channel
    UBYTE  anote;         // the note that indexes the audible (note seen in tracker)

    SWORD  ownper;        // internal for effects-period management across multiple effects
    SWORD  ownvol;        // internal for effects-volume management across multiple effects
    UBYTE  dca;           // duplicate check action
    UBYTE  dct;           // duplicate check type
    SBYTE  retrig;        // retrig value (0 means don't retrig)
    ULONG  speed;         // what finetune to use

    UBYTE  glissando;     // glissando (0 means off)
    UBYTE  wavecontrol;   //

    SBYTE  vibpos;        // current vibrato position
    SBYTE  trmpos;        // current tremolo position
    UBYTE  tremor;

    ULONG  soffset;       // last used low order of sample-offset (effect 9)

    UBYTE  panbwave;      // current panbrello waveform
    SBYTE  panbpos;       // current panbrello position

    UNISAMPLE *old_s;     // portamento crap - for sample override in inst mode.
    UWORD  newsamp;       // set to 1 upon a sample / inst change
    UWORD  offset_lo, offset_hi;

    // Pattern loop (PT E6x Effect) is now channel-based per PT-std.

    UWORD  rep_patpos;     // patternloop position (row)
    SWORD  rep_sngpos;     // patternloop position (pattern order)
    UWORD  pat_repcnt;     // times to loop

    // UniTrack Stuff (current row / position index)

    UBYTE  *row;           // row currently playing on this channel
    int    pos;            // current position to read from in a->row
    MP_EFFMEM *memory;     // memory space used by effects (munitrk.c).

} MP_CONTROL;


// -------------------------------------------------------------------------------------
    typedef struct MP_STATE
// -------------------------------------------------------------------------------------
{
    // All following variables can be modified at any time.

    UBYTE       bpm;            // current beats-per-minute speed
    UWORD       sngspd;         // current song speed
    int         channel;        // current working channel in the module.
    int         volume;         // global volume (0-128)

    // The following variables are considered useful for reading, and should
    // not be directly modified by the end user.

    int         panning[64];    // current channel panning positions
    int         chanvol[64];    // current channel volumes
    MP_CONTROL *control;        // Effects Channel information (pf->numchn alloc'ed)
    MP_VOICE   *voice;          // Voice information

    long        curtime;        // current time in the song in milliseconds.
    int         looping;        // current loopcount iteration
    UWORD       numrow;         // number of rows on current pattern
    UWORD       vbtick;         // tick counter (counts from 0 to sngspd)
    UWORD       patpos;         // current row number
    SWORD       sngpos;         // current song position.  This should not
                                // be modified directly.  Use MikMod_NextPosition,
                                // MikMod_PrevPosition, and MikMod_SetPosition.

    // The following variables should not be modified, and have information
    // that is pretty useless outside the internal player, so just ignore :)

    UWORD       patbrk;         // position where to start a new pattern
    UBYTE       patdly;         // patterndelay counter (command memory)
    UBYTE       patdly2;        // patterndelay counter (real one)
    UBYTE       framedly;       // frame-based patterndelay
    SWORD       posjmp;         // flag to indicate a position jump is needed...
                                //  changed since 1.00: now also indicates the
                                //  direction the position has to jump to:
                                //  0: Don't do anything
                                //  1: Jump back 1 position
                                //  2: Restart on current position
                                //  3: Jump forward 1 position

    ULONG     **pos_played;     // Indicates if the position has been played.
    int         patloop;        // incremented everytime a pattern loop is active.

    int         globtrk_pos;    // Global track current row
    UBYTE      *globtrk_row;    // global track!

    long        strip_timeout;  // for stripping out the end of songs.

} MP_STATE;


// -------------------------------------------------------------------------------------
    typedef struct MP_VOLFADER
// -------------------------------------------------------------------------------------
// Block of information set by Player_VolumeFade and Player_VolumeFadeEx, and used by the
// player updater to fade the song appropriately!
{
    int    startvol,       // starting volume (if == MP_VOLUME_CUR, then use current volume)
           destvol;        // destination volume (if == MP_VOLUME_CUR, then use current volume)

    long   starttime,      // starttime of the fade     (in milliseconds*64)
           desttime,       // destination time of fade  (in milliseconds*64)
           differential;   // differential factor       (in milliseconds*64)

} MP_VOLFADER;


// -------------------------------------------------------------------------------------
    typedef struct MPLAYER
// -------------------------------------------------------------------------------------
{
    BOOL          ended;         // set to 1 when the song ends naturally.
    MP_STATE      state;         // current state information

    struct MPLAYER *next;

    const UNIMOD *mf;            // the module associated with this information!
    uint          flags;
    int           numvoices;     // number of voices this song is allocated to use!
    int           loopcount;     // number of times we are looping
    long          songlen;       // length of the song in milliseconds.

    MD_VOICESET  *vs;            // voiceset this player instance is attached to.
    MM_FORBID    *mutex;         // multithread mutex jazz..
    MM_ALLOC     *allochandle;
    MM_ALLOC     *statealloc;

    // Advanced look-up table for skipping around in a song.  The state of the
    // player is recorded for each pattern by the pmstate structure.

    MP_STATE   *statelist;
    int         pms_alloc;       // current max allocation (for reallocation stuffs)
    int         statecount;
    int         old_sngpos;

    // Feature which tracks various pieces of information which are useful
    // in an end-user statistical sense only (ie, of no real use to the player code)

    BOOL        *useins;         // flagged true when an instrument is used.
    BOOL        *usesmp;         // flagged true when a sample is used

    // song fading features

    int         fadespeed;
    int         fadedest;
    void      (*fadepostfunc)(void *handle);
    void       *fadepostvoid;

    MP_VOLFADER volfade;
} MPLAYER;

#ifndef __GNUC__
#pragma pack (pop)
#endif


// MikMod Player Prototypes:
// ===========================================================

#ifdef __cplusplus
extern "C" {
#endif

MMEXPORT MPLAYER *Player_Create(const UNIMOD *mf, uint flags);
MMEXPORT MPLAYER *Player_InitSong(const UNIMOD *mf, MD_VOICESET *owner, uint flags, uint maxvoices);
MMEXPORT void     Player_FreeSong(MPLAYER *ps);
MMEXPORT void     Player_Register(MD_VOICESET *vs, MPLAYER *ps, uint maxchan);
MMEXPORT void     Player_Free(MPLAYER *ps);

MMEXPORT void     Player_Cleaner(MPLAYER *ps);
MMEXPORT void     Player_WipeVoices(MPLAYER *ps);

MMEXPORT BOOL     Player_SetLoopStatus(MPLAYER *ps, BOOL loop, int loopcount);

MMEXPORT BOOL     Player_Active(MPLAYER *ps);
MMEXPORT void     Player_Deactivate(void);

MMEXPORT void     Player_Start(MPLAYER *mp);
MMEXPORT void     Player_Stop(MPLAYER *mp);
MMEXPORT void     Player_Pause(MPLAYER *ps, BOOL nosilence);
MMEXPORT void     Player_TogglePause(MPLAYER *mp, BOOL noslience);
MMEXPORT BOOL     Player_Paused(MPLAYER *ps);

MMEXPORT int      Player_GetVolume(MPLAYER *ps);
MMEXPORT void     Player_SetVolume(MPLAYER *mp, int volume);
MMEXPORT void     Player_VolumeFade(MPLAYER *ps, int startvol, int destvol, long duration);
MMEXPORT void     Player_QuickFade(MPLAYER *ps, int start, int dest, int speed);
MMEXPORT void     Player_QuickFadeEx(MPLAYER *ps, int start, int dest, int speed, void (*callback), void *handle);
MMEXPORT void     Player_VolumeFadeEx(MPLAYER *ps, int startvol, int destvol, long starttime, int whence, long duration);

MMEXPORT BOOL     Player_Playing(MPLAYER *mp);
MMEXPORT void     Player_NextPosition(MPLAYER *mp);
MMEXPORT void     Player_PrevPosition(MPLAYER *mp);
MMEXPORT void     Player_SetPosition(MPLAYER *mp, int pos, int row);

MMEXPORT void     Player_Setpos_Time(MPLAYER *mp, long time);
MMEXPORT int      Player_GetChanVoice(MPLAYER *mp, uint chan);

MMEXPORT void __cdecl Player_Mute(MPLAYER *mp, uint arg1, ...);
MMEXPORT void __cdecl Player_Unmute(MPLAYER *mp, uint arg1, ...);
MMEXPORT void __cdecl Player_ToggleMute(MPLAYER *mp, uint arg1, ...);
MMEXPORT BOOL     Player_Muted(MPLAYER *mp, uint chan);

MMEXPORT uint     Player_HandleTick(MD_VOICESET *vs, MPLAYER *playerinfo);


// ---------------------------
// -/- Defined in SNGLEN.C -/-

MMEXPORT void Player_PredictSongLength(MPLAYER *ps);
MMEXPORT uint Player_BuildQuickLookups(MPLAYER *ps);
MMEXPORT void Player_SetPosTime(MPLAYER *ps, long time);
MMEXPORT void Player_PreProcessRow(MPLAYER *ps, MPLAYER *destps);
MMEXPORT void Player_Restore(MPLAYER *ps, uint stateidx);

#ifdef __cplusplus
}
#endif

#endif

