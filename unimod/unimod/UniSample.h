
#pragma once

#include "types.h"
#include <bitset>

namespace MikMod
{

    struct UniSample
    {
        enum LoopFlags
        {
            sl_loop,
            sl_bidirectional,
            sl_reverse,
            sl_sustain_loop,
            sl_sustain_bidirectional,
            sl_sustain_reverse,
            sl_declick,

            sl_resonance_filter,
        };

        enum FormatFlags
        {
            sf_16bit,
            sf_stereo,
            sf_signed,
            sf_delta,
            sf_big_endian,
        };

        // Mikmod's custom module player sample structure, used in the place of the MD_SAMPLE,
        // which I normally use in applications for sound effects.  Reason: this has a lot of
        // extra little tidbits which would be otherwise worthless in a sound effects situaiton.
        std::bitset<32> flags;  // looping and player flags!
        int    handle;          // handle to the sample loaded into the driver

        u32    speed;           // default playback speed (almost anything is legal)
        int    volume;          // 0 to 128 volume range
        int    panning;         // PAN_LEFT to PAN_RIGHT range

        u32    length;          // Length of the sample (samples, not bytes!)
        u32    loopstart;       // Sample looping smackdown!
        u32    loopend;
        u32    susbegin;        // sustain loop begin (in samples)
        u32    susend;          // sustain loop end

        int    cutoff;          // the cutoff frequency (range 0 to 16384)
        int    resonance;       // the resonance factor (range -128 to 128)

        string sName;           // name of the sample    

        bool   used;

        // Sample Loading information - relay info between the loaders and
        // unimod.c - like wow dude!

        std::bitset<32> format; // diskformat flags, describe sample prior to loading!
        u32    compress;        // compression status of the sample.
        u64    seekpos;         // seekpos within the module.

        u16*     pSampledata;

        UniSample() : pSampledata(0) {}
        ~UniSample() { delete[] pSampledata; }

    };

    class ExtSample { /* TODO... or even better... rethink */ };

};