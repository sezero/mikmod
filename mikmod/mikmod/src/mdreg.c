

/*

 Name:  MDREG.C

 Description:
 A single routine for registering all drivers in Mikmod for the current
 platform.

 Portability:
 DOS, WIN95, OS2, SunOS, Solaris,
 Linux, HPUX, AIX, SGI, Alpha

 Anything not listed above is assumed to not be supported by this procedure!

 All Others: n

 - all compilers!

*/

#include "mikmod.h"

void Mikmod_RegisterAllDrivers(void)
{
#ifdef MIKMOD_SDL
    Mikmod_RegisterDriver(drv_sdl);
#elif defined(SUN)
    Mikmod_RegisterDriver(drv_sun);
#elif defined(SOLARIS)
    Mikmod_RegisterDriver(drv_sun);
#elif defined(__alpha)
    Mikmod_RegisterDriver(drv_AF);
#elif defined(OSS)
    Mikmod_RegisterDriver(drv_oss);
    #ifdef ULTRA
       Mikmod_RegisterDriver(drv_ultra);
    #endif
#elif defined(__hpux)
    Mikmod_RegisterDriver(drv_hp);
#elif defined(AIX)
    Mikmod_RegisterDriver(drv_aix);
#elif defined(SGI)
    Mikmod_RegisterDriver(drv_sgi);
#elif defined(__OS2__)
    Mikmod_RegisterDriver(drv_os2);
#elif defined(__NT__)
    Mikmod_RegisterDriver(drv_ds);
    //Mikmod_RegisterDriver(drv_win);
#elif defined(__WIN32__)
    Mikmod_RegisterDriver(drv_ds);
    //Mikmod_RegisterDriver(drv_win);
#elif defined(WIN32)
    Mikmod_RegisterDriver(drv_ds);
    //Mikmod_RegisterDriver(drv_win);
#else
//    Mikmod_RegisterDriver(drv_awe);
    Mikmod_RegisterDriver(drv_gus);    // supports both hardware and software mixing - needed for games.
    //Mikmod_RegisterDriver(drv_gus2);     // use for hardware mixing only (smaller / faster)
    Mikmod_RegisterDriver(drv_pas);
//    Mikmod_RegisterDriver(drv_wss);
    Mikmod_RegisterDriver(drv_ss);
    Mikmod_RegisterDriver(drv_sb16);
    Mikmod_RegisterDriver(drv_sbpro);
    Mikmod_RegisterDriver(drv_sb);
#endif

}


