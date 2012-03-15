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
 Module: nc8.c

 Description:
  Mix 8 bit *STEREO* (oh yea!) data with volume ramping.  These functions 
  use special global variables that differ fom the ones used by the normal 
  mixers, so that they do not interfere with the true volume of the sound.

  (see v8 extern in wrap8.h)

*/

#include "mikmod.h"
#include "wrap8.h"

BOOL nc8ss_Check_Mono(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_MONO) || (format != SF_STEREO)) return 0;

    return 1;
}


BOOL nc8ss_Check_MonoInterp(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_MONO) || (format != SF_STEREO)) return 0;
    if(mixmode & DMODE_INTERP) return 1;

    return 0;
}


BOOL nc8ss_Check_Stereo(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_STEREO) || (format != SF_STEREO)) return 0;
    return 1;
}


BOOL nc8ss_Check_StereoInterp(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_STEREO) || (format != SF_STEREO)) return 0;
    if(mixmode & DMODE_INTERP) return 1;

    return 0;
}



// =====================================================================================
    void __cdecl Mix8StereoSS_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   v8.left.vol    += v8.left.inc;
        v8.right.vol   += v8.right.inc;
        *dest++ += v8.left.vol  * srce[(himacro(index)*2)];
        *dest++ += v8.right.vol * srce[(himacro(index)*2)+1];
        index  += increment;
    }
}


// =====================================================================================
    void __cdecl Mix8StereoSSI_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
	for(; todo; todo--)
    {   register SLONG  sroot;

        v8.left.vol   += v8.left.inc;
        v8.right.vol  += v8.right.inc;

        sroot = srce[(himacro(index)*2)];
        *dest++  += v8.left.vol  * (SBYTE)(sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        sroot = srce[(himacro(index)*2) + 1];
        *dest++  += v8.right.vol * (SBYTE)(sroot + ((((SLONG)srce[(himacro(index)*2) + 3] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));


        index   += increment;
    }
}


// =====================================================================================
    void __cdecl Mix8MonoSS_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   v8.left.vol     += v8.left.inc;
        *dest++          += (v8.left.vol * (srce[himacro(index)] + srce[(himacro(index)*2)+1])) / 2;
        index   += increment;
    }
}


// =====================================================================================
    void __cdecl Mix8MonoSSI_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG  sroot = srce[himacro(index)*2], crap;
        v8.left.vol += v8.left.inc;
        crap  = (sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        sroot = srce[(himacro(index)*2) + 1];        
        *dest++ += v8.left.vol * (crap + (sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS))) / 2;

        index   += increment;
    }
}
