/*

 Name:  MLREG.C

 Description:
 A single routine for registering all loaders in MikMod for the current
 platform.

 Portability:
 All systems - all compilers

*/

#include "mikmod.h"
#include "uniform.h"


void MikMod_RegisterAllLoaders(void)
{
   //MikMod_RegisterLoader(load_uni);
   Mikmod_RegisterLoader(load_it);
   Mikmod_RegisterLoader(load_xm);
   Mikmod_RegisterLoader(load_s3m);
   Mikmod_RegisterLoader(load_mod);
   Mikmod_RegisterLoader(load_mtm);
   Mikmod_RegisterLoader(load_stm);
   //MikMod_RegisterLoader(load_dsm);
   //MikMod_RegisterLoader(load_med);
   //MikMod_RegisterLoader(load_far);
   //MikMod_RegisterLoader(load_ult);
   //MikMod_RegisterLoader(load_669);
}
