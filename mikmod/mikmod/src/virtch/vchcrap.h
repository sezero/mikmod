
#ifndef _VCHCRAP_H_
#define _VCHCRAP_H_

#include "mikmod.h"

// Various other VIRTCH.C Compiler Options
// =======================================

// BITSHIFT : Controls the maximum volume of the sound output.  All data
//      is shifted right by BITSHIFT after being mixed.  Higher values
//      result in quieter sound and less chance of distortion.

#ifndef BITSHIFT
#define BITSHIFT 9
#endif

// BOUNDS_CHECKING : Forces VIRTCH to perform bounds checking.  Commenting
//      the line below will result in a slightly faster mixing process but
//      could cause nasty clicks and pops on some modules.  Disable this
//      option on games or demos only, where speed is very important all
//      songs / sndfx played can be specifically tested for pops.
//
// NOTE: Disabling this has almost no speed benefit when using MMX!

#ifndef NO_BOUNDS_CHECKING
#define BOUNDS_CHECKING
#endif

// Volume factors.  Higher numbers are quieter.
// BIT16_VOLFAC is based on BIT8_VOLFAC to ensure both have perfectly balanced
// volume scales.


#ifndef BIT16_VOLFAC
#define BIT16_VOLFAC 128
#endif

#define BIT8_VOLFAC    2



#define RAMPLEN_VOLUME   (vc->mixspeed / 1024)
#define RAMPLEN_NOTECUT  ((vc->mixspeed / 2048)+12)
#define RAMPLEN_NOTEKICK ((vc->mixspeed / 384)+10)

// ------------------------
//  Fixed Point Math Crap!
// ------------------------

#ifndef FRACBITS
#define FRACBITS 32
#endif

#define FRACMASK (INT64S)((1UL<<FRACBITS)-1UL)

#ifndef INTERPBITS
#define INTERPBITS 16
#endif
#define INTERPMASK (SLONG)((1ul<<INTERPBITS)-1ul)


#ifndef TICKLSIZE
#define TICKLSIZE 8192
#endif
#define TICKWSIZE (TICKLSIZE*2)
#define TICKBSIZE (TICKWSIZE*2)

// Macros used by the mixers to get the hi-part and low (fractional) part
// of the index passed to them.

#define himacro(x) ((SLONG)(x >> FRACBITS))
#define lomacro(x) ((ULONG)(x & FRACMASK))

#endif


