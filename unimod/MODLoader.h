
#pragma once

#include "types.h"
#include "unimod.h"
#include "fileio.h"
#include "loader.h"
#include <ios>

namespace
{
    struct ModNote;
};

namespace MikMod
{
    class MODLoader : public ILoader
    {
        static Note ConvertNote(const struct ModNote& n);
        static void LoadPatterns(BinaryStream& stream,Unimod* mod);
        static void LoadSamples(BinaryStream& stream,Unimod* mod);
    public:
        virtual const char* GetExtension() { return "MOD"; }

        virtual Unimod* Load(std::istream& stream);
    };
};