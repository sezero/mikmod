
#pragma once

#include "types.h"
#include <ios>
#include <map>

namespace MikMod
{
    // proto
    class Unimod;

    // Interface for module loaders
    class ILoader
    {
    public:
        virtual const char* GetExtension()=0;

        virtual Unimod* Load(std::istream& stream)=0;
        Unimod* Load(const string& fname);                  // convenience method for skipping the Loader class.
    };

    // Manages ILoader instances and presents a unified interface for loading modules.
    // Maybe this should be a singleton or something.
    class Loader
    {
        std::map<string,ILoader*> loaders;
    public:

        ~Loader();

        void Loader::RegisterLoader(ILoader* newloader);

        Unimod* Load(const string& fname);
    };
};