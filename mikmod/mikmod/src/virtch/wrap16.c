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
 Module: wrap16.c

  Basic common wrapper functions for all Mikmod-standard 16-bit mixers (both
  C and assembly versions).  Because the 16 bit mixers require no lookup tables,
  this module has no Init/Deinit functions.

  - CalculateVolumes
  - RampVolume
  
*/

#include "mikmod.h"
#include "wrap16.h"

VOLINFO16      v16;
VC_RESFILTER  *r16;

// =====================================================================================
    void VC_Volcalc16_Mono(VIRTCH *vc, VINFO *vnf)
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
}


// =====================================================================================
    void VC_Volcalc16_Stereo(VIRTCH *vc, VINFO *vnf)
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
}


// =====================================================================================
    void VC_Volramp16_Mono(VINFO *vnf, int done)
// =====================================================================================
{
    vnf->oldvol.front.left += (v16.left.inc * done);
}


// =====================================================================================
    void VC_Volramp16_Stereo(VINFO *vnf, int done)
// =====================================================================================
{
    vnf->oldvol.front.left  += (v16.left.inc * done);
    vnf->oldvol.front.right += (v16.right.inc * done);
}

