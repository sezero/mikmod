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
 Module: vc8.c

  Low-level mixer functions for mixing 8 bit sample data.  Includes normal and
  interpolated mixing, without declicking (no microramps, see vcMix_noclick.c
  for those).

*/

#include "mikmod.h"
#include "wrap8.h"

typedef SBYTE SCAST;

#include "stdmix.h"

VMIXER M8_MONO_INTERP =
{   NULL,

    "Mono-8 (Mono/Interp) v0.1",

    nc8_Check_MonoInterp,

    NULL,
    NULL,

    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoInterp_NoClick,
    Mix8MonoInterp_NoClick,
    MixMonoInterp,
    MixMonoInterp,
};

VMIXER M8_STEREO_INTERP =
{   NULL,

    "Mono-8 (Stereo/Interp) v0.1",

    nc8_Check_StereoInterp,

    NULL,
    NULL,

    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoInterp_NoClick,
    Mix8SurroundInterp_NoClick,
    MixStereoInterp,
    MixSurroundInterp,
};

VMIXER M8_MONO =
{   NULL,

    "Mono-8 (Mono) v0.1",

    nc8_Check_Mono,

    NULL,
    NULL,

    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoNormal_NoClick,
    Mix8MonoNormal_NoClick,
    MixMonoNormal,
    MixMonoNormal,
};

VMIXER M8_STEREO =
{   NULL,

    "Mono-8 (Stereo) v0.1",

    nc8_Check_Stereo,

    NULL,
    NULL,

    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoNormal_NoClick,
    Mix8SurroundNormal_NoClick,
    MixStereoNormal,
    MixSurroundNormal,
};
