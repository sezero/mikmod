
#pragma once

#include "types.h"
#include "note.h"
#include "instrument.h"
#include "pattern.h"
#include "sample.h"

namespace MikMod
{

    // Represents a MOD file, of any type.
    class Unimod
    {
    public:
        // This section of elements are all file-storage related.
        // all of this information can be found in the UNIMOD disk format.
        // For further details about there variables, see the MikMod Docs.

        u32         flags;          // See UniMod Flags above
        u32         numchannels;      // number of module channels
        u32         numvoices;        // voices allocated to NNA playback
        u64         nLength;        // length of the song as predicted by PredictSongLength (1/1000th of a second).
        u64         nFilesize;      // size of the module file, in bytes
        u32         nMemsize;       // number of per-channel effect memory entires (a->memory) to allocate.

        u32         numpositions;     // number of positions in this song
        u32         numpatterns;      // number of patterns in this song
        u32         numtracks;        // number of tracks
        u32         numinstruments;   // number of instruments
        u32         numsamples;       // number of samples
        int         reppos;         // restart position
        u32         initspeed;      // initial song speed
        u32         inittempo;      // initial song tempo
        u32         initvolume;     // initial global volume (0 - 128)

        int         panning[64];    // 64 initial panning positions
        u32         chanvol[64];    // 64 initial channel volumes
        bool        muted[64];      // 64 muting flags (I really should change this to a struct)

        int		    pansep;		    // Panning separation (0=mono, 128=full stereo).

        string      songname;       // name of the song
        string      filename;       // name of the file this song came from (optional)
        string      composer;       // name of the composer
        string      comment;        // module comments
        string      modtype;        // string type of module loaded

        //                              Instruments and Samples!
        
        vector<Instrument>      instruments;    // all instruments
        vector<Sample>          samples;        // all samples
        //vector<ExtSample>       extsamples;     // all extended sample-info (corresponds to each sample)

        //                      UniTrack related -> Tracks and reading them!
        
        // Big problem here.  I have NO IDEA what Air was doing here.  Sort of got the jist of it, then
        // coded my own implementation.  Which means it's not the same as Air's!  Hope the renderer 
        // can be adapted to cope easily.
        vector<Pattern> patterns;       // array of Patterns [index to tracks for each channel].
        vector<u16> positions;      // all positions

        u8          memflag[64];    // flags for each memory slot (alloc'd to mf->memsize)

        u64         strip_threshold;  // time, in milliseconds, before shortening song.
        int          sngpos_silence;   // number of positions in this song
        int          patpos_silence;   // number of patterns in this song
    };

};