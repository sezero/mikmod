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
 Module: vc8ss.c  - Stereo Sample Mixer!

  Low-level mixer functions for mixing 8 bit STEREO sample data.  Includes
  normal and interpolated mixing, without declicking (no microramps, see
  vc8ssnc.c for those).

  Note: Stereo Sample Mixing does not support Dolby Surround.  Dolby Surround
  is lame anyway, so get over it!

*/

#include "mikmod.h"
#include "wrap8.h"


typedef SBYTE SCAST;

#include "ssmix.h"


VMIXER S8_MONO_INTERP =
{   NULL,

    "Stereo-8 (Mono/Interp) v0.1",

    nc8ss_Check_MonoInterp,

    NULL,
    NULL,

    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoSSI_NoClick,
    Mix8MonoSSI_NoClick,
    MixMonoSSI,
    MixMonoSSI,
};

VMIXER S8_STEREO_INTERP =
{   NULL,

    "Stereo-8 (Stereo/Interp) v0.1",

    nc8ss_Check_StereoInterp,

    NULL,
    NULL,

    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoSSI_NoClick,
    Mix8StereoSSI_NoClick,
    MixStereoSSI,
    MixStereoSSI,
};

VMIXER S8_MONO =
{   NULL,

    "Stereo-8 (Mono) v0.1",

    nc8ss_Check_Mono,

    NULL,
    NULL,

    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoSS_NoClick,
    Mix8MonoSS_NoClick,
    MixMonoSS,
    MixMonoSS,
};

VMIXER S8_STEREO =
{   NULL,

    "Stereo-8 (Stereo) v0.1",

    nc8ss_Check_Stereo,
    
    NULL,
    NULL,

    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoSS_NoClick,
    Mix8StereoSS_NoClick,
    MixStereoSS,
    MixStereoSS,
};

