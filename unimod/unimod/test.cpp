/*
    A simple testbox.  Not part of the API.
*/

#include "unimod.h"
#include "modloader.h"
#include <iostream>

using namespace std;

void main()
{
    MikMod::Loader loader;
    loader.RegisterLoader(new MikMod::MODLoader);
    MikMod::Unimod* pMod=loader.Load("aurora.mod");

    for (vector<u16>::iterator i=pMod->positions.begin(); i!=pMod->positions.end(); i++)
    {
        cout << *i << " ";
    }

    cout << endl;

    delete pMod;
}