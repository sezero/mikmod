/*

 MikMod Sound System

  By Jake Stine of Divine Entertainment (2000)

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Module: placebo.c

 This is the placebo mixer for when mikmod cannot seem to find a valid mixer
 to match a sample's format and flags and stuff.  You do not have to register
 this mixer.  Mikmod will use it automatically.

}
*/

#include "vchcrap.h"

static void vc_placebo_volcalc(VIRTCH *vc, VINFO *vnf) {};
static void vc_placebo_volramp(VINFO *vnf, int done) {};
static void __cdecl vc_placebo_mix(void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo) {};

VMIXER VC_MIXER_PLACEBO =
{   NULL,

    "Placebo Mixer v1.0",

    NULL,
    NULL,
    NULL,
    
    vc_placebo_volcalc,
    vc_placebo_volramp,

    vc_placebo_mix,
    vc_placebo_mix,
    vc_placebo_mix,
    vc_placebo_mix,
};


