
#pragma once

#include <bitset>

namespace MikMod
{
    struct Sample
    {
        enum SampleFormat
        {
            sf_16bit=1,
            sf_stereo,
            sf_signed,
            sf_delta,
            sf_bigendian
        };

        enum SampleLoop
        {
            sl_loop=1,
            sl_bidi,
            sl_reverse,
            sl_sustain_loop,
            sl_sustain_bidi,
            sl_sustain_reverse,
            sl_declick,
        };

        string  sName;

        u16*    pSampledata;
        u32     length;
        u32     speed;
        int     volume;
        u32     loopstart;
        u32     loopend;

        std::bitset<16> format;
        std::bitset<16> loopflags;

        Sample() : pSampledata(0), length(0), speed(0), volume(0), loopstart(0), loopend(0)
        {}
    };
};