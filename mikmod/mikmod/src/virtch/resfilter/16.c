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
 Module: resfilter/16.c

 Description:
  Low-level mixer functions for mixing 16 bit sample data.  Includes normal and
  interpolated mixing, without declicking (no microramps, see vc16nc.c
  for those).

*/

#include "mikmod.h"
#include "../wrap16.h"

#include "resshare.h"

// =====================================================================================
    static int __inline filter(SWORD sroot, VC_RESFILTER *r16)
// =====================================================================================
{
    
    r16->speed += (((sroot<<10) - r16->pos) * r16->cofactor) >> VC_COFACTOR;
    r16->pos   += r16->speed;

    r16->speed *= r16->resfactor;
    r16->speed >>= VC_RESFACTOR;

    return r16->pos>>10;
}


// =====================================================================================
    static void VC_ResVolcalc16_Mono(VIRTCH *vc, VINFO *vnf)
// =====================================================================================
{
    vnf->vol.front.left  = ((vnf->volume.front.left+vnf->volume.front.right) * (vc->volume.front.left+vc->volume.front.right)) / 3;

    lvolsel = vnf->vol.front.left / BIT16_VOLFAC;

    // declicker: Set us up to volramp!
    if(vc->mode & DMODE_NOCLICK)
    {   if(vnf->vol.front.left != vnf->oldvol.front.left)
        {   if(!vnf->volramp) vnf->volramp = RAMPLEN_VOLUME;
            v16.left.inc = v16.right.inc = ((vnf->vol.front.left - (v16.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
        } else if(vnf->volramp)
        {   v16.left.inc  = ((vnf->vol.front.left - (v16.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            if(!v16.left.inc) vnf->volramp = 0;
        }
    }

    VC_CalcResonance(vc, vnf);
    r16 = &vnf->resfilter;
}


// =====================================================================================
    static void VC_ResVolcalc16_Stereo(VIRTCH *vc, VINFO *vnf)
// =====================================================================================
{
    vnf->vol.front.left  = vnf->volume.front.left  * vc->volume.front.left;
    vnf->vol.front.right = vnf->volume.front.right * vc->volume.front.right;

    lvolsel = vnf->vol.front.left / BIT16_VOLFAC;
    rvolsel = vnf->vol.front.right / BIT16_VOLFAC;

    // declicker: Set us up to volramp!
    if(vc->mode & DMODE_NOCLICK)
    {   if((vnf->vol.front.left != vnf->oldvol.front.left) || (vnf->vol.front.right != vnf->oldvol.front.right))
        {   if(!vnf->volramp) vnf->volramp = RAMPLEN_VOLUME;
            v16.left.inc  = ((vnf->vol.front.left - (v16.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            v16.right.inc = ((vnf->vol.front.right - (v16.right.vol = vnf->oldvol.front.right)) / (int)vnf->volramp);
        } else if(vnf->volramp)
        {   v16.left.inc  = ((vnf->vol.front.left - (v16.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            v16.right.inc = ((vnf->vol.front.right - (v16.right.vol = vnf->oldvol.front.right)) / (int)vnf->volramp);
            if(!v16.left.inc && !v16.right.inc) vnf->volramp = 0;
        }
    }

    VC_CalcResonance(vc, vnf);
    r16 = &vnf->resfilter;
}


// =====================================================================================
    void __cdecl Res16StereoNormal(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    SWORD  sample;

    for(; todo; todo--)
    {
        sample = filter(srce[himacro(index)], r16);
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ += rvolsel * sample;
    }
}


// =====================================================================================
    void __cdecl Res16StereoInterp(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG  sroot  = srce[himacro(index)];
        sroot = filter((SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r16);

        *dest++ += lvolsel * sroot;
        *dest++ += rvolsel * sroot;

        index  += increment;
    }
}


// =====================================================================================
    void __cdecl Res16SurroundNormal(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    SLONG  sample;

    for (; todo; todo--)
    {   sample = lvolsel * filter(srce[himacro(index)], r16);
        index += increment;

        *dest++ += sample;
        *dest++ -= sample;
    }
}


// =====================================================================================
    void __cdecl Res16SurroundInterp(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for (; todo; todo--)
    {   register SLONG  sroot = srce[himacro(index)];
        sroot = lvolsel * filter((SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r16);

        *dest++ += sroot;
        *dest++ -= sroot;
        index   += increment;
    }
}


// =====================================================================================
    void __cdecl Res16MonoNormal(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   *dest++ += lvolsel * filter(srce[himacro(index)], r16);
        index  += increment;
    }
}


// =====================================================================================
    void __cdecl Res16MonoInterp(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG  sroot = srce[himacro(index)];
        sroot = lvolsel * filter((SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r16);
        *dest++ += sroot;
        index  += increment;
    }
}


// =====================================================================================
    void __cdecl Res16StereoNormal_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {
        v16.left.vol    += v16.left.inc;
        v16.right.vol   += v16.right.inc;
        {
        register SLONG sample = filter(srce[himacro(index)], r16);
        *dest++ += (v16.left.vol  / BIT16_VOLFAC) * sample;
        *dest++ += (v16.right.vol / BIT16_VOLFAC) * sample;
        }
        index  += increment;
    }
}


// =====================================================================================
    void __cdecl Res16StereoInterp_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG  sroot  = srce[himacro(index)];

        v16.left.vol    += v16.left.inc;
        v16.right.vol   += v16.right.inc;

        sroot = filter((SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r16);

        *dest++  += (v16.left.vol  / BIT16_VOLFAC) * sroot;
        *dest++  += (v16.right.vol / BIT16_VOLFAC) * sroot;
        index    += increment;
    }
}


// =====================================================================================
    void __cdecl Res16SurroundNormal_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG sample;

        v16.left.vol += v16.left.inc;
        sample        = (v16.left.vol / BIT16_VOLFAC) * filter(srce[himacro(index)], r16);

        *dest++ += sample;
        *dest++ -= sample;
        index   += increment;
    }
}


// =====================================================================================
    void __cdecl Res16SurroundInterp_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG  sroot = srce[himacro(index)];
        v16.left.vol += v16.left.inc;
        sroot         = (v16.left.vol  / BIT16_VOLFAC) * filter((SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r16);

        *dest++ += sroot;
        *dest++ -= sroot;
        index   += increment;
    }
}


// =====================================================================================
    void __cdecl Res16MonoNormal_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    SLONG  sample;

    for(; todo; todo--)
    {   v16.left.vol  += v16.left.inc;
        sample         = (v16.left.vol  / BIT16_VOLFAC) * filter(srce[himacro(index)], r16);

        *dest++ += sample;
        index   += increment;
    }
}


// =====================================================================================
    void __cdecl Res16MonoInterp_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
// =====================================================================================
{
    for(; todo; todo--)
    {   register SLONG  sroot = srce[himacro(index)];
        v16.left.vol += v16.left.inc;
        sroot         = (v16.left.vol  / BIT16_VOLFAC) * filter((SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r16);

        *dest++ += sroot;
        index   += increment;
    }
}


static BOOL Check_Mono(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_MONO) || (format != SF_16BITS)) return 0;
    if(flags & SL_RESONANCE_FILTER) return 1;

    return 0;
}


static BOOL Check_MonoInterp(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_MONO) || (format != SF_16BITS)) return 0;
    if((flags & SL_RESONANCE_FILTER) && (mixmode & DMODE_INTERP)) return 1;

    return 0;
}


static BOOL Check_Stereo(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_STEREO) || (format != SF_16BITS)) return 0;
    if(flags & SL_RESONANCE_FILTER) return 1;

    return 0;
}


static BOOL Check_StereoInterp(uint channels, uint mixmode, uint format, uint flags)
{
    if((channels != MD_STEREO) || (format != SF_16BITS)) return 0;
    if((flags & SL_RESONANCE_FILTER) && (mixmode & DMODE_INTERP)) return 1;

    return 0;
}


VMIXER RF_M16_MONO_INTERP =
{   NULL,

    "Resonance Mono-16 (Mono/Interp) v0.1",

    Check_MonoInterp,

    NULL,
    NULL,

    VC_ResVolcalc16_Mono,
    VC_Volramp16_Mono,
    Res16MonoInterp_NoClick,
    Res16MonoInterp_NoClick,
    Res16MonoInterp,
    Res16MonoInterp,
};

VMIXER RF_M16_STEREO_INTERP =
{   NULL,

    "Resonance Mono-16 (Stereo/Interp) v0.1",

    Check_StereoInterp,

    NULL,
    NULL,

    VC_ResVolcalc16_Stereo,
    VC_Volramp16_Stereo,
    Res16StereoInterp_NoClick,
    Res16SurroundInterp_NoClick,
    Res16StereoInterp,
    Res16SurroundInterp,
};


VMIXER RF_M16_MONO =
{   NULL,

    "Resonance Mono-16 (Mono) v0.1",

    Check_Mono,
    
    NULL,
    NULL,

    VC_ResVolcalc16_Mono,
    VC_Volramp16_Mono,
    Res16MonoNormal_NoClick,
    Res16MonoNormal_NoClick,
    Res16MonoNormal,
    Res16MonoNormal,
};


VMIXER RF_M16_STEREO =
{   NULL,

    "Resonance Mono-16 (Stereo) v0.1",

    Check_Stereo,
    
    NULL,
    NULL,
    
    VC_ResVolcalc16_Stereo,
    VC_Volramp16_Stereo,
    Res16StereoNormal_NoClick,
    Res16SurroundNormal_NoClick,
    Res16StereoNormal,
    Res16SurroundNormal,
};
