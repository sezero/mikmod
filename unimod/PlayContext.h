/*
    I think this where the unimod mixer goes. :o
*/

#pragma once

#include "types.h"
#include "ISoundDriver.h"
#include "Unimod.h"

namespace MikMod
{
    // is an interface necessary?
    class PlayContext
    {
        Unimod*                 pMod;
        ISoundDriver*           pDriver;

        vector<ISoundBuffer*>   pSamples;

    public:
        PlayContext(Unimod* mod,ISoundDriver* driver);
        ~PlayContext();

        void Update();
    };
};