
#pragma once

#include "types.h"
#include "note.h"

namespace MikMod
{
    struct Pattern
    {
        struct Track
        {
            vector<Note> notes;
        };

    public:
        vector<Track> channel;
    };
};