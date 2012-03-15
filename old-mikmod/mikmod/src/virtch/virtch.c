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
 Module: VIRTCH.C

  Sample mixing routines, using a 32 bits mixing buffer.

  Optional features include:
   (a) Interpolation of sample data during mixing
   (b) Dolby Surround Sound
   (c) Optimized assembly mixers for the Intel platform
   (d) Optional high-speed (8 bit samples) or high-quality (16 bit samples) modes
   (e) Declicking via use of micro-volume ramps.

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

#include "vchcrap.h"

#include <stddef.h>
#include <string.h>

#include <assert.h>

int        rvolsel, lvolsel;

// =====================================================================================
    void VC_RegisterMixer(VIRTCH *vc, VMIXER *mixer)
// =====================================================================================
{
    if(vc)
    {   VMIXER  *cruise;

        // Should I check for duplicates here? :)

        if(cruise = vc->mixerlist)
        {   while(cruise->next) cruise = cruise->next;
            cruise->next = mixer;
        } else
            vc->mixerlist = mixer;

        mixer->next = NULL;
    }
}


// =====================================================================================
    static BOOL configsample(VIRTCH *vc, int handle, uint mixmode)
// =====================================================================================
{
    VMIXER  *cruise;

    // Mask out unimportant mixer settings that have no effect on mixers.

    if(!vc) return 1;
    
    mixmode &= DMODE_INTERP;

    // Search the list of registered sample mixers for one that matches the format
    // of the current sample.

    cruise  = vc->mixerlist;
    //goodone = 1;         // set to one, so that we fail if no good matches
    //goodmix = NULL;

    while(cruise)
    {
        // make sure we check for enabled/disabled resonance filtering!
        
        if(!(vc->mode & DMODE_RESONANCE)) vc->sample[handle].flags &= ~SL_RESONANCE_FILTER;

        // if Check is NULL then we assume an-all-capable mixer!

        if(!cruise->Check || cruise->Check(vc->channels, mixmode, vc->sample[handle].format, vc->sample[handle].flags))
        {   if(cruise->Init)
            {   if(cruise->Init(cruise))
                {   _mmlog("Mikmod > Virtch > Mixer '%s' Failed Init!",cruise->name);
                    return 1;
                }
            }

            vc->sample[handle].mixer = cruise;
            return 0;
        }

        cruise = cruise->next;
    }

    // woops, we can't mix this sample.

    _mmlog("Mikmod > virtch > Attempted to configure sample, but no suitable mixer was available!");
    vc->sample[handle].mixer = &VC_MIXER_PLACEBO;
    assert(FALSE);

    return 0;
}

// =====================================================================================
    static int getfreehandle(VIRTCH *vc)
// =====================================================================================
{
    uint     t, handle;

    if(!vc) return 0;
    
    for(handle=0; handle<vc->samplehandles; handle++)
        if(!vc->sample[handle].data) break;

    if(handle>=vc->samplehandles)
    {   // Allocate More sample handles
        t = vc->samplehandles;
        vc->samplehandles += 32;
        if((vc->sample = (VSAMPLE *)_mm_realloc(&vc->allochandle, vc->sample, sizeof(VSAMPLE) * vc->samplehandles)) == NULL) return -1;
	
        memset(&vc->sample[t], 0, sizeof(VSAMPLE) * 32);
    }

    return handle;
}

/*static void (*calculate_volumes)(VINFO *vnf);
static void volcalc_Quad(VINFO *vnf)
{
    // this isn't complete yet.  It requires new lvoltab/rvoltab for the rear
    // speakers, and some other changes!

    vnf->vol.front.left = vnf->volume.front.left * vc->volume.front.left;
    vnf->vol.front.right = vnf->volume.front.right * vc->volume.front.right;
    vnf->vol.rear.left  = vnf->volume.rear.left  * vc->volume.rear.left;
    vnf->vol.rear.right  = vnf->volume.rear.right  * vc->volume.rear.right;

    if(vnf->panflg & VC_SURROUND)
    {   lvoltab = rvoltab = voltab[(vnf->vol.front.left+1) / BIT8_TBLSCL];
        lvolsel = rvolsel = vnf->vol.front.left / BIT16_VOLFAC;
    } else
    {   lvolsel = vnf->vol.front.left / BIT16_VOLFAC;
        rvolsel = vnf->vol.front.right / BIT16_VOLFAC;
        lvoltab = voltab[vnf->vol.front.left / BIT8_TBLSCL];
        rvoltab = voltab[vnf->vol.front.right / BIT8_TBLSCL];
        //_mmlog("  Virtch : %-3d, %-3d",vnf->vol.front.left/ BIT8_TBLSCL, vnf->vol.front.right/ BIT8_TBLSCL);
    }

    // declicker: Set us up to volramp!
    if((vc->mode & DMODE_NOCLICK) && !vnf->volramp)
    {   if((vnf->vol.front.left != vnf->oldvol.front.left) || (vnf->vol.front.right != vnf->oldvol.front.right))
        {   vnf->volramp = vc->mixspeed / 2048;
            vnf->lvolinc = ((vnf->vol.front.left - vnf->oldvol.front.left) / (int)vnf->volramp);
            vnf->rvolinc = ((vnf->vol.front.right - vnf->oldvol.front.right) / (int)vnf->volramp);
            //_mmlog("vols:  %-5d, %-5d  %-5d, %-5d   volincs: %-7d, %-7d",vnf->lvol, vnf->loldvol, vnf->rvol, vnf->roldvol, vnf->lvolinc, vnf->rvolinc);
        }
    }
}*/


#include "mminline.h"

// =====================================================================================
    static void Mix32To16(VIRTCH *vc, SWORD *dste, SLONG *srce, SLONG count)
// =====================================================================================
{
    register SLONG x1, x2;
    int           remain;

#ifdef CPU_INTEL
    if(vc->cpu >= CPU_MMX)
    {   __asm
        {
            mov         eax, count
            mov         edi, dste
            mov         ecx, eax
            mov         esi, srce
            shr         ecx, 4                // counter / 16 (ecx)
            jz          getOuttaHere

        mainLoop:

            movq        mm7, [esi]
            movq        mm6, [esi+8]
            psrad       mm7, BITSHIFT
            movq        mm5, [esi+16]
            psrad       mm6, BITSHIFT
            movq        mm4, [esi+24]
            psrad       mm5, BITSHIFT
            movq        mm3, [esi+32]
            psrad       mm4, BITSHIFT
            movq        mm2, [esi+40]
            psrad       mm3, BITSHIFT
            movq        mm1, [esi+48]
            psrad       mm2, BITSHIFT
            movq        mm0, [esi+56]
            psrad       mm1, BITSHIFT
            add         esi, 64
            psrad       mm0, BITSHIFT

            add         edi, 32
            packssdw    mm7, mm6         // left/right or backwards?
            packssdw    mm5, mm4
            movq        [edi-32], mm7
            packssdw    mm3, mm2
            movq        [edi-24], mm5
            packssdw    mm1, mm0

            movq        [edi-16], mm3
            movq        [edi-8], mm1

            dec         ecx
            jnz         mainLoop

            emms

        getOuttaHere:
            and         eax, 15
            mov         dste, edi
            mov         srce, esi
            mov         count, eax
        }
    }

#endif
    {   remain = count & 1;

        for(count>>=1; count; count--)
        {   x1 = *srce++ >> BITSHIFT;
            x2 = *srce++ >> BITSHIFT;

            #ifdef BOUNDS_CHECKING
            x1 = (x1 > 32767) ? 32767 : (x1 < -32768) ? -32768 : x1;
            x2 = (x2 > 32767) ? 32767 : (x2 < -32768) ? -32768 : x2;
            #endif

            *dste++ = x1;
            *dste++ = x2;
        }

        for(; remain; remain--)
        {   x1 = *srce++ >> BITSHIFT;
            #ifdef BOUNDS_CHECKING
            x1 = (x1 > 32767) ? 32767 : (x1 < -32768) ? -32768 : x1;
            #endif
            *dste++ = x1;
        }
    }
}


// =====================================================================================
    static void Mix32To8(VIRTCH *vc, SBYTE *dste, SLONG *srce, SLONG count)
// =====================================================================================
{
    register int   x1, x2;
    int   remain;
    
#ifdef CPU_INTEL
    if(vc->cpu >= CPU_MMX)
    {   static unsigned long q128[2] = { 0x80808080, 0x80808080 };
        __asm
        {
            mov         eax, count
            mov         edi, dste
            mov         ecx, eax
            mov         esi, srce
            shr         ecx, 4                // counter / 16 (ecx)
            jz          getOuttaHere

        // if the order of this code looks confusing, that is because it
        // is.  In order to do good pipelinging in MMX, I have to write
        // ass backwards code.  Live with it. :)
        
        mainLoop:
            movq        mm7, [esi]
            movq        mm6, [esi+8]
            psrad       mm7, (BITSHIFT + 8)
            movq        mm5, [esi+16]
            psrad       mm6, (BITSHIFT + 8)
            movq        mm4, [esi+24]
            psrad       mm5, (BITSHIFT + 8)
            movq        mm3, [esi+32]
            psrad       mm4, (BITSHIFT + 8)
            movq        mm2, [esi+40]
            psrad       mm3, (BITSHIFT + 8)
            movq        mm1, [esi+48]
            psrad       mm2, (BITSHIFT + 8)
            movq        mm0, [esi+56]

            psrad       mm1, (BITSHIFT + 8)
            psrad       mm0, (BITSHIFT + 8)
            add         esi, 64

            packssdw    mm7, mm6         // left/right or backwards
            add         edi, 16
            packssdw    mm5, mm4
            packssdw    mm3, mm2
            packsswb    mm7, mm5

            packssdw    mm1, mm0

            packsswb    mm3, mm1            
            paddb       mm7, q128        // convert -128..127 to 0..255

            movq        [edi-16], mm7
            paddb       mm3, q128        // convert -128..127 to 0..255

            movq        [edi-8], mm3

            dec         ecx
            jnz         mainLoop

            emms

        getOuttaHere:
            and         eax, 15
            mov         dste, edi
            mov         srce, esi
            mov         count, eax
        }
    }

#endif
    {   remain = count & 1;
    
        for(count>>=1; count; count--)
        {   x1 = *srce++ >> (BITSHIFT + 8);
            x2 = *srce++ >> (BITSHIFT + 8);

            #ifdef BOUNDS_CHECKING
            x1 = (x1 > 127) ? 127 : (x1 < -128) ? -128 : x1;
            x2 = (x2 > 127) ? 127 : (x2 < -128) ? -128 : x2;
            #endif

            *dste++ = x1 + 128;
            *dste++ = x2 + 128;
        }

        for(; remain; remain--)
        {   x1 = *srce++ >> (BITSHIFT + 8);
            #ifdef BOUNDS_CHECKING
            x1 = (x1 > 127) ? 127 : (x1 < -128) ? -128 : x1;
            #endif
            *dste++ = x1 + 128;
        }
    }
}


// =====================================================================================
    static ULONG __inline samples2bytes(VIRTCH *vc, ULONG samples)
// =====================================================================================
{
    if(vc->mode & DMODE_16BITS) samples <<= 1;
    samples <<= (vc->channels / 2);
    return samples;
}


// =====================================================================================
    static ULONG __inline bytes2samples(VIRTCH *vc, ULONG bytes)
// =====================================================================================
{
    if(vc->mode & DMODE_16BITS) bytes >>= 1;
    bytes >>= (vc->channels / 2);
    return bytes;
}


// =====================================================================================
    static void __inline fullstop(VINFO *vnf)
// =====================================================================================
// Completely cuts a note so that it cannot be resumed or anything (usually done when
// the sample boudn to the voice has reached it's end and has nothing more to play).
{
    vnf->onhold       = TRUE;
    vnf->samplehandle = -1;
}


// =====================================================================================
    static void AddChannel(VIRTCH *vc, SLONG *ptr, SLONG todo, VINFO *vnf) //, int idxsize, int idxlpos, int idxlend)
// =====================================================================================
{
    SLONG      end, *current_hi;
    int        done;
    int        idxsize = vnf->size;
    VSAMPLE    *sinf   = &vc->sample[vnf->samplehandle];

    if(!sinf->data)
    {   vnf->current  = 0;
        fullstop(vnf);
        return;
    }

    current_hi = _mm_HI_SLONG(vnf->current);

    // Sampledata PrePrep --
    //   Looping requires us to copy the sample data from the front of the loop to the
    //   end of it, in order to avoid the interpolation mixer from reading bad data.
    // a) save sample data into buffer
    // b) copy start of loop to the space after the loop
    // c) special case for bidi: copy the end of the loop to the space after the loop, in reverse.
        
    if(sinf->format & SF_16BITS)
    {   if(vnf->flags & SL_SUSTAIN_LOOP)
        {   memcpy(vnf->loopbuf, &sinf->data[vnf->susend*2], 16); 
            if(vnf->flags & SL_SUSTAIN_BIDI)
            {   int   i;
                for(i=0; i<8; i++)
                {   sinf->data[(vnf->susend*2) + i] = sinf->data[(vnf->susend*2) - i - 2];
                    sinf->data[(vnf->susend*2) + i + 1] = sinf->data[(vnf->susend*2) - i - 1];
                }
            } else
                memcpy(&sinf->data[vnf->susend*2], &sinf->data[vnf->suspos*2], 16); 
        } else if(vnf->flags & SL_LOOP)
        {   memcpy(vnf->loopbuf, &sinf->data[vnf->repend*2], 16); 
            if(vnf->flags & SL_BIDI)
            {   int   i;
                for(i=0; i<8; i++)
                {   sinf->data[(vnf->repend*2) + i] = sinf->data[(vnf->repend*2) - i - 2];
                    sinf->data[(vnf->repend*2) + i + 1] = sinf->data[(vnf->repend*2) - i - 1];
                }
            } else
                memcpy(&sinf->data[vnf->repend*2], &sinf->data[vnf->reppos*2], 16); 
        }
    } else
    {   if(vnf->flags & SL_SUSTAIN_LOOP)
        {   memcpy(vnf->loopbuf, &sinf->data[vnf->susend], 16); 
            if(vnf->flags & SL_SUSTAIN_BIDI)
            {   int   i;
                for(i=0; i<16; i++)
                    sinf->data[(vnf->susend) + i] = sinf->data[(vnf->susend) - i - 1];
            } else
                memcpy(&sinf->data[vnf->susend], &sinf->data[vnf->suspos], 16); 
        } else if(vnf->flags & SL_LOOP)
        {   memcpy(vnf->loopbuf, &sinf->data[vnf->repend], 16); 
            if(vnf->flags & SL_BIDI)
            {   int   i;
                for(i=0; i<16; i++)
                    sinf->data[(vnf->repend) + i] = sinf->data[(vnf->repend) - i - 1];
            } else
                memcpy(&sinf->data[vnf->repend], &sinf->data[vnf->reppos], 16); 
        }
    }

    while(todo > 0)
    {   // Callback Check.  Thanks to code below all callbacks will always end up
        // running this code when the index is exactly equal to the current!

        //if(vnf->callback.proc && (*current_hi == vnf->callback.pos))
        //    vnf->callback.proc(&sinf->data[*current_hi], vnf->callback.userinfo);

        // update the 'current' index so the sample loops, or
        // stops playing if it reached the end of the sample

        if(vnf->flags & SL_REVERSE)
        {   // The sample is playing in reverse

            if(vnf->flags & SL_SUSTAIN_LOOP)
            {   end = vnf->suspos;
                if(*current_hi <= vnf->suspos)
                {   // the sample is looping, and it has
                    // reached the loopstart index

                    if(vnf->flags & SL_SUSTAIN_BIDI)
                    {   // sample is doing bidirectional loops, so 'bounce'
                        // the current index against the idxlpos

                        vnf->flags     &= ~SL_REVERSE;
                        vnf->increment  = -vnf->increment;
                        *current_hi     = vnf->suspos + (vnf->suspos - *current_hi);
                        end             = vnf->susend;
                    } else
                    {   // normal backwards looping, so set the
                        // current position to loopend index

                        *current_hi = vnf->susend - (vnf->suspos - *current_hi);
                    }
                }
            } else if(vnf->flags & SL_LOOP)
            {   end = vnf->reppos;
                if(*current_hi <= vnf->reppos)
                {   // the sample is looping, and it has
                    // reached the loopstart index

                    if(vnf->flags & SL_BIDI)
                    {   // sample is doing bidirectional loops, so 'bounce'
                        // the current index against the idxlpos

                        vnf->flags     &= ~SL_REVERSE;
                        vnf->increment  = -vnf->increment;
                        *current_hi     = vnf->reppos + (vnf->reppos - *current_hi);
                        end             = vnf->repend;
                    } else
                        // normal backwards looping, so set the
                        // current position to loopend index

                        *current_hi = vnf->repend - (vnf->reppos - *current_hi);
                }
            } else
            {   // the sample is not looping, so check
                // if it reached index 0

                if(*current_hi <= 0)
                {   // playing index reached 0, so stop
                    // playing this sample
                    vnf->current      = 0;
                    fullstop(vnf);
                    break;
                }
                end = 0;
            }
        } else
        {   // The sample is playing forward

            if(vnf->flags & SL_SUSTAIN_LOOP)
            {   register int idxsend = vnf->susend;

                end  = idxsend;

                if(*current_hi >= idxsend)
                {   // the sample is looping, so check if
                    // it reached the loopend index

                    if(vnf->flags & SL_SUSTAIN_BIDI)
                    {   // sample is doing bidirectional loops, so 'bounce'
                        //  the current index against the idxlend

                        vnf->flags    |= SL_REVERSE;
                        vnf->increment = -vnf->increment;
                        *current_hi    = idxsend - (*current_hi - idxsend);
                        end = vnf->suspos;
                    } else
                    {   // normal forwards looping, so set the
                        // current position to loopstart index

                        *current_hi = vnf->suspos + (*current_hi - idxsend);
                        end         = idxsend;
                    }
                }
            } else if(vnf->flags & SL_LOOP)
            {   register int idxlend = vnf->repend;

                end = idxlend;

                if(*current_hi >= idxlend)
                {   // the sample is looping, so check if
                    // it reached the loopend index

                    if(vnf->flags & SL_BIDI)
                    {   // sample is doing bidirectional loops, so 'bounce'
                        //  the current index against the idxlend

                        vnf->flags    |= SL_REVERSE;
                        vnf->increment = -vnf->increment;
                        *current_hi    = idxlend - (*current_hi - idxlend);
                        end            = vnf->reppos+1;
                    } else
                    {   // normal forwards looping, so set the
                        // current position to loopstart index

                        *current_hi = vnf->reppos + (*current_hi - idxlend);
                    }
                }
            } else
            {   // sample is not looping, so check
                // if it reached the last position

                if(*current_hi >= idxsize)
                {   // yes, so stop playing this sample

                    vnf->current  = 0;
                    fullstop(vnf);
                    break;
                }

                end = idxsize;
            }
        }

        done = (((INT64S)end << FRACBITS) - vnf->current) / vnf->increment + 1;

        if(done<=todo)
        {   if((vc->mode & DMODE_NOCLICK) && (vnf->flags & SL_DECLICK) && !vnf->volramp)
            {   
                // The Final Stage of the Declicker
                // --------------------------------
                // what happens if a composer, in either ignorant or malicious form, puts a pop
                // *AT THE END OF THEIR SAMPLE*?  Well, *I* have to deal with it.  Right here.
                // It becomes *my* problem.  I should be paid for this.

                // make sure done leaves 32 samples for later:

                if(done <= 32)
                {
                    memset(&vnf->volume,0,sizeof(MMVOLUME));
                    vnf->volramp = done;
                    vc->sample[vnf->samplehandle].mixer->CalculateVolumes(vc, vnf);
                } else
                    done -= 32;
            }
        } else
            done = todo;
        
        /*if(!done)
        {   // not enough room to mix
            vnf->current += vnf->increment;
            continue;
        }*/

        // Callback Functionality:
        // -----------------------

        /*if(vnf->callback.proc)
        {   // Callback is active, so see if we are going to cross over the position

            if(vnf->flags & SL_REVERSE)
            {   if((*current_hi > vnf->callback.pos) && ((*current_hi - done) < vnf->callback.pos))
                    done = *current_hi - vnf->callback.pos;
            } else
            {   if((*current_hi < vnf->callback.pos) && ((*current_hi + done) > vnf->callback.pos))
                    done = vnf->callback.pos - *current_hi;
            }
        }*/


        if(vnf->volramp)
        {   // Only mix the number of samples in volramp...
            if(done > vnf->volramp)
                done = vnf->volramp;

            if(vnf->panflg & VC_SURROUND)
                sinf->mixer->NoClickSurround(sinf->data,ptr,vnf->current,vnf->increment,done);
            else
                sinf->mixer->NoClick(sinf->data,ptr,vnf->current,vnf->increment,done);

            // Declicking addtions:
            // Ramp volumes and reset them if need be.

            vnf->volramp -= done;
            sinf->mixer->RampVolume(vnf, done);
            if(vnf->volramp <= 0)
            {   vnf->volramp = 0;
                vnf->oldvol  = vnf->vol;
            }
        } else if(vnf->vol.front.left || vnf->vol.front.right)
        {   if(vnf->panflg & VC_SURROUND)
               sinf->mixer->MixSurround(sinf->data,ptr,vnf->current,vnf->increment,done);
            else
               sinf->mixer->Mix(sinf->data,ptr,vnf->current,vnf->increment,done);
        }

        vnf->current += (vnf->increment*done);
        todo -= done;
        ptr  += done << (vc->channels / 2);
    }

    // undo our sample declicking modifications.
    if(sinf->format & SF_16BITS)
    {   if(vnf->flags & SL_SUSTAIN_LOOP) memcpy(&sinf->data[vnf->susend*2], vnf->loopbuf, 16);
        else if(vnf->flags & SL_LOOP)    memcpy(&sinf->data[vnf->repend*2], vnf->loopbuf, 16);
    } else
    {   if(vnf->flags & SL_SUSTAIN_LOOP) memcpy(&sinf->data[vnf->susend], vnf->loopbuf, 16);
        else if(vnf->flags & SL_LOOP)    memcpy(&sinf->data[vnf->repend], vnf->loopbuf, 16);
    }
}


// =====================================================================================
    void VC_Preempt(MD_DEVICE *md)
// =====================================================================================
// Preempt the mixer as soon as possible to allow the mdriver to update
// the player status and timings.
{
    if(md->vc) md->vc->preemption = 1;
}


// =====================================================================================
    void VC_VoiceSetCallback(MD_DEVICE *md, uint voice, ulong pos, void *userinfo, void (*callback)(SBYTE *dest, void *userinfo))
// =====================================================================================
// Sets a user callback for when a voice reaches a certain sample during mixing. This
// feature allows Mikmod to attach streaming audio mixers to any voice with relative
// simplicity, and complete efficiency.
{
    if(md->vc)
    {   md->vc->vinf[voice].callback.proc     = callback;
        md->vc->vinf[voice].callback.pos      = pos;
        md->vc->vinf[voice].callback.userinfo = userinfo;
    }
}


// =====================================================================================
    void VC_WriteSamples(MDRIVER *md, SBYTE *buf, long todo)
// =====================================================================================
{
    VIRTCH  *vc;
    
    if(vc=md->device.vc)
    {
        int       left;
        uint      portion, count;
        SBYTE    *buffer;
        uint      t;

        while(todo)
        {   // Check if the mdriver has asked for a preemption into the MD_Player
            if(vc->preemption || (vc->TICKLEFT==0))
            {   ulong  timepass;

                vc->preemption = 0;

                // timepass : get the amount of time that has passed since the
                // last MD_Player update (could be volatile thanks to player
                // preemption!)

                timepass = ((INT64U)(vc->TICK - vc->TICKLEFT) * 100000UL) / vc->mixspeed;

                // ADD ONE: Effectively causes the math to round up, instead of down,
                // which prevents us from returning, and in turn getting back, really
                // small values that stall the system!  No accuracy is lost, however,
                // since MD_Player is smart and tracks over/undertimings.

                vc->TICK = vc->TICKLEFT = (((INT64U)(MD_Player(md, timepass)) * vc->mixspeed) / 100000ul) + 1;
            }

            left = MIN(vc->TICKLEFT, todo);
        
            buffer    = buf;
            vc->TICKLEFT -= left;
            todo     -= left;

            buf += samples2bytes(vc,left);

            while(left)
            {   VINFO *vnf;

                portion = MIN(left, vc->samplesthatfit);
                count   = portion << (vc->channels / 2);
            
                memset(vc->TICKBUF, 0, count<<2);

                // Mix in the old channels that are being ramped to 0 volume first...

                if(vc->mode & DMODE_NOCLICK)
                {   uint   tpor, mddiv = RAMPLEN_NOTECUT;

                    tpor = MIN(mddiv, portion);
                    vnf  = vc->vold;

                    for(t=0; t<vc->clickcnt; t++, vnf++)
                    {
                        if(!vnf->onhold) // && vnf->handle->mixer)
                        {   // always ramp to 0 (special coded where vnf->lvol and vnf->rvol
                            // are assumed to always be 0, and ramp using a very fast speed.
                            // Otherwise, the two samples will blend together and a proper
                            // '0' base is never reached!

                            if(!vnf->volramp)
                            {   memset(&vnf->volume,0,sizeof(MMVOLUME));
                                vnf->volramp = mddiv;
                                vc->sample[vnf->samplehandle].mixer->CalculateVolumes(vc, vnf);
                            }

                            if(vnf->volramp && vnf->increment)
                                AddChannel(vc, vc->TICKBUF, tpor, vnf);

                            if(!vnf->volramp) vnf->onhold = TRUE;
                        }
                    }

                    // Only reset the declicker if we didn't do a partial mix.

                    if(tpor>=mddiv) vc->clickcnt = 0;
                }

                // Now mix in the real deal...

                vnf = vc->vinf;
                for(t=0; t<vc->numchn; t++, vnf++)
                {
                    // Check for notes to kick.  Only kick them if
                    // they have a valid mixer attached to them.

                    if(vnf->kick)
                    {   vnf->current = (INT64S)(vnf->start) << FRACBITS;
                        vnf->kick    = 0;
                        vnf->volramp = 0;

                        if(vc->mode & DMODE_NOCLICK)
                        {
                            // check the first four samples of the sample.  If they are properly
                            // formed, then don't declick!

                            if(!vnf->onhold)
                            {   VSAMPLE   *handle = &vc->sample[vnf->samplehandle];
                                if(handle->format & SF_16BITS)
                                {
                                    uint  i;
                                    SWORD *src = (SWORD *)handle->data;
                                    src += vnf->start;
                            
                                    for(i=0; i<4; i++)
                                    {   if((src[i] < -1000) || (src[i] > 1000))
                                        {   vnf->volramp = RAMPLEN_NOTEKICK;
                                            break;
                                        }
                                    }
                                } else
                                {
                                    uint  i;
                                    SBYTE *src = handle->data;
                                    src += vnf->start;
                            
                                    for(i=0; i<4; i++)
                                    {   if((src[i] < -6) || (src[i] > 6))
                                        {   vnf->volramp = RAMPLEN_NOTEKICK;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        memset(&vnf->oldvol,0,sizeof(MMVOLUME));
                    }

                    if(vnf->frq && !vnf->onhold) // && vnf->handle->mixer)
                    {   vnf->increment = ((INT64S)(vnf->frq) << FRACBITS) / (INT64S)vc->mixspeed;
                        vnf->increment -= 3;
                        if(vnf->flags & SL_REVERSE) vnf->increment =- vnf->increment;

                        vc->sample[vnf->samplehandle].mixer->CalculateVolumes(vc, vnf);

                        AddChannel(vc, vc->TICKBUF, portion, vnf);
                    }
                }
                

                if(vc->mode & DMODE_16BITS)
                    Mix32To16(vc, (SWORD *) buffer, vc->TICKBUF, count);
                else
                    Mix32To8(vc, (SBYTE *) buffer, vc->TICKBUF, count);
                buffer += samples2bytes(vc, portion);
                left   -= portion;
            }
        }
    }
}


// =====================================================================================
    void VC_SilenceBytes(MDRIVER *md, SBYTE *buf, long todo)
// =====================================================================================
//  Fill the buffer with 'todo' bytes of silence (it depends on the mixing
//  mode how the buffer is filled)
{
    // clear the buffer to zero (16 bits
    // signed ) or 0x80 (8 bits unsigned)

    if(md->device.vc)
    {   if(md->device.vc->mode & DMODE_16BITS)
            memset(buf,0,todo);
        else
            memset(buf,0x80,todo);
    }
}


// =====================================================================================
    ULONG VC_WriteBytes(MDRIVER *md, SBYTE *buf, long todo)
// =====================================================================================
//  Writes 'todo' mixed SBYTES (!!) to 'buf'. It returns the number of
//  SBYTES actually written to 'buf' (which is rounded to number of samples
//  that fit into 'todo' bytes).
{
    if(!md->device.vc) return 0;

    if(md->device.vc->numchn == 0)
    {   VC_SilenceBytes(md,buf,todo);
        return todo;
    }

    todo = bytes2samples(md->device.vc,todo);
    VC_WriteSamples(md, buf,todo);

    return samples2bytes(md->device.vc,todo);
}


// =====================================================================================
    BOOL VC_SetMode(VIRTCH *vc, uint mixspeed, uint mode, uint channels, uint cpumode)
// =====================================================================================
{
    if(vc)
    {   if(mixspeed) vc->mixspeed = mixspeed;
        vc->channels = channels;
        vc->cpu      = cpumode;

        // reformat all loaded sample callback functions (based on interpolation,
        // cpumode, channels, etc chnges).

        if(vc->mode != mode)
        {   uint    t;
            vc->mode = mode;
            for(t=0; t<vc->samplehandles; t++) configsample(vc, t, vc->mode);
        }

        // Recalculate constants based on new values

        vc->samplesthatfit = TICKLSIZE >> (vc->channels / 2);
    }
    return 0;
}


// =====================================================================================
    void VC_GetMode(VIRTCH *vc, uint *mixspeed, uint *mode, uint *channels, uint *cpumode)
// =====================================================================================
{
    if(vc)
    {   *mixspeed = vc->mixspeed;  *mode    = vc->mode;
        *channels = vc->channels;  *cpumode = vc->cpu;
    } else
    {   *mixspeed = 0;   *mode    = 0;
        *channels = 0;   *cpumode = 0;
    }
}


// =====================================================================================
    void VC_SetVolume(MD_DEVICE *md, const MMVOLUME *volume)
// =====================================================================================
{
    if(md->vc) md->vc->volume = *volume;
}


// =====================================================================================
    void VC_GetVolume(MD_DEVICE *md, MMVOLUME *volume)
// =====================================================================================
{
    if(md->vc) *volume = md->vc->volume;
}


// =====================================================================================
    VIRTCH *VC_Init(void)
// =====================================================================================
{
    VIRTCH    *vc;

    _mmlog("Mikmod > virtch > Entering Initialization Sequence.");
    
    vc  = (VIRTCH *)_mmalloc_create_ex("VIRTCH", NULL, sizeof(VIRTCH));

    vc->volume.front.left  = 
    vc->volume.front.right = 
    vc->volume.rear.left   = 
    vc->volume.rear.right  = 128;
    
    if((vc->sample = _mmobj_array(vc, vc->samplehandles=64, VSAMPLE)) == NULL) return 0;

    if(!vc->TICKBUF)
        if((vc->TICKBUF = _mmobj_array(vc, (TICKLSIZE+32), SLONG)) == NULL) return 0;

    vc->TICKLEFT = vc->TICKREMAIN = 0;
    vc->numchn = 0;

    _mmlog("Mikmod > virtch > Initialization successful!");

#ifndef VC_NO_MIXERS

    VC_RegisterMixer(vc, &RF_M8_MONO_INTERP);
    VC_RegisterMixer(vc, &RF_M16_MONO_INTERP);
    VC_RegisterMixer(vc, &RF_M8_STEREO_INTERP);
    VC_RegisterMixer(vc, &RF_M16_STEREO_INTERP);

#ifdef CPU_INTEL
    VC_RegisterMixer(vc, &ASM_M8_MONO_INTERP);
    VC_RegisterMixer(vc, &ASM_M16_MONO_INTERP);
    VC_RegisterMixer(vc, &ASM_M8_STEREO_INTERP);
    VC_RegisterMixer(vc, &ASM_M16_STEREO_INTERP);

#else
    VC_RegisterMixer(vc, &M8_MONO_INTERP);
    VC_RegisterMixer(vc, &M16_MONO_INTERP);
    VC_RegisterMixer(vc, &M8_STEREO_INTERP);
    VC_RegisterMixer(vc, &M16_STEREO_INTERP);
#endif

#endif VC_NO_MIXERS

    return vc;
}

// =====================================================================================
    void VC_Exit(VIRTCH *vc)
// =====================================================================================
{
    if(!vc) return;
    _mmalloc_close(&vc->allochandle);
}


// =====================================================================================
    BOOL VC_SetSoftVoices(VIRTCH *vc, uint voices)
// =====================================================================================
{
    uint t;

    if(!vc) return 0;

    if(vc->numchn == voices) return 0;
    
    if(!voices)
    {   vc->numchn = 0;
        _mmlogd("Virtch > Deallocating all voices.");
        _mmobj_free(vc, vc->vinf);
        _mmobj_free(vc, vc->vold);
        return 0;
    }

    // reallocate the stuff to the new values.
    
    _mmlogd2("Virtch > Reallocating voices.  Old = %d, New = %d", vc->numchn, voices);

    if((vc->vinf = (VINFO *)_mm_realloc(&vc->allochandle, vc->vinf, sizeof(VINFO) * (voices+1))) == NULL) return 1;
    if(vc->mode & DMODE_NOCLICK)
        if((vc->vold = (VINFO *)_mm_realloc(&vc->allochandle, vc->vold, sizeof(VINFO) * (voices+1))) == NULL) return 1;
    
    for(t=vc->numchn; t<voices+1; t++)
    {   vc->vinf[t].frq          = 10000;
        vc->vinf[t].panflg       = 0;
        vc->vinf[t].kick         = 0;    
        vc->vinf[t].samplehandle = -1;
        vc->vinf[t].onhold       = TRUE;

        memset(&vc->vinf[t].volume,0,sizeof(MMVOLUME));
        memset(&vc->vinf[t].resfilter,0,sizeof(VC_RESFILTER));

        if(vc->mode & DMODE_NOCLICK)
        {   vc->vold[t].kick         = 0;
            vc->vold[t].onhold       = TRUE;
        }
    }

    vc->numchn   = voices;

    vc->clickcnt = 0;

    return 0;
}


// =====================================================================================
    void VC_VoiceSetVolume(MD_DEVICE *md, uint voice, const MMVOLUME *volume)
// =====================================================================================
{
    //vc->vinf[voice].oldvol = vc->vinf[voice].volume;
    //if(vc->vinf[voice].handle) _mmlog("Volume Set: %5d : %4d",voice, volume->front.left);
    md->vc->vinf[voice].volume = *volume;
}

// =====================================================================================
    void VC_VoiceGetVolume(MD_DEVICE *md, uint voice, MMVOLUME *volume)
// =====================================================================================
{
    *volume = md->vc->vinf[voice].volume;
}


// =====================================================================================
    void VC_VoiceSetFrequency(MD_DEVICE *md, uint voice, ulong frq)
// =====================================================================================
{
    md->vc->vinf[voice].frq = frq;
}


// =====================================================================================
    ulong VC_VoiceGetFrequency(MD_DEVICE *md, uint voice)
// =====================================================================================
{
    return md->vc->vinf[voice].frq;
}


// =====================================================================================
    ulong __inline VC_VoiceGetPosition(MD_DEVICE *md, uint voice)
// =====================================================================================
{
    return(md->vc->vinf[voice].current >> FRACBITS);
}


// =====================================================================================
    void VC_VoiceSetPosition(MD_DEVICE *md, uint voice, ulong pos)
// =====================================================================================
{
    if(pos == SF_START_CURRENT)
        pos = VC_VoiceGetPosition(md, voice);
    if(pos == SF_START_BEGIN)
        pos = (md->vc->vinf[voice].flags & SL_REVERSE) ? md->vc->vinf[voice].size : 0;
    else if(pos < 0) pos = 0;

    md->vc->vinf[voice].current = (INT64S)pos << FRACBITS;
}


// =====================================================================================
    void VC_VoicePlay(MD_DEVICE *md, uint voice, uint handle, long start, uint size, int reppos, int repend, int suspos, int susend, uint flags)
// =====================================================================================
{
    VIRTCH *vc = md->vc;
    
    // Declicker: Make sure the old voices will get ramped to nil

    if(vc->mode & DMODE_NOCLICK)
    {
        if(vc->clickcnt<vc->numchn)
        {   vc->vold[vc->clickcnt] = vc->vinf[voice];
            vc->clickcnt++;
        }
    }

    if((handle < vc->samplehandles) && vc->sample[handle].data)
    {
        vc->vinf[voice].samplehandle = handle;
        vc->vinf[voice].onhold       = FALSE;
        vc->vinf[voice].flags    = vc->sample[handle].flags = flags;
        vc->vinf[voice].size     = size;
        vc->vinf[voice].reppos   = reppos;
        vc->vinf[voice].repend   = repend;
        vc->vinf[voice].suspos   = suspos;
        vc->vinf[voice].susend   = susend;

        if(start == SF_START_CURRENT)
            start = 0;
        if(start == SF_START_BEGIN)
            start = (flags & SL_REVERSE) ? size : 0;
        else if(start < 0) start = 0;

        vc->vinf[voice].start = start;

		//zero 8.26.00 
		//i dont see why these would hurt anything. they make sense, if you ask me,
		//and fix some bugs
		vc->vinf[voice].volramp   = 0;
		vc->vinf[voice].current   = 0;
		vc->vinf[voice].increment = 0;
        vc->vinf[voice].kick      = 1;
		//--

        //vc->vinf[voice].resfilter.speed = 0;
        //vc->vinf[voice].resfilter.pos   = 0;

        configsample(vc, handle, vc->mode);
    } else
        vc->vinf[voice].onhold = TRUE;

}

// =====================================================================================
    void VC_VoiceSetResonance(MD_DEVICE *md, uint voice, int cutoff, int resonance)
// =====================================================================================
// By mikmod rules, we expect the cutoff to be range 0 to 16384, and the resonance to
// be range -128 to 128 (where negative values actually de-amplify).
{
    // Build the resonant filter tables here:

    md->vc->vinf[voice].cutoff    = cutoff;
    md->vc->vinf[voice].resonance = resonance;

    //vc->vinf[voice].resfilter.speed = 0;
    //vc->vinf[voice].resfilter.pos   = 0;

}


// =====================================================================================
    void VC_VoiceResume(MD_DEVICE *md, uint voice)
// =====================================================================================
{
    VIRTCH *vc = md->vc;
    
    if((vc->vinf[voice].samplehandle >= 0) && vc->sample[vc->vinf[voice].samplehandle].data)
        vc->vinf[voice].onhold = FALSE;
        //vc->vinf[voice].handle = &vc->sample[vc->vinf[voice].samplehandle];
}


// =====================================================================================
    void VC_VoiceStop(MD_DEVICE *md, uint voice)
// =====================================================================================
{
    VIRTCH  *vc = md->vc;
    
    // Declicker: Make sure the old voices will get ramped to nil

    if(vc->mode & DMODE_NOCLICK)
    {   if(vc->clickcnt<vc->numchn)
        {   vc->vold[vc->clickcnt] = vc->vinf[voice];
            vc->clickcnt++;
        }
    }

    vc->vinf[voice].onhold       = TRUE;
    //vc->vinf[voice].samplehandle = -1;
    //vc->vinf[voice].volramp=0;
	//vc->vinf[voice].current=0;
}


// =====================================================================================
    BOOL VC_VoiceStopped(MD_DEVICE *md, uint voice)
// =====================================================================================
{
    return(md->vc->vinf[voice].onhold);
}


// =====================================================================================
    void VC_VoiceReleaseSustain(MD_DEVICE *md, uint voice)
// =====================================================================================
{
    VIRTCH  *vc = md->vc;
    VINFO   *vnf = &vc->vinf[voice];

    // Release the sustain loop, and make sure the playing continues in
    // the 'right' direction. (matching the direction of the sample)

    if(!vnf->onhold && (vnf->flags & SL_SUSTAIN_LOOP))
    {   vnf->flags &= ~SL_SUSTAIN_LOOP;
        if(vc->sample[vnf->samplehandle].flags & SL_REVERSE)
            vnf->flags |= SL_REVERSE;
        else
            vnf->flags &= ~SL_REVERSE;
    }
}


// =====================================================================================
    void VC_VoiceSetSurround(MD_DEVICE *md, uint voice, int flags)
// =====================================================================================
// This is the best way I can think to implement surround sound.  This is
// surely better than my old method of using a special 'illegal' panning value.
{
    if(md->vc->mode & DMODE_SURROUND) return;

    if(flags & MD_ENABLE_SURROUND)
    {   // reconfigure the voice to 'surround' mode.
        md->vc->vinf[voice].panflg |= VC_SURROUND;
    } else
    {   // reconfigure the voice to normal mode.
        md->vc->vinf[voice].panflg &= VC_SURROUND;
    }
}


/**************************************************
***************************************************
***************************************************
**************************************************/

// =====================================================================================
    int VC_SampleAlloc(MD_DEVICE *md, uint length, uint *flags)
// =====================================================================================
// Allocates memory for a sample of the given size.
// flags is a pointer because it is potentially modified during the allocation
// process to reflect the actual format of the buffer in the driver.
// Returns the handle, or -1 if failed!

{
    VIRTCH  *vc = md->vc;
    int      handle;
    uint     t;

    if((handle = getfreehandle(vc)) == -1) return -1;

    // everything is always signed in our mixer!
    *flags |= SF_SIGNED;
    if(*flags & SF_STEREO) length *= 2;

    if(!(vc->mode & DMODE_SAMPLE_8BIT) && (*flags & SF_16BITS))
    {
        SWORD  *samp16;

        *flags |= SF_16BITS;

        if((vc->sample[handle].data = _mm_malloc(&vc->allochandle, (length+12)<<1))==NULL) return -1;

        samp16 = (SWORD *)vc->sample[handle].data;
        for(t=0; t<8; t++) samp16[t+length] = 0;
    } else
    {
        SBYTE  *samp8;
        *flags &= ~SF_16BITS;
        if((vc->sample[handle].data = _mm_malloc(&vc->allochandle, length+12))==NULL) return -1;

        samp8 = vc->sample[handle].data;
        for(t=0; t<8; t++) samp8[t+length] = 0;
    }

    vc->sample[handle].format = *flags & (SF_16BITS | SF_STEREO);
    if(configsample(vc, handle, vc->mode)) return -1;

    return handle;
}


// =====================================================================================
    void *VC_SampleGetPtr(MD_DEVICE *md, uint handle)
// =====================================================================================
{
    if(!md->vc) return NULL;
    return md->vc->sample[handle].data;
}


// =====================================================================================
    void VC_SampleUnload(MD_DEVICE *md, uint handle)
// =====================================================================================
{
    VIRTCH *vc = md->vc;
    
    if(vc && (handle < vc->samplehandles))
    {   //_mmlog("Unloading Sample %d : %x", handle, vc->sample[handle].data);

vc->clickcnt = 0; /* Prevent declicker from trying to access unloaded samples
			 that have been stopped but not declicked!
		         We really ought to check whether a sample is in use
	 		 before unloading it. */

        _mmobj_free(vc, vc->sample[handle].data);

        if(vc->sample[handle].mixer)
        {   if(vc->sample[handle].mixer->Exit) vc->sample[handle].mixer->Exit(vc->sample[handle].mixer);
            vc->sample[handle].mixer = vc->sample[handle].mixer = &VC_MIXER_PLACEBO;
        }
    }
}


// =====================================================================================
    int VC_SampleLoad(MD_DEVICE *md, SAMPLOAD *sload, int type)
// =====================================================================================
// the driver-optimized 'efficient sample loader.'
// returns a driver sample handle, or -1 if it failed!
// Notes:
//   type is provided for API compliance only, as virtch loads both static
//   and dynamic samples.
{
    int      handle = 0;

    if(md->vc)
    {
        VIRTCH  *vc = md->vc;
        uint     length;
        uint     t;

        if((handle = getfreehandle(vc)) == -1) return -1;

        SL_SampleSigned(sload);

        length = sload->length;
        if(sload->outfmt & SF_STEREO) length *= 2;

        if(!(vc->mode & DMODE_SAMPLE_8BIT) && (sload->outfmt & SF_16BITS))
        {   SWORD  *samp16;

            SL_Sample8to16(sload);
            if((vc->sample[handle].data = _mm_malloc(&vc->allochandle, (length+16)<<1))==NULL) return -1;
            SL_Load(samp16 = (SWORD *)vc->sample[handle].data,sload,sload->length);

            for(t=0; t<8; t++) samp16[t+length] = 0;
        } else
        {   SBYTE  *samp8;
            SL_Sample16to8(sload);
            if((vc->sample[handle].data = _mm_malloc(&vc->allochandle, length+16))==NULL) return -1;
            SL_Load(samp8 = vc->sample[handle].data,sload,sload->length);

            for(t=0; t<8; t++) samp8[t+length] = 0;
        }

        vc->sample[handle].format = sload->outfmt & (SF_STEREO | SF_16BITS);
        if(configsample(vc, handle, vc->mode)) return -1;
    }
    return handle;
}


// =====================================================================================
    ULONG VC_SampleSpace(MD_DEVICE *md, int type)
// =====================================================================================
{
    return md->vc->memory;
}


// =====================================================================================
    ULONG VC_SampleLength(MD_DEVICE *md, int type, SAMPLOAD *s)
// =====================================================================================
{
    return (s->length * ((s->infmt & SF_16BITS) ? 2 : 1)) + 16;
}


/**************************************************
***************************************************
***************************************************
**************************************************/

// =====================================================================================
    ULONG VC_VoiceRealVolume(MD_DEVICE *md, uint voice)
// =====================================================================================
{
/*    ULONG i,s,size;
    int k,j;
#ifdef __FASTMIXER__
    SBYTE *smp;
#else
    SWORD *smp;
#endif
    SLONG t;
                    
    t = vc->vinf[voice].current>>FRACBITS;
    if(vc->vinf[voice].active==0) return 0;

    s    = vc->vinf[voice].handle;
    size = vc->vinf[voice].size;

    i=64; t-=64; k=0; j=0;
    if(i>size) i = size;
    if(t<0) t = 0;
    if(t+i > size) t = size-i;

    i &= ~1;  // make sure it's EVEN.

    smp = &Samples[s][t];
    for(; i; i--, smp++)
    {   if(k<*smp) k = *smp;
        if(j>*smp) j = *smp;
    }

#ifdef __FASTMIXER__
    k = abs(k-j)<<8;
#else
    k = abs(k-j);
#endif

    return k;
    */
    return 0;
}

#ifdef __MM_WINAMP__

// -----------------------------------------
// For Seamless skip forward/backard Stuff.


typedef struct VCBOOM
{   UBYTE   channel;
    VINFO   vc->vinf;
} VCBOOM;

typedef struct VCSTATE
{   UBYTE   numchn;
    VCBOOM  *boom;
} VCSTATE;

// =====================================================================================
    void VC_SaveState(VIRTCH *vc, PMSTATE *pmstate)
// =====================================================================================
{
    int   i, total;
    VCSTATE *vcs;
    VCBOOM  *boom;
    
    for(i=0, total=0; i<vc->numchn; i++)
        if(vc->vinf[i].active && vc->vinf[i].vol) total++;

    if(total == 0)
    {   pmstate->virtch = NULL;
        return;
    }

    vcs  = _mm_malloc(sizeof(VCSTATE));

    vcs->numchn = total;
    boom = vcs->boom = _mm_malloc(sizeof(VCBOOM) * total);

    for(i=0; i<vc->numchn; i++)
    {   if(vc->vinf[i].active && vc->vinf[i].vol)
        {   boom->channel = i;
            memcpy(&boom->vc->vinf,&vc->vinf[i],sizeof(VINFO));
            boom++;
        }
    }

    pmstate->virtch = vcs;
}


void VC_FreeState(PMSTATE *pmstate)
{
    VCSTATE *vcs;

    if(pmstate->virtch == NULL) return;
    vcs = (VCSTATE *)pmstate->virtch;
    _mm_free(vcs->boom);
    _mm_free(pmstate->virtch);
}

#endif

// =====================================================================================
    int VC_GetActiveVoices(MD_DEVICE *md)
// =====================================================================================
{
    VIRTCH  *vc = md->vc;
    uint     vcnt=0, i=0;
    
    for(; i<vc->numchn; i++)
    {   if(!vc->vinf[i].onhold && (vc->vinf[i].vol.front.left || vc->vinf[i].vol.front.right)) vcnt++;
    }

    return vcnt;
}

// =========================
// Per-Sample Interpolation
//
// Ever wanted that magical ability to enable or disable interpolation on a
// per-sample basis?  Don't deny it!  Sure you have!  And here is your big
// chance:  Virtch supports it!

void VC_EnableInterpolation(VIRTCH *vc, int handle)
{
    if(handle != -1) configsample(vc, handle, vc->mode | DMODE_INTERP);
}

void VC_DisableInterpolation(VIRTCH *vc, int handle)
{
    if(handle != -1) configsample(vc, handle, vc->mode & ~DMODE_INTERP);
}



