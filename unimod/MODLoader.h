
#pragma once

#include <iostream>
#include "note.h"
#include "fileio.h"

namespace
{
    struct ModNote;
};

namespace MikMod
{
    class Unimod;

    class MODLoader
    {
    private:
        Note ConvertNote(const ModNote& n);

        void LoadPatterns(BinaryStream& stream,Unimod* mod);
        void LoadSamples(BinaryStream& stream,Unimod* mod);
    public:
        Unimod* Load(std::istream& _stream);
    };
};