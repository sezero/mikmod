
#ifndef _RESSHARE_H_
#define _RESSHARE_H_

#include <math.h>

#define VC_RESFACTOR   16
#define VC_COFACTOR    16

// =====================================================================================
    static void __inline VC_CalcResonance(VIRTCH *vc, VINFO *vnf)
// =====================================================================================
// I backwards engineered Lim's resonance values, and this is what I came up with:
// cutoff * (freq / 294912)
// resonance^4 / 4194304
{
    // was 138

    //double  cutoff   = (pow(2,(vnf->cutoff-202) / (24.0)) * vc->mixspeed);
    double  cutoff   = pow(2,(vnf->cutoff-202) / (24.0)); // * vc->mixspeed);
    double  resopimp = vnf->resonance ? (pow(vnf->resonance/48,2) + 1.0) : 1;

    double w = (2.0*3.14159*cutoff); ///vc->mixspeed; // Pole angle
    double q = 1.0-w/(2.0*(resopimp+0.5/(1.0+w))+w-2.0); // Pole magnitude
    double r = q*q;
    double c = r + 1.0 - (2.0*cos(w)*q);

    vnf->resfilter.resfactor = r * (1l<<VC_RESFACTOR);
    vnf->resfilter.cofactor  = c * (1l<<VC_COFACTOR);
}


#endif
