/*
 MikMod Sound System

  By Jake Stine of Divine Entertainment (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Module: sn16.c

  Assembly Mixer Plugin : Stereo 16 bit Sample data
  
  Generic all-purpose assembly wrapper for mikmod VMIXER plugin thingie.
  As long as your platform-dependant asm code uses the naming conventions
  below (Asm StereoInterp, SurroundInterp, etc), then you can use this
  module and the others related to it to register your mixer into mikmod.

  See Also:

    mi8.c
    mn8.c
    sn8.c
    si8.c
    mn16.c
    sn16.c
    mi16.c
    si16.c
*/

#include "mikmod.h"
#include "..\wrap16.h"
#include "asmapi.h"

VMIXER ASM_S16_MONO =
{   NULL,

    "Assembly Stereo-16 (Mono) v0.1",

    nc16ss_Check_Mono,

    NULL,
    NULL,
    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoSS_NoClick,
    Mix16MonoSS_NoClick,
    Asm16MonoSS,
    Asm16MonoSS,
};

VMIXER ASM_S16_STEREO =
{   NULL,

    "Assembly Stereo-16 (Stereo) v0.1",

    nc16ss_Check_Stereo,

    NULL,
    NULL,
    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoSS_NoClick,
    Mix16StereoSS_NoClick,
    Asm16StereoSS,
    Asm16StereoSS,
};
