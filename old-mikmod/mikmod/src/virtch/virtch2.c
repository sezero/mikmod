/*

Name:  VIRTCH2.C

Description:
 All-c sample mixing routines, using a 32 bits mixing buffer, interpolation,
 and sample smoothing [improves sound quality and removes clicks].

 Future Additions:
   Low-Pass filter to remove annoying staticy buzz.


C Mixer Portability:
 All Systems -- All compilers.

Assembly Mixer Portability:

 MSDOS:  BC(?)   Watcom(y)   DJGPP(y)
 Win95:  ?
 Os2:    ?
 Linux:  y

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include <stddef.h>
#include <string.h>
#include "mikmod.h"

// USE_64BIT_INTS : Uncomment this line only if you have a compiler /
//      platform that supports 64 bit integers, and have the SLONGLONG
//      and ULONGLONG typedefs proper in TDEFS.H.  Otherwise, floating
//      point math will be used.

#define USE_64BIT_INTS


// Constant Definitions
// ======================================================================

// MAXVOL_FACTOR : Controls the maximum volume of the output data.
//      All mixed data is divided by this number after mixing, so
//      larger numbers result in quieter mixing.  Smaller numbers
//      will increase the likliness of distortion on loud modules.

// REVERBERATION : Larger numbers result in shorter reverb duration.
//      Longer reverb durations can cause unwanted static and make the
//      reverb sound more like a crappy echo.

// SAMPLING_SHIFT : Specified the shift multiplier which controls by how
//      much the mixing rate is multiplied while mixing.  Higher values
//      can improve quality by smoothing the sound and reducing pops and
//      clicks.  Note, this is a shift value, so a value of 2 becomes a
//      mixing-rate multiplier of 4, and a value of 3 = 8, etc.

// FRACBITS : the number of bits per integer devoted to the fractional
//      part of the number.  Generally, this number should not be changed
//      for any reason.


// IMPORTANT:  All values below MUST ALWAYS be greater than 0!

const unsigned int   REVERBERATION    = 11000;
const int            MAXVOL_FACTOR    = 512;
const unsigned int   SAMPLING_SHIFT   = 2;
const unsigned int   FRACBITS         = 28;
const unsigned int   TICKLSIZE        = 8192;
const unsigned int   CLICK_SHIFT_BASE = 6;

// Do not modify the following macros / constants

#define SAMPLING_FACTOR  (1l << SAMPLING_SHIFT)
#define FRACMASK         ((1UL << FRACBITS) - 1UL)
#define TICKWSIZE        (TICKLSIZE * 2)
#define TICKBSIZE        (TICKWSIZE * 2)
#define CLICK_SHIFT      (CLICK_SHIFT_BASE * SAMPLING_SHIFT)
#define CLICK_BUFFER     (1L << CLICK_SHIFT)


#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifdef USE_64BIT_INTS
typedef SLONGLONG FRACTYPE;
#else
#include <math.h>
typedef double FRACTYPE;
#endif



typedef struct
{   UBYTE     kick;                  // =1 -> sample has to be restarted
    UBYTE     active;                // =1 -> sample is playing
    UWORD     flags;                 // 16/8 bits looping/one-shot
    SWORD     handle;                // identifies the sample
    ULONG     start;                 // start index
    ULONG     size;                  // samplesize
    ULONG     reppos;                // loop start
    ULONG     repend;                // loop end
    ULONG     frq;                   // current frequency
    int       vol;                   // current volume
    int       pan;                   // current panning position

    int       click, rampvol;
    SLONG     lastvalL, lastvalR;
    int       lvolsel, rvolsel,   // Volume factor .. range 0-255
              oldlvol, oldrvol;

    FRACTYPE  current;               // current index in the sample
    FRACTYPE  increment;             // increment value
} VINFO;


static SWORD    **Samples;

static VINFO    *vinf = NULL;
static VINFO    *vnf;
static long     samplesthatfit, TICKLEFT;
static long     vc_memory = 0;
static int      vc_softchn;
static FRACTYPE idxsize,idxlpos,idxlend;
static SLONG    *VC2_TICKBUF = NULL;
static UWORD    vc_mode;


// Reverb control variables
// ========================

static int     RVc1, RVc2, RVc3, RVc4, RVc5, RVc6, RVc7, RVc8;
static ULONG   RVRindex;

// For Mono or Left Channel
static SLONG  *RVbuf1 = NULL, *RVbuf2 = NULL, *RVbuf3 = NULL,
              *RVbuf4 = NULL, *RVbuf5 = NULL, *RVbuf6 = NULL,
              *RVbuf7 = NULL, *RVbuf8 = NULL;

// For Stereo only (Right Channel)
static SLONG  *RVbuf9 = NULL, *RVbuf10 = NULL, *RVbuf11 = NULL,
              *RVbuf12 = NULL, *RVbuf13 = NULL, *RVbuf14 = NULL,
              *RVbuf15 = NULL, *RVbuf16 = NULL;


#ifndef USE_64BIT_INTS

// ==============================================================
//  Floating point versions!!

static double MixStereoNormal(SWORD *srce, SLONG *dest, double index, double increment, ULONG todo)
{
    SWORD   sample;
    double  i, f;

    for(; todo; todo--)
    {   f = modf(index,&i);
        sample = (srce[(SLONG)i] * (1.0-f)) + (srce[(SLONG)i+1] * f);
        index += increment;

        if(vnf->rampvol)
        {   *dest++ += (long)(((((double)vnf->oldlvol * vnf->rampvol) + (vnf->lvolsel * (CLICK_BUFFER-vnf->rampvol))) * (double)sample) / (1<<CLICK_SHIFT));
            *dest++ += (long)(((((double)vnf->oldrvol * vnf->rampvol) + (vnf->rvolsel * (CLICK_BUFFER-vnf->rampvol))) * (double)sample) / (1<<CLICK_SHIFT));
            vnf->rampvol--;
        } else if(vnf->click)
        {   *dest++ += (long)((((double)(vnf->lvolsel * (CLICK_BUFFER-vnf->click)) * (double)sample) + (vnf->lastvalL * vnf->click)) / (1<<CLICK_SHIFT));
            *dest++ += (long)(((((double)vnf->rvolsel * (CLICK_BUFFER-vnf->click)) * (double)sample) + (vnf->lastvalR * vnf->click)) / (1<<CLICK_SHIFT));
            vnf->click--;
        } else
        {   *dest++ += vnf->lvolsel * sample;
            *dest++ += vnf->rvolsel * sample;
        }
    }

    vnf->lastvalL = vnf->lvolsel * sample;
    vnf->lastvalR = vnf->rvolsel * sample;

    return index;
}


static double MixStereoSurround(SWORD *srce, SLONG *dest, double index, double increment, ULONG todo)
{
    SWORD   sample;
    long    whoop;
    double  i, f;

    for(dest--; todo; todo--)
    {   f = modf(index,&i);
        sample = (srce[(SLONG)i] * (1.0-f)) + (srce[(SLONG)i+1] * f);
        index += increment;

        if(vnf->rampvol)
        {   whoop = (long)((((double)(vnf->oldlvol * vnf->rampvol) + (vnf->lvolsel * (CLICK_BUFFER-vnf->rampvol))) * (double)sample) / (1<<CLICK_SHIFT));
            *dest++ += whoop;
            *dest++ -= whoop;
            vnf->rampvol--;
        } else if(vnf->click)
        {   whoop = (long)(((((double)vnf->lvolsel * (CLICK_BUFFER-vnf->click)) * (double)sample) + (vnf->lastvalL * vnf->click)) / (1<<CLICK_SHIFT));
            *dest++ += whoop;
            *dest++ -= whoop;
            vnf->click--;
        } else
        {   *dest++ += vnf->lvolsel * sample;
            *dest++ -= vnf->rvolsel * sample;
        }
    }
    
    vnf->lastvalL = vnf->lvolsel * sample;
    vnf->lastvalR = vnf->rvolsel * sample;

    return index;
}


static double MixMonoNormal(SWORD *srce, SLONG *dest, double index, double increment, SLONG todo)
{
    SWORD   sample;
    double  i, f;

    for(; todo; todo--)
    {   f = modf(index,&i);
        sample = (srce[(SLONG)i] * (1.0-f)) + (srce[(SLONG)i+1] * f);
        index += increment;
    
        if(vnf->rampvol)
        {   *dest++ += (long)((((double)(vnf->oldlvol * vnf->rampvol) + (vnf->lvolsel * (CLICK_BUFFER-vnf->rampvol))) * (double)sample) / (1<<CLICK_SHIFT));
            vnf->rampvol--;
        } else if(vnf->click)
        {   *dest++ += (long)(((((double)vnf->lvolsel * (CLICK_BUFFER-vnf->click)) * (double)sample) + (vnf->lastvalL * vnf->click)) / (1<<CLICK_SHIFT));
            vnf->click--;
        } else
            *dest++ += vnf->lvolsel * sample;
    }
    
    vnf->lastvalL = vnf->lvolsel * sample;

    return index;
}

#else

// =============================
//  64 bit integer versions!

static SLONGLONG MixStereoNormal(SWORD *srce, SLONG *dest, SLONGLONG index, SLONGLONG increment, ULONG todo)
{
    SWORD  sample;

    for(; todo; todo--)
    {   
        sample = (SWORD)(((SLONGLONG)srce[index >> FRACBITS] * ((FRACMASK+1L) - (index & FRACMASK))) +
                 ((SLONGLONG)srce[(index >> FRACBITS)+1] * (index & FRACMASK)) >> FRACBITS);
        index += increment;
    
        if(vnf->rampvol)
        {   *dest++ += (((SLONGLONG)(vnf->oldlvol * vnf->rampvol) + (vnf->lvolsel * (CLICK_BUFFER-vnf->rampvol))) * (SLONGLONG)sample) >> CLICK_SHIFT;
            *dest++ += (((SLONGLONG)(vnf->oldrvol * vnf->rampvol) + (vnf->rvolsel * (CLICK_BUFFER-vnf->rampvol))) * (SLONGLONG)sample) >> CLICK_SHIFT;
            vnf->rampvol--;
        } else if(vnf->click)
        {   *dest++ += ((((SLONGLONG)vnf->lvolsel * (CLICK_BUFFER-vnf->click)) * (SLONGLONG)sample) + ((SLONGLONG)vnf->lastvalL * vnf->click)) >> CLICK_SHIFT;
            *dest++ += ((((SLONGLONG)vnf->rvolsel * (CLICK_BUFFER-vnf->click)) * (SLONGLONG)sample) + ((SLONGLONG)vnf->lastvalR * vnf->click)) >> CLICK_SHIFT;
            vnf->click--;
        } else
        {   *dest++ += vnf->lvolsel * sample;
            *dest++ += vnf->rvolsel * sample;
        }
    }

    vnf->lastvalL = vnf->lvolsel * sample;
    vnf->lastvalR = vnf->rvolsel * sample;

    return index;
}


static SLONGLONG MixStereoSurround(SWORD *srce, SLONG *dest, SLONGLONG index, SLONGLONG increment, ULONG todo)
{
    SWORD  sample;
    long   whoop;

    for(; todo; todo--)
    {   sample = (SWORD)(((SLONGLONG)srce[index >> FRACBITS] * ((FRACMASK+1L) - (index & FRACMASK))) +
                 ((SLONGLONG)srce[(index >> FRACBITS)+1] * (index & FRACMASK)) >> FRACBITS);
        index += increment;

        if(vnf->rampvol)
        {   whoop = (((SLONGLONG)(vnf->oldlvol * vnf->rampvol) + (vnf->lvolsel * (CLICK_BUFFER-vnf->rampvol))) * (SLONGLONG)sample) >> CLICK_SHIFT;
            *dest++ += whoop;
            *dest++ -= whoop;
            vnf->rampvol--;
        } else if(vnf->click)
        {   whoop = ((((SLONGLONG)vnf->lvolsel * (CLICK_BUFFER-vnf->click)) * (SLONGLONG)sample) + ((SLONGLONG)vnf->lastvalL * vnf->click)) >> CLICK_SHIFT;
            *dest++ += whoop;
            *dest++ -= whoop;
            vnf->click--;
        } else
        {   *dest++ += vnf->lvolsel * sample;
            *dest++ -= vnf->lvolsel * sample;
        }
    }

    vnf->lastvalL = vnf->lvolsel * sample;
    vnf->lastvalR = vnf->lvolsel * sample;

    return index;
}


static SLONGLONG MixMonoNormal(SWORD *srce, SLONG *dest, SLONGLONG index, SLONGLONG increment, SLONG todo)
{
    SWORD  sample;
    
    for(; todo; todo--)
    {   sample = (SWORD)((SLONGLONG)(srce[index >> FRACBITS] * ((FRACMASK+1L) - (index & FRACMASK))) +
                 ((SLONGLONG)srce[(index >> FRACBITS)+1] * (index & FRACMASK)) >> FRACBITS);
        index += increment;

        if(vnf->rampvol)
        {   *dest++ += (((SLONGLONG)(vnf->oldlvol * vnf->rampvol) + (vnf->lvolsel * (CLICK_BUFFER-vnf->rampvol))) * (SLONGLONG)sample) >> CLICK_SHIFT;
            vnf->rampvol--;
        } else if(vnf->click)
        {   *dest++ += ((((SLONGLONG)vnf->lvolsel * (CLICK_BUFFER-vnf->click)) * (SLONGLONG)sample) + ((SLONGLONG)vnf->lastvalL * vnf->click)) >> CLICK_SHIFT;
            vnf->click--;
        } else
            *dest++ += vnf->lvolsel * sample;
    }

    vnf->lastvalL = vnf->lvolsel * sample;

    return index;
}

#endif

static void (*Mix32to16)(SWORD *dste, SLONG *srce, SLONG count);
static void (*Mix32to8)(SBYTE *dste, SLONG *srce, SLONG count);
static void (*MixReverb)(SLONG *srce, SLONG count);


static void MixReverb_Normal(SLONG *srce, SLONG count)
{
    SLONG        speedup;
    int          ReverbPct;
    unsigned int loc1, loc2, loc3, loc4,
                 loc5, loc6, loc7, loc8;

    ReverbPct = 58 + (md_reverb*4);

    loc1 = RVRindex % RVc1;
    loc2 = RVRindex % RVc2;
    loc3 = RVRindex % RVc3;
    loc4 = RVRindex % RVc4;
    loc5 = RVRindex % RVc5;
    loc6 = RVRindex % RVc6;
    loc7 = RVRindex % RVc7;
    loc8 = RVRindex % RVc8;

    for(; count; count--)
    {
        // Compute the LEFT CHANNEL echo buffers!

        speedup = *srce >> 3;

        RVbuf1[loc1] = speedup + ((ReverbPct * RVbuf1[loc1]) / 128);
        RVbuf2[loc2] = speedup + ((ReverbPct * RVbuf2[loc2]) / 128);
        RVbuf3[loc3] = speedup + ((ReverbPct * RVbuf3[loc3]) / 128);
        RVbuf4[loc4] = speedup + ((ReverbPct * RVbuf4[loc4]) / 128);
        RVbuf5[loc5] = speedup + ((ReverbPct * RVbuf5[loc5]) / 128);
        RVbuf6[loc6] = speedup + ((ReverbPct * RVbuf6[loc6]) / 128);
        RVbuf7[loc7] = speedup + ((ReverbPct * RVbuf7[loc7]) / 128);
        RVbuf8[loc8] = speedup + ((ReverbPct * RVbuf8[loc8]) / 128);

        // Prepare to compute actual finalized data!

        RVRindex++;
        loc1 = RVRindex % RVc1;
        loc2 = RVRindex % RVc2;
        loc3 = RVRindex % RVc3;
        loc4 = RVRindex % RVc4;
        loc5 = RVRindex % RVc5;
        loc6 = RVRindex % RVc6;
        loc7 = RVRindex % RVc7;
        loc8 = RVRindex % RVc8;

        // Left Channel!
        
        *srce++ += (RVbuf1[loc1] - RVbuf2[loc2] + RVbuf3[loc3] - RVbuf4[loc4] +
                    RVbuf5[loc5] - RVbuf6[loc6] + RVbuf7[loc7] - RVbuf8[loc8]);
    }
}


static void MixReverb_Stereo(SLONG *srce, SLONG count)
{
    SLONG        speedup;
    int          ReverbPct;
    unsigned int loc1, loc2, loc3, loc4,
                 loc5, loc6, loc7, loc8;

    ReverbPct = 58 + (md_reverb*4);

    loc1 = RVRindex % RVc1;
    loc2 = RVRindex % RVc2;
    loc3 = RVRindex % RVc3;
    loc4 = RVRindex % RVc4;
    loc5 = RVRindex % RVc5;
    loc6 = RVRindex % RVc6;
    loc7 = RVRindex % RVc7;
    loc8 = RVRindex % RVc8;

    for(; count; count--)
    {
        // Compute the LEFT CHANNEL echo buffers!

        speedup = *srce >> 3;

        RVbuf1[loc1] = speedup + ((ReverbPct * RVbuf1[loc1]) / 128);
        RVbuf2[loc2] = speedup + ((ReverbPct * RVbuf2[loc2]) / 128);
        RVbuf3[loc3] = speedup + ((ReverbPct * RVbuf3[loc3]) / 128);
        RVbuf4[loc4] = speedup + ((ReverbPct * RVbuf4[loc4]) / 128);
        RVbuf5[loc5] = speedup + ((ReverbPct * RVbuf5[loc5]) / 128);
        RVbuf6[loc6] = speedup + ((ReverbPct * RVbuf6[loc6]) / 128);
        RVbuf7[loc7] = speedup + ((ReverbPct * RVbuf7[loc7]) / 128);
        RVbuf8[loc8] = speedup + ((ReverbPct * RVbuf8[loc8]) / 128);

        // Compute the RIGHT CHANNEL echo buffers!
        
        speedup = srce[1] >> 3;

        RVbuf9[loc1] = speedup + ((ReverbPct * RVbuf9[loc1]) / 128);
        RVbuf10[loc2] = speedup + ((ReverbPct * RVbuf11[loc2]) / 128);
        RVbuf11[loc3] = speedup + ((ReverbPct * RVbuf12[loc3]) / 128);
        RVbuf12[loc4] = speedup + ((ReverbPct * RVbuf12[loc4]) / 128);
        RVbuf13[loc5] = speedup + ((ReverbPct * RVbuf13[loc5]) / 128);
        RVbuf14[loc6] = speedup + ((ReverbPct * RVbuf14[loc6]) / 128);
        RVbuf15[loc7] = speedup + ((ReverbPct * RVbuf15[loc7]) / 128);
        RVbuf16[loc8] = speedup + ((ReverbPct * RVbuf16[loc8]) / 128);

        // Prepare to compute actual finalized data!

        RVRindex++;
        loc1 = RVRindex % RVc1;
        loc2 = RVRindex % RVc2;
        loc3 = RVRindex % RVc3;
        loc4 = RVRindex % RVc4;
        loc5 = RVRindex % RVc5;
        loc6 = RVRindex % RVc6;
        loc7 = RVRindex % RVc7;
        loc8 = RVRindex % RVc8;

        // Left Channel!
        
        *srce++ += RVbuf1[loc1] - RVbuf2[loc2] + RVbuf3[loc3] - RVbuf4[loc4] +
                   RVbuf5[loc5] - RVbuf6[loc6] + RVbuf7[loc7] - RVbuf8[loc8];

        // Right Channel!
        
        *srce++ += RVbuf9[loc1] - RVbuf10[loc2] + RVbuf11[loc3] - RVbuf12[loc4] +
                   RVbuf13[loc5] - RVbuf14[loc6] + RVbuf15[loc7] - RVbuf16[loc8];
    }
}


static void Mix32To16_Normal(SWORD *dste, SLONG *srce, SLONG count)
{
    SLONG    x1, x2, tmpx;
    int      i;
    
    for(count/=SAMPLING_FACTOR; count; count--)
    {   tmpx = 0;

        for(i=SAMPLING_FACTOR/2; i; i--)
        {  x1 = *srce++ / MAXVOL_FACTOR;
           x2 = *srce++ / MAXVOL_FACTOR;

           x1 = (x1 > 32767) ? 32767 : (x1 < -32768) ? -32768 : x1;
           x2 = (x2 > 32767) ? 32767 : (x2 < -32768) ? -32768 : x2;

           tmpx += x1 + x2;
        }

        *dste++ = tmpx / SAMPLING_FACTOR;
    }
}


static void Mix32To16_Stereo(SWORD *dste, SLONG *srce, SLONG count)
{
    SLONG   x1, x2, x3, x4, tmpx, tmpy;
    int     i;

    for(count/=SAMPLING_FACTOR; count; count--)
    {   tmpx = tmpy = 0;

        for(i=SAMPLING_FACTOR/2; i; i--)
        {   x1 = *srce++ / MAXVOL_FACTOR;
            x2 = *srce++ / MAXVOL_FACTOR;
            x3 = *srce++ / MAXVOL_FACTOR;
            x4 = *srce++ / MAXVOL_FACTOR;

            x1 = (x1 > 32767) ? 32767 : (x1 < -32768) ? -32768 : x1;
            x2 = (x2 > 32767) ? 32767 : (x2 < -32768) ? -32768 : x2;
            x3 = (x3 > 32767) ? 32767 : (x3 < -32768) ? -32768 : x3;
            x4 = (x4 > 32767) ? 32767 : (x4 < -32768) ? -32768 : x4;

            tmpx += x1 + x3;
            tmpy += x2 + x4;
        }

        *dste++ = tmpx / SAMPLING_FACTOR;
        *dste++ = tmpy / SAMPLING_FACTOR;
    }
}


static void Mix32To8_Normal(SBYTE *dste, SLONG *srce, SLONG count)
{
    int    x1, x2;
    int    i, tmpx;
    
    for(count/=SAMPLING_FACTOR; count; count--)
    {   tmpx = 0;

        for(i=SAMPLING_FACTOR/2; i; i--)
        {   x1 = *srce++ / (MAXVOL_FACTOR * 256);
            x2 = *srce++ / (MAXVOL_FACTOR * 256);

            x1 = (x1 > 127) ? 127 : (x1 < -128) ? -128 : x1;
            x2 = (x2 > 127) ? 127 : (x2 < -128) ? -128 : x2;

            tmpx += x1 + x2;
        }

        *dste++ = (tmpx / SAMPLING_FACTOR) + 128;
    }
}


static void Mix32To8_Stereo(SBYTE *dste, SLONG *srce, SLONG count)
{
    int    x1, x2, x3, x4;
    int    i, tmpx, tmpy;

    for(count/=SAMPLING_FACTOR; count; count--)
    {   tmpx = tmpy = 0;

        for(i=SAMPLING_FACTOR/2; i; i--)
        {   x1 = *srce++ / (MAXVOL_FACTOR * 256);
            x2 = *srce++ / (MAXVOL_FACTOR * 256);
            x3 = *srce++ / (MAXVOL_FACTOR * 256);
            x4 = *srce++ / (MAXVOL_FACTOR * 256);
    
            x1 = (x1 > 127) ? 127 : (x1 < -128) ? -128 : x1;
            x2 = (x2 > 127) ? 127 : (x2 < -128) ? -128 : x2;
            x3 = (x3 > 127) ? 127 : (x3 < -128) ? -128 : x3;
            x4 = (x4 > 127) ? 127 : (x4 < -128) ? -128 : x4;
    
            tmpx += x1 + x3;
            tmpy += x2 + x4;
        }

        *dste++ = (tmpx / SAMPLING_FACTOR) + 128;        
        *dste++ = (tmpy / SAMPLING_FACTOR) + 128;        
    }
}


static ULONG samples2bytes(ULONG samples)
{
    if(vc_mode & DMODE_16BITS) samples <<= 1;
    if(vc_mode & DMODE_STEREO) samples <<= 1;

    return samples;
}


static ULONG bytes2samples(ULONG bytes)
{
    if(vc_mode & DMODE_16BITS) bytes >>= 1;
    if(vc_mode & DMODE_STEREO) bytes >>= 1;

    return bytes;
}


static void AddChannel(SLONG *ptr, SLONG todo)
{
    FRACTYPE  end;
    SLONG     done;
    SWORD     *s;

    if(!(s=Samples[vnf->handle]))
    {   vnf->current = 0;
        vnf->active  = 0;

        vnf->lastvalL = 0;
        vnf->lastvalR = 0;

        return;
    }

    while(todo > 0)
    {   // update the 'current' index so the sample loops, or
        // stops playing if it reached the end of the sample

        if(vnf->flags & SF_REVERSE)
        {
            // The sample is playing in reverse

            if((vnf->flags & SF_LOOP) && (vnf->current < idxlpos))
            {
                // the sample is looping, and it has
                // reached the loopstart index

                if(vnf->flags & SF_BIDI)
                {
                    // sample is doing bidirectional loops, so 'bounce'
                    // the current index against the idxlpos

                    vnf->current    = idxlpos + (idxlpos - vnf->current);
                    vnf->flags     &= ~SF_REVERSE;
                    vnf->increment  = -vnf->increment;
                } else
                    // normal backwards looping, so set the
                    // current position to loopend index

                   vnf->current = idxlend - (idxlpos-vnf->current);
            } else
            {
                // the sample is not looping, so check
                // if it reached index 0

                if(vnf->current < 0)
                {
                    // playing index reached 0, so stop
                    // playing this sample

                    vnf->current = 0;
                    vnf->active  = 0;
                    break;
                }
            }
        } else
        {
            // The sample is playing forward

            if((vnf->flags & SF_LOOP) && (vnf->current > idxlend))
            {
                 // the sample is looping, so check if
                 // it reached the loopend index

                 if(vnf->flags & SF_BIDI)
                 {
                     // sample is doing bidirectional loops, so 'bounce'
                     //  the current index against the idxlend

                     vnf->flags    |= SF_REVERSE;
                     vnf->increment = -vnf->increment;
                     vnf->current   = idxlend - (vnf->current-idxlend);
                 } else
                     // normal backwards looping, so set the
                     // current position to loopend index

                     vnf->current = idxlpos + (vnf->current-idxlend);
            } else
            {
                // sample is not looping, so check
                // if it reached the last position

                if(vnf->current > idxsize)
                {
                    // yes, so stop playing this sample

                    vnf->current = 0;
                    vnf->active  = 0;
                    break;
                }
            }
        }

        end = (FRACTYPE)(vnf->flags & SF_REVERSE) ? 
                (vnf->flags & SF_LOOP) ? idxlpos : 0 :
                (vnf->flags & SF_LOOP) ? idxlend : idxsize;

        done = MIN((end - vnf->current) / vnf->increment + 1, todo);

        if(!done)
        {   vnf->active = 0;
            break;
        }

        if(vnf->vol || vnf->rampvol)
        {   if(vc_mode & DMODE_STEREO)
                if((vnf->pan == PAN_SURROUND) && (vc_mode & DMODE_SURROUND))
                    vnf->current = MixStereoSurround(s,ptr,vnf->current,vnf->increment,done);
                else
                    vnf->current = MixStereoNormal(s,ptr,vnf->current,vnf->increment,done);
            else
                vnf->current = MixMonoNormal(s,ptr,vnf->current,vnf->increment,done);
        } else
        {   vnf->lastvalL = 0;
            vnf->lastvalR = 0;
        }

        todo -= done;
        ptr  += (vc_mode & DMODE_STEREO) ? (done<<1) : done;
    }
}


void VC2_WriteSamples(SBYTE *buf, long todo)
{
    int    left, portion = 0;
    SBYTE  *buffer;
    int    t;
    int    pan, vol;

    todo *= SAMPLING_FACTOR;

    while(todo)
    {   if(TICKLEFT==0)
        {   if(vc_mode & DMODE_SOFT_MUSIC) md_player();
            TICKLEFT = (md_mixfreq * 125l * SAMPLING_FACTOR) / (md_bpm * 50l);
            TICKLEFT &= ~(SAMPLING_FACTOR-1);
        }

        left = MIN(TICKLEFT, todo);
        
        buffer    = buf;
        TICKLEFT -= left;
        todo     -= left;

        buf += samples2bytes(left) / SAMPLING_FACTOR;

        while(left)
        {   portion = MIN(left, samplesthatfit);
            memset(VC2_TICKBUF, 0, portion << ((vc_mode & DMODE_STEREO) ? 3 : 2));

            for(t=0; t<vc_softchn; t++)
            {   vnf = &vinf[t];

                if(vnf->kick)
                {
                    #ifdef USE_64BIT_INTS
                    vnf->current = (SLONGLONG)(vnf->start) << FRACBITS;
                    #else
                    vnf->current = (double)vnf->start;
                    #endif

                    vnf->kick    = 0;
                    vnf->active  = 1;
                    vnf->click   = CLICK_BUFFER;
                    vnf->rampvol = 0;
                }
                
                if(vnf->frq == 0) vnf->active = 0;
                
                if(vnf->active)
                {
                    #ifdef USE_64BIT_INTS
                    vnf->increment = ((SLONGLONG)(vnf->frq) << (FRACBITS-SAMPLING_SHIFT)) / (SLONGLONG)md_mixfreq;
                    #else
                    vnf->increment = ((double)(vnf->frq) / (1<<SAMPLING_SHIFT)) / (double)md_mixfreq;
                    #endif

                    if(vnf->flags & SF_REVERSE) vnf->increment = -vnf->increment;

                    vol = vnf->vol;  pan = vnf->pan;

                    if(vc_mode & DMODE_STEREO)
                    {   if(pan!=PAN_SURROUND)
                        {   vnf->oldlvol = vnf->lvolsel;  vnf->oldrvol = vnf->rvolsel;
                            vnf->lvolsel = (vol * (255-pan)) >> 8;
                            vnf->rvolsel = (vol * pan) >> 8;
                        } else
                        {   vnf->oldlvol = vnf->lvolsel;
                            vnf->lvolsel = (vol * 256l) / 480;
                        }
                    } else
                    {   vnf->oldlvol = vnf->lvolsel;
                        vnf->lvolsel = vol;
                    }

                    #ifdef USE_64BIT_INTS
                    idxsize = (vnf->size)   ? ((SLONGLONG)(vnf->size) << FRACBITS)-1 : 0;
                    idxlend = (vnf->repend) ? ((SLONGLONG)(vnf->repend) << FRACBITS)-1 : 0;
                    idxlpos = (SLONGLONG)(vnf->reppos) << FRACBITS;
                    #else
                    idxsize = (double)(vnf->size);     // ? vnf->size : 0);
                    idxlend = (double)(vnf->repend);   // ? vnf->repend : 0);
                    idxlpos = (double)(vnf->reppos);
                    #endif
                    AddChannel(VC2_TICKBUF, portion);
                }
            }

            if(md_reverb) MixReverb(VC2_TICKBUF, portion);

            if(vc_mode & DMODE_16BITS)
                Mix32to16((SWORD *) buffer, VC2_TICKBUF, portion);
            else
                Mix32to8((SBYTE *) buffer, VC2_TICKBUF, portion);

            buffer += samples2bytes(portion) / SAMPLING_FACTOR;
            left   -= portion;
        }
    }
}


void VC2_SilenceBytes(SBYTE *buf, long todo)

//  Fill the buffer with 'todo' bytes of silence (it depends on the mixing
//  mode how the buffer is filled)

{
    // clear the buffer to zero (16 bits
    // signed ) or 0x80 (8 bits unsigned)

    if(vc_mode & DMODE_16BITS)
        memset(buf,0,todo);
    else
        memset(buf,0x80,todo);
}


ULONG VC2_WriteBytes(SBYTE *buf, long todo)

//  Writes 'todo' mixed SBYTES (!!) to 'buf'. It returns the number of
//  SBYTES actually written to 'buf' (which is rounded to number of samples
//  that fit into 'todo' bytes).

{
    if(vc_softchn == 0)
    {   VC2_SilenceBytes(buf,todo);
        return todo;
    }

    todo = bytes2samples(todo);
    VC2_WriteSamples(buf,todo);

    return samples2bytes(todo);
}


BOOL VC2_PlayStart(void)
{
    md_mode |= DMODE_INTERP;

    samplesthatfit = TICKLSIZE;
    if(vc_mode & DMODE_STEREO) samplesthatfit >>= 1;
    TICKLEFT = 0;

#ifndef USE_64BIT_INTS
    RVc1 = (SLONG)(500.0    * md_mixfreq) / REVERBERATION;
    RVc2 = (SLONG)(507.8125 * md_mixfreq) / REVERBERATION;
    RVc3 = (SLONG)(531.25   * md_mixfreq) / REVERBERATION;
    RVc4 = (SLONG)(570.3125 * md_mixfreq) / REVERBERATION;
    RVc5 = (SLONG)(625.0    * md_mixfreq) / REVERBERATION;
    RVc6 = (SLONG)(695.3125 * md_mixfreq) / REVERBERATION;
    RVc7 = (SLONG)(781.25   * md_mixfreq) / REVERBERATION;
    RVc8 = (SLONG)(882.8125 * md_mixfreq) / REVERBERATION;
#else
    RVc1 = (5000L * md_mixfreq) / (REVERBERATION * 10);
    RVc2 = (5078L * md_mixfreq) / (REVERBERATION * 10);
    RVc3 = (5313L * md_mixfreq) / (REVERBERATION * 10);
    RVc4 = (5703L * md_mixfreq) / (REVERBERATION * 10);
    RVc5 = (6250L * md_mixfreq) / (REVERBERATION * 10);
    RVc6 = (6953L * md_mixfreq) / (REVERBERATION * 10);
    RVc7 = (7813L * md_mixfreq) / (REVERBERATION * 10);
    RVc8 = (8828L * md_mixfreq) / (REVERBERATION * 10);
#endif

    if((RVbuf1 = (SLONG *)_mm_calloc((RVc1+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf2 = (SLONG *)_mm_calloc((RVc2+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf3 = (SLONG *)_mm_calloc((RVc3+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf4 = (SLONG *)_mm_calloc((RVc4+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf5 = (SLONG *)_mm_calloc((RVc5+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf6 = (SLONG *)_mm_calloc((RVc6+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf7 = (SLONG *)_mm_calloc((RVc7+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf8 = (SLONG *)_mm_calloc((RVc8+1),sizeof(SLONG))) == NULL) return 1;

    if((RVbuf9 = (SLONG *)_mm_calloc((RVc1+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf10 = (SLONG *)_mm_calloc((RVc2+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf11 = (SLONG *)_mm_calloc((RVc3+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf12 = (SLONG *)_mm_calloc((RVc4+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf13 = (SLONG *)_mm_calloc((RVc5+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf14 = (SLONG *)_mm_calloc((RVc6+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf15 = (SLONG *)_mm_calloc((RVc7+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf16 = (SLONG *)_mm_calloc((RVc8+1),sizeof(SLONG))) == NULL) return 1;
    
    RVRindex = 0;
    return 0;
}


void VC2_PlayStop(void)
{
    if(RVbuf1  != NULL) _mm_free(RVbuf1);
    if(RVbuf2  != NULL) _mm_free(RVbuf2);
    if(RVbuf3  != NULL) _mm_free(RVbuf3);
    if(RVbuf4  != NULL) _mm_free(RVbuf4);
    if(RVbuf5  != NULL) _mm_free(RVbuf5);
    if(RVbuf6  != NULL) _mm_free(RVbuf6);
    if(RVbuf7  != NULL) _mm_free(RVbuf7);
    if(RVbuf8  != NULL) _mm_free(RVbuf8);
    if(RVbuf9  != NULL) _mm_free(RVbuf9);
    if(RVbuf10 != NULL) _mm_free(RVbuf10);
    if(RVbuf11 != NULL) _mm_free(RVbuf11);
    if(RVbuf12 != NULL) _mm_free(RVbuf12);
    if(RVbuf13 != NULL) _mm_free(RVbuf13);
    if(RVbuf14 != NULL) _mm_free(RVbuf14);
    if(RVbuf15 != NULL) _mm_free(RVbuf15);
    if(RVbuf16 != NULL) _mm_free(RVbuf16);

    RVbuf1  = RVbuf2  = RVbuf3  = RVbuf4  = RVbuf5  = RVbuf6  = RVbuf7  = RVbuf8  = NULL;
    RVbuf9  = RVbuf10 = RVbuf11 = RVbuf12 = RVbuf13 = RVbuf14 = RVbuf15 = RVbuf16 = NULL;
}


BOOL VC2_Init(void)
{
    _mm_errno = MMERR_INITIALIZING_MIXER;
    if((Samples = (SWORD **)calloc(MAXSAMPLEHANDLES, sizeof(SWORD *))) == NULL) return 1;
    if(VC2_TICKBUF==NULL) if((VC2_TICKBUF=(SLONG *)malloc((TICKLSIZE+32) * sizeof(SLONG))) == NULL) return 1;

    if(md_mode & DMODE_STEREO)
    {   Mix32to16  = Mix32To16_Stereo;
        Mix32to8   = Mix32To8_Stereo;
        MixReverb  = MixReverb_Stereo;
    } else
    {   Mix32to16  = Mix32To16_Normal;
        Mix32to8   = Mix32To8_Normal;
        MixReverb  = MixReverb_Normal;
    }

    md_mode |= DMODE_INTERP;
    vc_mode = md_mode;

    _mm_errno = 0;
    return 0;
}


void VC2_Exit(void)

// Yay, the joys and fruits of C and C++ -
//                        Deallocation of arrays!

{
    //if(VC2_TICKBUF!=NULL) free(VC2_TICKBUF);
    if(vinf!=NULL) free(vinf);
    if(Samples!=NULL) free(Samples);

//    VC2_TICKBUF     = NULL;
    vinf            = NULL;
    Samples         = NULL;
}


BOOL VC2_SetNumVoices(void)
{
    int t;

    md_mode |= DMODE_INTERP;

    if((vc_softchn = md_softchn) == 0) return 0;

    if(vinf!=NULL) free(vinf);
    if((vinf = _mm_calloc(sizeof(VINFO),vc_softchn)) == NULL) return 1;
    
    for(t=0; t<vc_softchn; t++)
    {   vinf[t].frq = 10000;
        vinf[t].pan = (t & 1) ? 0 : 255;
    }

    return 0;
}


void VC2_VoiceSetVolume(UBYTE voice, UWORD vol)
{    
    if(abs((int)vinf[voice].vol - (int)vol) > 32) vinf[voice].rampvol = CLICK_BUFFER;
    vinf[voice].vol     = vol;
}


void VC2_VoiceSetFrequency(UBYTE voice, ULONG frq)
{
    vinf[voice].frq = frq;
}


void VC2_VoiceSetPanning(UBYTE voice, int pan)
{
    if(abs((int)vinf[voice].pan - (int)pan) > 48) vinf[voice].rampvol = CLICK_BUFFER;
    vinf[voice].pan = pan;
}


void VC2_VoicePlay(UBYTE voice, SWORD handle, ULONG start, ULONG size, ULONG reppos, ULONG repend, UWORD flags)
{
    vinf[voice].flags    = flags;
    vinf[voice].handle   = handle;
    vinf[voice].start    = start;
    vinf[voice].size     = size;
    vinf[voice].reppos   = reppos;
    vinf[voice].repend   = repend;
    vinf[voice].kick     = 1;
}


void VC2_VoiceStop(UBYTE voice)
{
    vinf[voice].active = 0;
}  


BOOL VC2_VoiceStopped(UBYTE voice)
{
    return(vinf[voice].active == 0);
}


void VC2_VoiceReleaseSustain(UBYTE voice)
{

}


SLONG VC2_VoiceGetPosition(UBYTE voice)
{
    #ifdef USE_64BIT_INTS
    return(vinf[voice].current >> FRACBITS);
    #else
    return((SLONG)vinf[voice].current);
    #endif
}


/**************************************************
***************************************************
***************************************************
**************************************************/


void VC2_SampleUnload(SWORD handle)
{
    free(Samples[handle]);
    Samples[handle] = NULL;
}


SWORD VC2_SampleLoad(SAMPLOAD *sload, int type)
{
    SAMPLE *s = sload->sample;
    int    handle;
    ULONG  t, length,loopstart,loopend;

    if(type==MD_HARDWARE) return -1;

    // Find empty slot to put sample address in

    for(handle=0; handle<MAXSAMPLEHANDLES; handle++)
        if(Samples[handle]==NULL) break;

    if(handle==MAXSAMPLEHANDLES)
    {   _mm_errno = MMERR_OUT_OF_HANDLES;
        return -1;
    }

    length    = s->length;
    loopstart = s->loopstart;
    loopend   = s->loopend;

    SL_SampleSigned(sload);
    SL_Sample8to16(sload);

    if((Samples[handle]=(SWORD *)malloc((length+20)<<1))==NULL)
    {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
        return -1;
    }

    SL_Load(Samples[handle],sload,length);    // read sample into buffer.

    // Unclick samples:

    if(s->flags & SF_LOOP)
    {   if(s->flags & SF_BIDI)
            for(t=0; t<16; t++) Samples[handle][loopend+t] = Samples[handle][(loopend-t)-1];
        else
            for(t=0; t<16; t++) Samples[handle][loopend+t] = Samples[handle][t+loopstart];
    } else
        for(t=0; t<16; t++) Samples[handle][t+length] = 0;

    return handle;
}


ULONG VC2_SampleSpace(int type)
{
    return vc_memory;
}


ULONG VC2_SampleLength(int type, SAMPLE *s)
{
    return (s->length * ((s->flags & SF_16BITS) ? 2 : 1)) + 16;
}


/**************************************************
***************************************************
***************************************************
**************************************************/

