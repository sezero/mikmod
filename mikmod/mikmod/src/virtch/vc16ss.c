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
 Module: vc16ss.c - 16bit STEREO sample mixers!

 Description:
  Stereo, stereo.  It comes from all around.  Stereo, stereo, the multi-
  pronged two-headed attack of sound!  Stereo, Stereo, it owns you-hu!
  Stereo, Stereo, It rings soooooo... oh sooooooo... truuuuuuee!

*/

#include "mikmod.h"
#include "wrap16.h"

typedef SWORD SCAST;

#include "ssmix.h"


VMIXER S16_MONO_INTERP =
{   NULL,

    "Stereo-16 (Mono/Interp) v0.1",

    nc16ss_Check_MonoInterp,

    NULL,
    NULL,

    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoSSI_NoClick,
    Mix16MonoSSI_NoClick,
    MixMonoSSI,
    MixMonoSSI,
};

    
VMIXER S16_STEREO_INTERP =
{   NULL,

    "Stereo-16 (Stereo/Interp) v0.1",

    nc16ss_Check_StereoInterp,

    NULL,
    NULL,

    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoSSI_NoClick,
    Mix16StereoSSI_NoClick,
    MixStereoSSI,
    MixStereoSSI,
};

VMIXER S16_MONO =
{   NULL,

    "Stereo-16 (Mono) v0.1",

    nc16ss_Check_Mono,

    NULL,
    NULL,

    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoSS_NoClick,
    Mix16MonoSS_NoClick,
    MixMonoSS,
    MixMonoSS,
};

VMIXER S16_STEREO =
{   NULL,

    "Stereo-16 (Stereo) v0.1",

    nc16ss_Check_Stereo,

    NULL,
    NULL,

    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoSS_NoClick,
    Mix16StereoSS_NoClick,
    MixStereoSS,
    MixStereoSS,
};

