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
 Module: S3M_IT.C

  Common functions between S3Ms and ITs  (smaller .EXE! :).  Let it be noted
  that I DO take the extra effort to make my programs better and smaller!

 Portability:
  All systems - all compilers (hopefully)

*/

#include "mikmod.h"
#include "itshare.h"

static void WriteVolSlide(UTRK_WRITER *ut, int hi, int lo, int oldeffect);
static void WritePanSlide(UTRK_WRITER *ut, int hi, int lo, int oldeffect);
static void WriteGlobVolSlide(UTRK_WRITER *ut, int hi, int lo);
static void WriteChanVolSlide(UTRK_WRITER *ut, int hi, int lo);

// IT / S3M Extended SS effects:

enum
{   SS_GLISSANDO = 1,
    SS_FINETUNE,
    SS_VIBWAVE,
    SS_TREMWAVE,
    SS_PANWAVE,
    SS_FRAMEDELAY,
    SS_S7EFFECTS,
    SS_PANNING,
    SS_SURROUND,
    SS_HIOFFSET,
    SS_PATLOOP,
    SS_NOTECUT,
    SS_NOTEDELAY,
    SS_PATDELAY
};

// =====================================================================================
    uint S3MIT_ProcessCmd(UTRK_WRITER *ut, UBYTE *poslookup, uint cmd, uint inf, BOOL oldeffect, int gxxmem)
// =====================================================================================
{

/* oldeffect bits have the following meanings (bit value - meaning):
   1	- IT "old effects" mode (i.e. optional S3M compatibility bug fix mode).
   2	- S3M file, not IT.
   4	- IT "compatible Gxx" mode (i.e. optional XM compatibility bug fix mode).
   8	- Saved by Scream Tracker 3, not by IT (i.e. has some bugs Lim never noticed).
   16	- S3M fast volume slide mode (required for ST3.00 bug compatibility).

   What a mess. */

    UNITRK_EFFECT effdat;
    UBYTE         hi, lo;
    uint          retval = 0;

    lo = inf & 0xf;
    hi = inf >> 4;

    // process S3M / IT specific command structure

    effdat.framedly       = 0;
    effdat.param.u        = 0;

    if(cmd!=255)
    {   switch(cmd)
        {   case 1:                 // Axx set speed to xx
               if(inf)
               {   effdat.effect   = UNI_GLOB_SPEED;
                   effdat.framedly = UFD_RUNONCE;
                   effdat.param.u  = inf;
                   utrk_write_global(ut, &effdat, STMEM_SPEED);
               } else utrk_memory_global(ut, &effdat, STMEM_SPEED);
            break;

            case 2:                 // Bxx position jump
                pt_write_effect(ut,0xb, poslookup ? poslookup[inf] : inf);
            break;

            case 3:                 // Cxx patternbreak to row xx
                if(oldeffect & 2) /* Lim got this one right! Incredible! */
                    pt_write_effect(ut,0xd, (((inf & 0xf0) >> 4) * 10) + (inf & 0xf));
                else
                    pt_write_effect(ut,0xd, inf);
            break;

            case 4:                 // Dxy volumeslide
                WriteVolSlide(ut,hi,lo,oldeffect);
            break;

            case 5:                 // Exy toneslide down
            case 6:                 // Fxy toneslide up
                effdat.effect   = UNI_PITCHSLIDE;
                if(hi >= 0xe)
                    effdat.framedly = UFD_RUNONCE;
                else
                    effdat.framedly = 1;

                if(inf)
                {   effdat.param.s = (hi == 0xe) ? lo*2 : (hi == 0xf) ? lo*8 : inf*8;
                    if(cmd == 5) effdat.param.s = -effdat.param.s;
                    utrk_write_local(ut, &effdat, STMEM_PITCHSLIDE);
                } else utrk_memory_local(ut, &effdat, STMEM_PITCHSLIDE, (cmd == 5) ? -1 : 1);
            break;

            case 7:                 // Gxx Tone portamento, speed xx
                if(oldeffect & 4) {
		    if (oldeffect & 8)
                    	pt_write_effect(ut,0x3,inf); /* S3M. */
		    else {
			/* Impulse Tracker sucks, reason #3276:
			   IT always retriggers notes on portamento if no
			   note is currently playing (irrespective of
			   old effects, compatible Gxx, whatever), which
			   no other tracker does (PT doesn't, FT2 doesn't,
			   ST3 doesn't; believe me, I checked).

			   We need an effect that retriggers notes like
			   the IT-style portamento, but otherwise behaves
			   ST3-style (egad!).
			*/
	                effdat.effect   = UNI_PORTAMENTO_BUGGY;
	                if(inf)
	                {   effdat.param.u  = inf*8;
	                    utrk_write_local(ut, &effdat, PTMEM_PORTAMENTO);
	                } else utrk_memory_local(ut, &effdat, PTMEM_PORTAMENTO, 0);
		    }
                }
                else
                {   // ImpulseTracker Sucks, Reason #2577:
                    // Lim decides it's a good idea for Exx/Fxx AND Gxx to share memory
                    // because they both have to do with pitch, I guess.

                    effdat.effect   = UNI_PORTAMENTO;
                    if(inf)
                    {   effdat.param.s  = inf<<3;
                        utrk_write_local(ut, &effdat, gxxmem);
                    } else
                        utrk_memory_local(ut, &effdat, gxxmem, (gxxmem == PTMEM_PORTAMENTO) ? 0 : 1);
                } 
            break;

            case 8:                 // Hxy vibrato
                // ImpulseTracker Sucks, Reason #1233:
                // The original version of IT was buggy.. Vibratos vibrated too shallowly.
                // (much like Lim's intellect?)  So he makes it an option.  uGH.
                if(oldeffect & 1)
                    pt_write_effect(ut,0x4, inf);
                else
                {   if(lo)
                    {   effdat.effect  = UNI_VIBRATO_DEPTH;
                        effdat.param.u = lo*8;
                        utrk_write_local(ut, &effdat, PTMEM_VIBRATO_DEPTH);
                    } else utrk_memory_local(ut, NULL, PTMEM_VIBRATO_DEPTH, 0);

                    if(hi)
                    {   effdat.effect  = UNI_VIBRATO_SPEED;
                        effdat.param.u = hi*4;
                        utrk_write_local(ut, &effdat, PTMEM_VIBRATO_SPEED);
                    } else utrk_memory_local(ut, NULL, PTMEM_VIBRATO_SPEED, 0);
                }
            break;

            case 9:                 // Ixy tremor, ontime x, offtime y
                if(inf)
                {   effdat.effect   = UNI_TREMOR;
                    effdat.framedly = 0;
                    if(oldeffect & 1)
                    {   // should this be masked or not? Will we ever know?  Will it ever matter?
                        effdat.param.loword.u = (lo+1)&0xf;
                        effdat.param.hiword.u = (hi+1)&0xf;
                    } else
                    {   effdat.param.loword.u = lo;
                        effdat.param.hiword.u = hi;
                    }
                    utrk_write_local(ut, &effdat, STMEM_TREMOR);
                } else utrk_memory_local(ut, &effdat, STMEM_TREMOR, 0);
            break;

            case 0xa:               // Jxy arpeggio
                if(inf)
                {   effdat.param.byte_a = lo;
                    effdat.param.byte_b = hi;
                    effdat.effect  = UNI_ARPEGGIO;
                    utrk_write_local(ut, &effdat, STMEM_ARPEGGIO);
                } else utrk_memory_local(ut, NULL, STMEM_ARPEGGIO, 0);
            break;

            case 0xb:               // Kxy Dual command H00 & Dxy
                utrk_memory_local(ut, NULL, PTMEM_VIBRATO_DEPTH, 0);
                utrk_memory_local(ut, NULL, PTMEM_VIBRATO_SPEED, 0);
                WriteVolSlide(ut,hi,lo, oldeffect);
            break;

            case 0xc:               // Lxy Dual command G00 & Dxy
                if(gxxmem == PTMEM_PORTAMENTO)
                {   effdat.effect   = UNI_PORTAMENTO_LEGACY;
                    utrk_memory_local(ut, &effdat, gxxmem, 0);
                } else
                {   effdat.effect   = UNI_PORTAMENTO;
                    utrk_memory_local(ut, &effdat, gxxmem, 1);
                }
                WriteVolSlide(ut,hi,lo,oldeffect);
            break;

            case 0xd:               // Mxx Set Channel Volume
               effdat.param.u  = (inf>0x40) ? 0x40 : inf;
               effdat.effect   = UNI_CHANVOLUME;
               effdat.framedly = UFD_RUNONCE;
               utrk_write_local(ut, &effdat, UNIMEM_NONE);
            break;       

            case 0xe:               // Nxy Slide Channel Volume
                WriteChanVolSlide(ut,hi,lo);
            break;

            case 0xf:               // Oxx set sampleoffset xx00h
		if (inf) {
                  effdat.effect         = UNI_OFFSET_LEGACY;
                  effdat.param.loword.u = inf<<8;
		  if (!(oldeffect & 1)) effdat.param.hiword.u=0x2000;
			/* Must ignore offset commands that go past
			   end of sample when old effects are off. */
			   
                  effdat.framedly       = UFD_RUNONCE;
                  utrk_write_local(ut, &effdat, PTMEM_OFFSET);
		}
		else utrk_memory_local(ut,NULL,PTMEM_OFFSET,0);
            break;

            case 0x10:              // Pxy Slide Panning Commands
                WritePanSlide(ut,hi,lo,oldeffect);
            break;

            case 0x11:              // Qxy Retrig (+volumeslide)
                if(inf)
                {   effdat.param.loword.u = lo;
                    effdat.param.hiword.u = hi;
                    effdat.effect   = UNI_RETRIG;
                    effdat.framedly = 0;
                    utrk_write_local(ut, &effdat, STMEM_RETRIG);
                } else utrk_memory_local(ut, &effdat, STMEM_RETRIG, 0);
            break;

            case 0x12:              // Rxy tremolo speed x, depth y
                if(lo)
                {   effdat.param.u   = lo * 4;
                    effdat.effect    = UNI_TREMOLO_DEPTH;
                    utrk_write_local(ut, &effdat, PTMEM_TREMOLO_DEPTH);
                } else utrk_memory_local(ut, NULL, PTMEM_TREMOLO_DEPTH, 0);

                if(hi)
                {   effdat.param.u   = hi * 4;
                    effdat.effect    = UNI_TREMOLO_SPEED;
                    effdat.framedly  = (oldeffect & 1) ? 1 : 0;
                    utrk_write_local(ut, &effdat, PTMEM_TREMOLO_SPEED);
                } else utrk_memory_local(ut, NULL, PTMEM_TREMOLO_SPEED, 0);
            break;

            case 0x13:              // Sxx special commands
                if(inf)
                {   BOOL globeffect = 0;

                    effdat.framedly = UFD_RUNONCE;
                    switch(hi)
                    {   /*case SS_GLISSANDO:      // S1x set glissando voice
                        break;

                        case SS_FINETUNE:       // S2x set finetune
                        break;*/ 

                        case SS_VIBWAVE:        // S3x set vibrato waveform
                            effdat.param.u  = lo;
                            effdat.effect   = UNI_VIBRATO_WAVEFORM;
                        break;
    
                        case SS_TREMWAVE:       // S4x set tremolo waveform
                            effdat.param.u  = lo;
                            effdat.effect   = UNI_TREMOLO_WAVEFORM;
                        break;
    
                        case SS_PANWAVE:        // The Satanic Panbrello waveform
                            effdat.param.u  = lo;
                            effdat.effect   = UNI_PANBRELLO_WAVEFORM;
                        break;
    
                        case SS_FRAMEDELAY:     // S6x Delay x number of frames (patdly)
                            effdat.effect          = UNI_GLOB_DELAY;
                            effdat.param.loword.u  = lo;
                            globeffect = 1;
                        break;
    
                        case SS_S7EFFECTS:      // ImpulseTracker NNA control effects
                            switch(lo)
                            {   case 0:         // S70 Past note cut
                                    effdat.param.u  = NNA_CUT;
                                    effdat.effect   = UNI_NNA_CHILDREN;
                                break;

                                case 1:         // S71 Past note off
                                    effdat.param.u  = NNA_OFF;
                                    effdat.effect   = UNI_NNA_CHILDREN;
                                break;

                                case 2:         // S72 Past note fade
                                    effdat.param.u  = NNA_FADE;
                                    effdat.effect   = UNI_NNA_CHILDREN;
                                break;

                                // Change NNA behavior (for current note only)
                                case 3:         // S73 Set to cut
                                case 4:         // S74 Set to continue
                                case 5:         // S74 Set to off
                                case 6:         // S74 Set to fade
                                    effdat.param.u  = lo - 3;
                                    effdat.effect   = UNI_NNA_CONTROL;
                                break;

                                case 8:         // S78 Volume Envelope on
                                    effdat.param.byte_b = TRUE;

                                case 7:         // S77 Volume Envelope off
                                    effdat.effect = UNI_ENVELOPE_CONTROL;
                                break;

                                case 0xa:       // S7a Panning Envelope on
                                    effdat.param.byte_b = TRUE;

                                case 9:         // S79 Panning Envelope off
                                    effdat.effect       = UNI_ENVELOPE_CONTROL;
                                    effdat.param.byte_a = 1;
                                break;

                                case 0xc:       // S7c Pitch Envelope on
                                    effdat.param.byte_b = TRUE;

                                case 0xb:       // S7b Pitch Envelope off
                                    effdat.effect       = UNI_ENVELOPE_CONTROL;
                                    effdat.param.byte_a = 2;
                                break;
                            }
                        break;

                        case SS_PANNING:        // S8x Set Panning
                            effdat.param.s  = ((lo<=8) ? (lo*16) : (lo*17)) + PAN_LEFT;
                            effdat.effect   = UNI_PANNING;
                        break;

                        case SS_SURROUND:       // Set Surround Sound
                            effdat.param.s  = PAN_SURROUND;
                            effdat.effect   = UNI_PANNING;
                        break;

                        case SS_HIOFFSET:       // set the upper bits of the sample offset.
                            effdat.param.hiword.u = lo|0x1000; /* Flag to indicate hi word. */
                            effdat.effect         = UNI_OFFSET_LEGACY;
                        break;

                        case SS_PATLOOP:        // SBx pattern loop
                            effdat.param.u  = lo;
                            effdat.effect   = lo ? UNI_GLOB_LOOP : UNI_GLOB_LOOPSET;
                            globeffect      = 1;
                        break;

                        case SS_NOTECUT:        // SCx Note Cut
                            if(lo==0) lo++;     // make sure we never run on 0
                            effdat.effect    = UNI_NOTEKILL;
                            effdat.framedly |= lo;
                        break;

                        case SS_NOTEDELAY:      // Sdx Note Delay
                            effdat.param.u  = lo;
                            effdat.effect   = UNI_NOTEDELAY;
                            effdat.framedly = 0;
                            retval = lo;
                        break;

                        case SS_PATDELAY:
                            effdat.param.hiword.u = lo;
                            effdat.effect         = UNI_GLOB_DELAY;
                            globeffect = 1;
                        break;
                    }                    
                    if(globeffect)
                        utrk_write_global(ut, &effdat, STMEM_SEFFECT);
                    else
                        utrk_write_local(ut, &effdat, STMEM_SEFFECT);
                } else utrk_memory_local(ut, NULL, STMEM_SEFFECT, 0);
            break;

            case 0x14:              // Txx tempo
                if(inf)
                {   if(inf < 0x20)
                    {   effdat.effect   = UNI_GLOB_TEMPOSLIDE;
                        effdat.param.s  = (inf < 0x10) ? (0 - inf) : (inf-0x10);
                        effdat.framedly = 1;
                        utrk_write_global(ut, &effdat, STMEM_TEMPO);
                    } else 
                    {   effdat.effect   = UNI_GLOB_TEMPO;
                        effdat.param.u  = inf;
                        effdat.framedly = UFD_RUNONCE;
                        utrk_write_global(ut, &effdat, STMEM_TEMPO);
                    }
                } else utrk_memory_global(ut, &effdat, STMEM_TEMPO);
            break;

            case 0x15:      // Uxy Fine Vibrato speed x, depth y
                // Impulse Tracker sucks, Reason #1234:
                //  Lim messed up fine vibratos too, of course (see reason # 1233 above).
                if(lo)
                {   effdat.param.u = (oldeffect & 1) ? (lo * 4) : (lo * 2);
                    effdat.effect  = UNI_VIBRATO_DEPTH;
                    utrk_write_local(ut, &effdat, PTMEM_VIBRATO_DEPTH);
                } else utrk_memory_local(ut, NULL, PTMEM_VIBRATO_DEPTH, 0);


                if(hi)
                {   if(oldeffect & 1) effdat.framedly = 1;
                    effdat.param.u = hi*4;
                    effdat.effect  = UNI_VIBRATO_SPEED;
                    utrk_write_local(ut, &effdat, PTMEM_VIBRATO_SPEED);
                } else utrk_memory_local(ut, NULL, PTMEM_VIBRATO_SPEED, 0);
            break;

            case 0x16:      // Vxx Set Global Volume

                // Note: The volume should be ignored if it is above
                // 0x80, *not* set to 0x80!
                
                if(inf <= 0x80)
                {   effdat.framedly = UFD_RUNONCE;
                    effdat.param.u  = inf;
                    effdat.effect   = UNI_GLOB_VOLUME;
                    utrk_write_global(ut, &effdat, UNIMEM_NONE);
                }
            break;

            case 0x17:      // Wxy Global Volume Slide
                WriteGlobVolSlide(ut,hi,lo);
            break;

            case 0x18:      // Xxx amiga command 8xx
                // S3Ms were range 0 to 80h?
                // ImpulseTracker Sucks, Reason #592:
                //  Lim explains how S3M Panning is range 0-80h, and how IT is 0-ffh,
                //  but he forgets to implement 0-80h in his 'oldeffects' mode.  Not
                //  only was it confusing to me, but any S3Ms saved with IT are not going
                //  to pan properly.  What a total dweeb.

		/* Huh? _My_ copy of Impulse Tracker both loads and saves Xxx commands
		   in S3M correctly - it just multiplies by 2 when loading S3Ms and
		   divides when saving S3Ms. Old effects mode has nothing to do with it.

		   I don't see anything about fixing this in the IT change log, either,
		   meaning that Lim either managed to confuse Jake, or fixed it without
		   telling anyone. Given the clarity of Lim's documentation, I'd choose
		   the former. */

                pt_write_effect(ut,0x8,(oldeffect & 2) ? (inf * 2) : inf);
            break;

            case 0x19:      // Yxy Panbrello  speed x, depth y
                if(lo)
                {   effdat.param.u  = lo;
                    effdat.effect   = UNI_PANBRELLO_DEPTH;
                    utrk_write_local(ut, &effdat, STMEM_PANBRELLO_DEPTH);
                } else utrk_memory_local(ut, &effdat, STMEM_PANBRELLO_DEPTH, 0);

                if(hi)
                {   effdat.param.u  = hi;
                    effdat.effect   = UNI_PANBRELLO_SPEED;
                    utrk_write_global(ut, &effdat, STMEM_PANBRELLO_SPEED);
                } else utrk_memory_local(ut, &effdat, STMEM_PANBRELLO_SPEED, 0);
            break;

            case 0x1a:      // Zxx - MIDI Macro (resonance/cutoff)
                // 0x00 - 0x7F is cutoff
                // 0x80 - 0x8F is resonance

                effdat.framedly = UFD_RUNONCE;
                if(inf < 0x80)
                {   effdat.effect   = UNI_FILTER_CUTOFF;
                    effdat.param.s  = (inf == 0x7f) ? 128 : (inf);
                } else if(inf < 0x90)
                {   effdat.effect   = UNI_FILTER_RESONANCE;
                    effdat.param.u  = ((inf-0x80) * ((inf < 0x88) ? 256 : 270)) >> 5;
                }
                utrk_write_local(ut, &effdat, UNIMEM_NONE);
            break;
       }
   }
   return retval;
}

// Volume slides need to be split into their specialized effects categories.


// =====================================================================================
    static BOOL SuperDuperSliderMan(UNITRK_EFFECT *eff, int hi, int lo, int oldeffect)
// =====================================================================================
// Used by all the s3m volume slider commands to properly assign eff.param and
// eff.framedly based on the hi/lo prarameters passed to it.  Returns true if
// the effect is valid or false if it should be ignored.
{
    
    if(hi == 0)
    {   eff->param.s  = 0 - lo;
        eff->framedly = (!(oldeffect & 16) && (lo != 0xf)) ? 1 : 0;
    } else if(lo == 0)
    {   eff->param.s  = hi;
        eff->framedly = (!(oldeffect & 16) && (hi != 0xf)) ? 1 : 0;
    } else if(hi == 0xf)
    {   eff->param.s  = 0 - lo;
        eff->framedly = UFD_RUNONCE;
    } else if(lo == 0xf)
    {   eff->param.s  = hi;
        eff->framedly = UFD_RUNONCE;
    } else return 0;

    return 1;
}


// =====================================================================================
    static BOOL SuperSliderMan(UNITRK_EFFECT *eff, int hi, int lo)
// =====================================================================================
{
    if(hi == 0)
    {   eff->param.s  = 0 - lo;
        eff->framedly = 1;
    } else if(lo == 0)
    {   eff->param.s  = hi;
        eff->framedly = 1;
    } else if(hi == 0xf)
    {   eff->param.s  = 0 - lo;
        eff->framedly = UFD_RUNONCE;
    } else if(lo == 0xf)
    {   eff->param.s  = hi;
        eff->framedly = UFD_RUNONCE;
    } else return 0;

    return 1;
}


// =====================================================================================
    static void WriteVolSlide(UTRK_WRITER *ut, int hi, int lo, int oldeffect)
// =====================================================================================
{
    if(hi || lo)
    {   UNITRK_EFFECT  eff;
        if(SuperDuperSliderMan(&eff, hi, lo, oldeffect))
        {   eff.effect   = UNI_VOLSLIDE;
            eff.param.s *= 2;
            utrk_write_local(ut, &eff, STMEM_VOLSLIDE);
        }
    } else utrk_memory_local(ut, NULL, STMEM_VOLSLIDE, 0);
}

// =====================================================================================
    static void WritePanSlide(UTRK_WRITER *ut, int hi, int lo, int oldeffect)
// =====================================================================================
// Split this sucker up into a set of specialized commands.
{
    if(hi || lo)
    {   UNITRK_EFFECT  eff;

        if(SuperDuperSliderMan(&eff, lo, hi, oldeffect))
        {   eff.effect   = UNI_PANSLIDE;
            eff.param.s *= 4;    // adust from 0->64 to -128->128 scale.
            utrk_write_local(ut, &eff, STMEM_PANSLIDE);
        }
    } else utrk_memory_local(ut, NULL, STMEM_PANSLIDE, 0);
}

// ImpulseTracker Sucks, Reason #3662:
//   Lim decides to treat these effects the same, whether oldeffects is on or off,
//   since these were not effects available in ST3, destroying any semblance of
//   consistancy, and not allowing for the S3M format to ever be properly expanded
//   upon in the future.  Sigh.

/* ModPlug Tracker sucks, reason 0x00:
   While Lim at least provides a way for the poor player to detect his twisted
   version of S3M, Olivier Lapicque's ModPlug Tracker insists on saving exactly the
   same version identifier as ScreamTracker 3.20. This makes it impossible to tell
   the difference between these two, so we _have to_ interpret these effects, or
   risk alienating ModPlug users. 

   Jake, maybe we should add Olivier Lapicque to our list of nasty people who have
   polluted the S3M format. Of course, it could all be Jeffery Lim's fault, so
   I'll give Lapicque the benefit of the doubt. For now. */

// =====================================================================================
    static void WriteChanVolSlide(UTRK_WRITER *ut, int hi, int lo)
// =====================================================================================
{
    if(hi || lo)
    {   UNITRK_EFFECT  eff;
        if(SuperSliderMan(&eff, hi, lo))
        {   eff.effect   = UNI_CHANVOLSLIDE;
            utrk_write_local(ut, &eff, STMEM_CHANVOLSLIDE);
        }
    } else utrk_memory_local(ut, NULL, STMEM_CHANVOLSLIDE, 0);
}


// =====================================================================================
    static void WriteGlobVolSlide(UTRK_WRITER *ut, int hi, int lo)
// =====================================================================================
{
    if(hi || lo)
    {   UNITRK_EFFECT  eff;
        if(SuperSliderMan(&eff, hi,lo))
        {   eff.effect   = UNI_GLOB_VOLSLIDE;
            utrk_write_global(ut, &eff, STMEM_GLOBVOLSLIDE);
        }
    } else utrk_memory_global(ut, NULL, STMEM_GLOBVOLSLIDE);
}


