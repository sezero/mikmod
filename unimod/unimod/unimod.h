/*
    Unimod.

    VERY closely related to Air's MikMod implementation.  Very little code was directly used,
    but the loaders are all based on his, and the structures are more or less his as well.

    This is purely an in-memory representation of a MOD file.  Nothing here even closely relates to
    actually doing anything other than storing a MOD file.
*/

#pragma once

#include "types.h"
#include "UniSample.h"

namespace MikMod
{

    class Instrument { /* TODO */ };


    // This gets convoluted.  Sadly.
    // A mod consists of a number of patterns.
    // A pattern is a number of tracks.  One for each channel.
    // A track is a number of notes for one channel.

    // The usual visual representation is that a pattern is a 2D grid, with a list of tracks on it.
    // The mod consists of several of these.

    struct Note
    {
        int instrument;             // instrument index
        int note;                   // The note, fool.
        int effect;                 // Effect thingies I'm not sure.
        int effectdata;             // Fuck if I know.
    };

    struct Pattern
    {
        struct Track
        {
            vector<Note>    notes;
        };
        
        vector<Track> channel;
    };

    class Unimod
    {
    public:
        // Untimely ripped from Air's mikmod stuff.  All I did was C++ize it. -- andy

        // This section of elements are all file-storage related.
        // all of this information can be found in the UNIMOD disk format.
        // For further details about there variables, see the MikMod Docs.

//        u32        flags;         // See UniMod Flags above
        // Options
        bool bXmbehaviour;          // XP periods/finetuning/behaviour
        bool bLinear;               // linear periods
        bool bLinearpitenv;         // linear slides on envelopes?
        bool bInstruments;          // using instruments?
        bool bNewnoteactions;       // New Note Actions used (set numvoices rather than numchn)
        bool bLinearfreq;           // Tran's frequency-linear mode (instead of periods)
        bool bNopanning;            // dispable 8xx panning effects
        bool bNoextspeed;           // disable PT extended speed
        bool bNoresonance;          // disable 8xx panning effects
        bool bExtsamples;           // uses extended samples?
        //bool

        // ...
        u32        numchannels;     // number of module channels
        u32        numvoices;       // voices allocated to NNA playback
        u64        songlen;         // length of the song as predicted by PredictSongLength (1/1000th of a second).
        u64        filesize;        // size of the module file, in bytes
        u32        memsize;         // number of per-channel effect memory entires (a->memory) to allocate.

        u32        numpositions;    // number of positions in this song
        u32        numpatterns;     // number of patterns in this song
        u32        numtracks;       // number of tracks
        u32        numinstruments;  // number of instruments
        u32        numsamples;      // number of samples
        u32        reppos;          // restart position
        u32        initspeed;       // initial song speed
        u32        inittempo;       // initial song tempo
        u32        initvolume;      // initial global volume (0 - 128)

        int        panning[64];     // 64 initial panning positions
        u32        chanvol[64];     // 64 initial channel volumes
        bool       muted[64];       // 64 muting flags (I really should change this to a struct)

        int        pansep;          // Panning separation (0=mono, 128=full stereo).

        string     songname;        // name of the song
        string     filename;        // name of the file this song came from (optional)
        string     composername;    // name of the composer
        string     comment;         // module comments
        string     modtype;         // string type of module loaded

        // Instruments and Samples!
        vector<Instrument> instruments; // all instruments
        vector<UniSample> samples;      // all samples
        vector<ExtSample> extsamples;   // all extended sample-info (corresponds to each sample)

        // UniTrack related -> Tracks and reading them!
  
        // Big problem here.  I have NO IDEA how this worked.  Sort of got the jist of it, then
        // coded my own implementation.  Which means it's not the same as Air's!  Hope the renderer 
        // can be adapted to cope easily.
        vector<Pattern> patterns;
        vector<u16> positions;      // pattern indeces.  Contains the order in which the patterns are to be played in.


/*        // STL!  oh god, please.  STL.
        u8     **tracks;         // array of numtrk pointers to tracks
        u8     **globtracks;     // array of numpat pointers to global tracks
        u16      *patterns;       // array of Patterns [index to tracks for each channel].
        u16      *pattrows;       // array of number of rows for each pattern*/

        u8       memflag[64];    // flags for each memory slot (alloc'd to mf->memsize)

        //void      (*localeffects)(int effect, INT_MOB dat);
        //void      (*globaleffects)(int effect, INT_MOB dat);

        u64          strip_threshold;  // time, in milliseconds, before shortening song.
        int          sngpos_silence;   // number of positions in this song
        int          patpos_silence;   // number of patterns in this song

        //------------------------------------------------
        // Methods
    };

    u16 FineTune(u16 w);
};