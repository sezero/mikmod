
#include "types.h"
#include "loader.h"
#include "unimod.h"
#include <fstream>

using namespace MikMod;

Unimod* ILoader::Load(const string& fname)
{
    std::ifstream fstream(fname.c_str(),std::ios::binary);

    return Load(fstream);
}

Loader::~Loader()
{
    for (std::map<string,ILoader*>::iterator i=loaders.begin(); i!=loaders.end(); i++)
    {
        delete i->second;
    }
}

void Loader::RegisterLoader(ILoader* newloader)
{
    string extension=newloader->GetExtension();

    if (loaders[extension]!=0)
        delete loaders[extension];

    loaders[extension]=newloader;
}

Unimod* Loader::Load(const string& fname)
{
    string s=fname;
    for (int i=0; i<s.length(); i++)
        if (s[i]>='a' && s[i]<='z')
            s[i]&=~32;

    int pos=s.rfind(".");
    if (pos==string::npos)
        return 0;

    string sExtension=s.substr(pos+1);

    std::ifstream fstream(fname.c_str(),std::ios::binary);

    if (loaders[sExtension])
        return loaders[sExtension]->Load(fstream);

    return 0;
}
