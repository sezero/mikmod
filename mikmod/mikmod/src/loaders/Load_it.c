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
 Module: LOAD_IT.C

  ImpulseTracker (IT) module loader.  Created by Jeffrey Lim, who, unfor-
  tunately, had a lack of knowledge and experience when it comes to making
  good file-formats.

 Portability:
  All systems - all compilers (hopefully)
*/

#include <string.h>
#include "mikmod.h"
#include "itshare.h"

#define IT_PANMUL  (PAN_RIGHT/32)

/**************************************************************************
**************************************************************************/


// =====================================================================================
    typedef struct ITHEADER
// =====================================================================================
// Raw IT header struct:
{   
    CHAR      songname[26];
    UBYTE     blank01[2];
    UWORD     ordnum;
    UWORD     insnum;
    UWORD     smpnum;
    UWORD     patnum;
    UWORD     cwt;          // Created with tracker (y.xx = 0x0yxx)
    UWORD     cmwt;         // Compatable with tracker ver > than val.
    UWORD     flags;
    UWORD     special;      // bit 0 set = song message attached
    UBYTE     globvol;
    UBYTE     mixvol;       // mixing volume [ignored]
    UBYTE     initspeed;
    UBYTE     inittempo;
    UBYTE     pansep;       // panning separation between channels
    UBYTE     zerobyte;       
    UWORD     msglength;
    ULONG     msgoffset;
    UBYTE     blank02[4];
 
    UBYTE     pantable[64];
    UBYTE     voltable[64];
} ITHEADER;



// =====================================================================================
    typedef struct ITSAMPLE
// =====================================================================================
// Raw IT sampleinfo struct:
{   
    CHAR  filename[12];
    UBYTE zerobyte;
    UBYTE globvol;
    UBYTE flag;
    UBYTE volume;
    UBYTE panning;
    CHAR  sampname[28];
    UWORD convert;        // sample conversion flag
    ULONG length;
    ULONG loopbeg;
    ULONG loopend;
    ULONG c5spd;
    ULONG susbegin;
    ULONG susend;
    ULONG sampoffset;
    UBYTE vibspeed;
    UBYTE vibdepth;
    UBYTE vibrate;
    UBYTE vibwave;    // 0 = sine; 1 = rampdown; 2 = square; 3 = random (speed ignored)
} ITSAMPLE;


// =====================================================================================
    typedef struct ITINSTHEADER
// =====================================================================================
{   
    ULONG size;             // (dword) Instrument size
    CHAR  filename[12];     // (char) Instrument filename
    UBYTE zerobyte;         // (byte) Instrument type (always 0)
    UBYTE volflg;
    UBYTE volpts;   
    UBYTE volbeg;           // (byte) Volume loop start (node)
    UBYTE volend;           // (byte) Volume loop end (node)
    UBYTE volsusbeg;        // (byte) Volume sustain begin (node)
    UBYTE volsusend;        // (byte) Volume Sustain end (node)
    UBYTE panflg;
    UBYTE panpts;  
    UBYTE panbeg;           // (byte) channel loop start (node)
    UBYTE panend;           // (byte) channel loop end (node)
    UBYTE pansusbeg;        // (byte) cahnnel sustain begin (node)
    UBYTE pansusend;        // (byte) channel Sustain end (node)
    UBYTE pitflg;
    UBYTE pitpts;   
    UBYTE pitbeg;           // (byte) pitch loop start (node)
    UBYTE pitend;           // (byte) pitch loop end (node)
    UBYTE pitsusbeg;        // (byte) pitch sustain begin (node)
    UBYTE pitsusend;        // (byte) pitch Sustain end (node)
    UWORD blank;
    UBYTE globvol;
    UBYTE chanpan;
    UWORD fadeout;          // Envelope end / NNA volume fadeout
    UBYTE dnc;              // Duplicate note check
    UBYTE dca;              // Duplicate check action
    UBYTE dct;              // Duplicate check type
    UBYTE nna;              // New Note Action [0,1,2,3]
    UWORD trkvers;          // tracker version used to save [in files only]
    UBYTE ppsep;            // Pitch-pan Separation
    UBYTE ppcenter;         // Pitch-pan Center
    UBYTE rvolvar;          // random volume varations
    UBYTE rpanvar;          // random panning varations
    UWORD numsmp;           // Number of samples in instrument [in files only]
    CHAR  name[26];         // Instrument name
    UBYTE cutoff;           // Cutoff frequency for resonant filters
    UBYTE resonance;        // Resonance value for resonant filters.
    UBYTE blank01[6];
    UWORD samptable[120];   // sample for each note [note / samp pairs]

    UBYTE volenv[200];      // volume envelope (IT 1.x stuff)
    UBYTE oldvoltick[25];   // volume tick position (IT 1.x stuff)
    UBYTE volnode[25];      // aplitude of volume nodes
    UWORD voltick[25];      // tick value of volume nodes
    SBYTE pannode[25];      // panenv - node points
    UWORD pantick[25];      // tick value of panning nodes
    SBYTE pitnode[25];      // pitchenv - node points
    UWORD pittick[25];      // tick value of pitch nodes
} ITINSTHEADER;                       


typedef struct _ITNOTE
{   UBYTE note,ins,volpan,cmd,inf;
} ITNOTE;

/**************************************************************************
**************************************************************************/

static int       old_effect;      // if set, use S3M old-effects stuffs
   
static CHAR IT_Version[] = "ImpulseTracker x.xx";

// =====================================================================================
    typedef struct _IT_HANDLE
// =====================================================================================
{   
    MM_ALLOC    *allochandle;
    UTRK_WRITER *ut;
    ULONG       *paraptr;         // parapointer array (see IT docs)
    UBYTE        mask[64];
    ITHEADER     mh;
    SBYTE        remap[64];       // for removing empty channels
    UBYTE        poslookup[256];  // S3M/IT fix - removing blank patterns needs a
                                  // lookup table to fix position-jump commands
    ITNOTE       last[64];

} IT_HANDLE;


// =====================================================================================
    BOOL IT_Test(MMSTREAM *mmfile)
// =====================================================================================
{
    UBYTE id[4];
    
    if(!_mm_read_UBYTES(id,4,mmfile)) return 0;
    if(!memcmp(id,"IMPM",4)) return 1;

    return 0;
}


// =====================================================================================
    ML_HANDLE *IT_Init(void)
// =====================================================================================
{
    IT_HANDLE  *handle;
    MM_ALLOC   *allochandle;

    allochandle = _mmalloc_create("Load_IT", NULL);
    handle = (IT_HANDLE *)_mm_calloc(allochandle, 1, sizeof(IT_HANDLE));
    handle->allochandle = allochandle;

    return handle;
}


// =====================================================================================
    void IT_Cleanup(IT_HANDLE *handle)
// =====================================================================================
{
    if(!handle) return;
    
    _mmalloc_close(handle->allochandle); 
}

static CHAR *iterr_eof_patdat = "load_it > Failure: Unexpected end of file reading pattern data";
static CHAR *iterr_eof_header = "load_it > Failure: Unexpected end of file reading module header";

// =====================================================================================
    BOOL IT_GetNumChannels(MMSTREAM *mmfile, UBYTE *mask, SBYTE *remap, UWORD patrows)
// =====================================================================================
// Because so many IT files have 64 channels as the set number used, but really
// only use far less (usually 8 to 12 still), I had to make this function,
// which determines the number of channels that are actually USED by a pattern.
//
// For every channel that's used, it sets the appropriate array entry of the
// global variable 'isused'
//
// NOTE: You must first seek to the file location of the pattern before calling
//       this procedure.
// Returns 1 on error
{
    int  row = 0,flag,ch;

    do
    {   flag = _mm_read_UBYTE(mmfile);
        if(flag == EOF)
        {   _mmlog(iterr_eof_patdat);
            return 1;
        }

        if(flag == 0)
        {   row++;
        } else
        {   ch = (flag-1) & 63;
            remap[ch] = 0;
            if(flag & 128) mask[ch] = _mm_read_UBYTE(mmfile);
            if(mask[ch] & 1) _mm_read_UBYTE(mmfile);
            if(mask[ch] & 2) _mm_read_UBYTE(mmfile);
            if(mask[ch] & 4) _mm_read_UBYTE(mmfile);
            if(mask[ch] & 8) { _mm_read_UBYTE(mmfile); _mm_read_UBYTE(mmfile); }
        }
    } while(row < patrows);

    return 0;
}


// table for porta-to-note command within volume/panning column
static UBYTE   portatable[] = { 0, 1, 4, 8, 16, 32, 64, 96, 128, 255 };

// =====================================================================================
    BOOL IT_ReadPattern(MMSTREAM *mmfile, IT_HANDLE *h, UWORD patrows, int gxx_memory)
// =====================================================================================
{
    int      row=0,flag,ch;
    ITNOTE  *l, dummy;
    uint     note, ins, volpan, cmd, inf;

    utrk_reset(h->ut);

    do
    {   flag = _mm_read_UBYTE(mmfile);
        if(_mm_feof(mmfile))
        {   _mmlog(iterr_eof_patdat);
            return 0;
        }

        if(flag == 0)
        {   row++;
            utrk_newline(h->ut);
        } else
        {   ch = h->remap[(flag-1) & 63];
            note = ins = volpan = cmd = inf = 255;
            l = (ch != -1) ? &h->last[ch] : &dummy;

            if(flag & 128) h->mask[ch] = _mm_read_UBYTE(mmfile);
            if(h->mask[ch] &   1) if((l->note = note   = _mm_read_UBYTE(mmfile)) == 255)
                                  {   l->note = note   = 253; }
            if(h->mask[ch] &   2) l->ins      = ins    = _mm_read_UBYTE(mmfile);
            if(h->mask[ch] &   4) l->volpan   = volpan = _mm_read_UBYTE(mmfile);
            if(h->mask[ch] &   8) { l->cmd    = cmd    = _mm_read_UBYTE(mmfile);
                                    l->inf    = inf    = _mm_read_UBYTE(mmfile); }
            if(h->mask[ch] &  16) note     = l->note;
            if(h->mask[ch] &  32) ins      = l->ins;
            if(h->mask[ch] &  64) volpan   = l->volpan;
            if(h->mask[ch] & 128) { cmd    = l->cmd;
                                    inf    = l->inf; }

            if(ch != -1)
            {   UNITRK_EFFECT  eff;

                utrk_settrack(h->ut, ch);
                eff.param.u = eff.effect = 0;
                eff.framedly = UFD_RUNONCE;
                if(note!=255)
                {   if(note==253)
                    {   eff.effect = UNI_KEYFADE;
                        utrk_write_local(h->ut, &eff, UNIMEM_NONE);
                    } else if(note==254)
                    {   eff.effect = UNI_NOTEKILL;
                        utrk_write_local(h->ut, &eff, UNIMEM_NONE);
                    } else utrk_write_note(h->ut, note+1);
                }

                if((ins != 0) && (ins < 100)) utrk_write_inst(h->ut, ins);

                eff.framedly = S3MIT_ProcessCmd(h->ut, h->poslookup, cmd, inf, old_effect, gxx_memory);
                eff.framedly = 0;

                //if(eff.framedly)
                    //_mmlog("Checkme!");

                // process volume / panning column
                //   A/B/C/D share memory, as does E/F (and G if that flag is set...)
                // ImpulseTracker Sucks, Reason #371:
                //   they also share the same memory with the std effects, except
                //   these are all separate effects, which means I can't assume any-
                //   thing about the STMEM_VOLSLIDE memory slot (effect, framedly, 
                //   and sign must all be specified). Sigh!

                // At least there is no panning slide volumn column effect...

                if(volpan<=64)
                {   eff.param.u   = volpan*2;                     // Set Volume
                    eff.effect    = UNI_VOLUME;
                    eff.framedly |= UFD_RUNONCE;
                    utrk_write_local(h->ut, &eff, UNIMEM_NONE);
                } else if((volpan>=65) && (volpan<=74))        // fine volume slide up (65-74)
                {   eff.param.s   = (volpan-65)*2;
                    eff.effect    = UNI_VOLSLIDE;
                    eff.framedly |= UFD_RUNONCE;
                    if(eff.param.s)
                        utrk_write_local(h->ut, &eff, STMEM_VOLSLIDE);
                    else
                        utrk_memory_local(h->ut, &eff, STMEM_VOLSLIDE, 1);
                } else if((volpan>=75) && (volpan<=84))        // fine volume slide down   (75-84)
                {   eff.param.s   = 0 - ((volpan-75) * 2);
                    eff.effect    = UNI_VOLSLIDE;
                    eff.framedly |= UFD_RUNONCE;
                    if(eff.param.s)
                        utrk_write_local(h->ut, &eff, STMEM_VOLSLIDE);
                    else
                        utrk_memory_local(h->ut, &eff, STMEM_VOLSLIDE, -1);
                } else if((volpan>=85) && (volpan<=94))        // volume slide up (85-94)
                {   eff.param.s   = (volpan-85) * 2;
                    eff.effect    = UNI_VOLSLIDE;
                    eff.framedly += 1;
                    if(eff.param.s)
                        utrk_write_local(h->ut, &eff, STMEM_VOLSLIDE);
                    else
                        utrk_memory_local(h->ut, &eff, STMEM_VOLSLIDE, 1);
                } else if((volpan>=95) && (volpan<=104))       // volume slide down   (95-104)
                {   eff.param.s   = 0 - ((volpan-95) * 2);
                    eff.effect    = UNI_VOLSLIDE;
                    eff.framedly += 1;
                    if(eff.param.s)
                        utrk_write_local(h->ut, &eff, STMEM_VOLSLIDE);
                    else
                        utrk_memory_local(h->ut, &eff, STMEM_VOLSLIDE, -1);
                } else if((volpan>=105) && (volpan<=114))      // pitch slide down (105-114)
                {   eff.param.s  = 0 - (volpan-105)*32;
                    if(eff.param.s)
                    {   eff.effect    = UNI_PITCHSLIDE;
                        eff.framedly += 1;
                        utrk_write_local(h->ut, &eff, STMEM_PITCHSLIDE);
                    } else
                        utrk_memory_local(h->ut, &eff, STMEM_PITCHSLIDE, -1);
                } else if((volpan>=115) && (volpan<=124))      // pitch slide up (115-124)
                {   eff.param.s  = (volpan-115)*32;
                    if(eff.param.s)
                    {   eff.effect    = UNI_PITCHSLIDE;
                        eff.framedly += 1;                
                        utrk_write_local(h->ut, &eff, STMEM_PITCHSLIDE);
                    } else
                        utrk_memory_local(h->ut, &eff, STMEM_PITCHSLIDE, 1);
                } else if((volpan>=128) && (volpan<=192))      // Set Panning.
                {   eff.param.s  = (volpan-128-32) * 4;
                    eff.effect   = UNI_PANNING;
                    utrk_write_local(h->ut, &eff, UNIMEM_NONE);
                } else if((volpan>=193) && (volpan<=202))      // portamento to note
                {   eff.param.u   = portatable[volpan-193]*8;
                    eff.effect    = UNI_PORTAMENTO;
		    //                    eff.framedly += 1;
                    if(eff.param.u)
                        utrk_write_local(h->ut, &eff, gxx_memory);
                    else
                        utrk_memory_local(h->ut, &eff, gxx_memory, (gxx_memory == STMEM_PITCHSLIDE) ? 1 : 0);
                } else if((volpan>=203) && (volpan<=212))      // vibrato  [Hx]
                {   eff.param.u  = (volpan-203)*8;
                    eff.effect   = UNI_VIBRATO_DEPTH;
                    //eff.framedly = 0;
                    if(eff.param.u)
                        utrk_write_local(h->ut, &eff, PTMEM_VIBRATO_DEPTH);
                    else
                        utrk_memory_local(h->ut, &eff, PTMEM_VIBRATO_DEPTH, 0);

                    eff.effect = UNI_VIBRATO_SPEED;
                    utrk_memory_local(h->ut, &eff, PTMEM_VIBRATO_SPEED, 0);
                }
            }
        }
    } while(row < patrows);

    return 1;
}


// =====================================================================================
    BOOL IT_Load(IT_HANDLE *h, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    uint        t;
    int         u,lp;
    INSTRUMENT  *d;
    UNISAMPLE   *q;
    EXTSAMPLE   *eq;

    // try to read module header

    _mm_read_I_ULONG(mmfile);   // kill the 4 byte header
    _mm_read_string(h->mh.songname,26,mmfile);
    _mm_read_UBYTES(h->mh.blank01,2,mmfile);
    h->mh.ordnum      =_mm_read_I_UWORD(mmfile);
    h->mh.insnum      =_mm_read_I_UWORD(mmfile);
    h->mh.smpnum      =_mm_read_I_UWORD(mmfile);
    h->mh.patnum      =_mm_read_I_UWORD(mmfile);
    h->mh.cwt         =_mm_read_I_UWORD(mmfile);
    h->mh.cmwt        =_mm_read_I_UWORD(mmfile);
    h->mh.flags       =_mm_read_I_UWORD(mmfile);
    h->mh.special     =_mm_read_I_UWORD(mmfile);

    h->mh.globvol     =_mm_read_UBYTE(mmfile);
    h->mh.mixvol      =_mm_read_UBYTE(mmfile);
    h->mh.initspeed   =_mm_read_UBYTE(mmfile);
    h->mh.inittempo   =_mm_read_UBYTE(mmfile);
    h->mh.pansep      =_mm_read_UBYTE(mmfile);
    h->mh.zerobyte    =_mm_read_UBYTE(mmfile);
    h->mh.msglength   =_mm_read_I_UWORD(mmfile);
    h->mh.msgoffset   =_mm_read_I_ULONG(mmfile);
    _mm_read_UBYTES(h->mh.blank02,4,mmfile);
    _mm_read_UBYTES(h->mh.pantable,64,mmfile);
    _mm_read_UBYTES(h->mh.voltable,64,mmfile);

    if(_mm_feof(mmfile))
    {   _mmlog(iterr_eof_header);
        return 0;
    }

    // set module variables

    of->memsize     = STMEM_LAST;      // Number of memory slots to reserve!
    of->modtype     = _mm_strdup(of->allochandle, IT_Version);
    of->modtype[15] = (h->mh.cwt >> 8) + 0x30;
    of->modtype[17] = ((h->mh.cwt >> 4) & 0xf) + 0x30;
    of->modtype[18] = ((h->mh.cwt) & 0xf) + 0x30;
    of->songname    = DupStr(of->allochandle, h->mh.songname,26);    // make a cstr of songname 
    of->reppos      = 0;
    of->numpat      = h->mh.patnum;
    of->numins      = h->mh.insnum;
    of->numsmp      = h->mh.smpnum;
    of->initspeed   = h->mh.initspeed;
    of->inittempo   = h->mh.inittempo;
    of->initvolume  = h->mh.globvol;
 
    old_effect = 0;
    of->flags   = UF_LINEAR_PITENV;
    if(h->mh.flags & 8) { of->flags |= UF_LINEAR; }
    if((h->mh.cwt >= 0x106) && (h->mh.flags & 16))  old_effect |= 1;
    if(h->mh.flags & 32) old_effect |= 4;

    if (h->mh.flags&1) of->pansep=h->mh.pansep; else of->pansep=0; /* Force to mono. */

    // set channel volumes
    for(t=0; t<64; t++)
        of->chanvol[t] = h->mh.voltable[t];

    // read the order data
    if(!AllocPositions(of, h->mh.ordnum)) return 0;

    for(t=0; t<h->mh.ordnum; t++)
        of->positions[t] = _mm_read_UBYTE(mmfile);

    if(_mm_feof(mmfile))
    {   _mmlog(iterr_eof_header);
        return 0;
    }
    
    of->numpos = 0;
    for(t=0; t<h->mh.ordnum; t++)
    {   if((of->positions[of->numpos] = of->positions[t]) < h->mh.patnum)
        {   h->poslookup[t]      = of->numpos;    // bug fix for FREAKY S3Ms / ITs
            if(of->positions[t] < 254) of->numpos++;
        }
    }

    if((h->paraptr = (ULONG *)_mm_malloc(h->allochandle, (h->mh.insnum+h->mh.smpnum+of->numpat)*sizeof(ULONG))) == NULL) return 0;

    // read the instrument, sample, and pattern parapointers
    _mm_read_I_ULONGS(h->paraptr,h->mh.insnum+h->mh.smpnum+of->numpat,mmfile);

    // now is a good time to check if the header was too short :)
    if(_mm_feof(mmfile))
    {   _mmlog(iterr_eof_header);
        return 0;
    }

    // Check for and load song comment
    if(h->mh.special & 1)
    {   _mm_fseek(mmfile,(long)(h->mh.msgoffset),SEEK_SET);
        if(!(of->comment=(CHAR *)_mm_malloc(of->allochandle, h->mh.msglength+1))) return 0;
        _mm_read_UBYTES(of->comment,h->mh.msglength,mmfile);
        of->comment[h->mh.msglength] = 0;

        /*for(t=0; t<h->mh.msglength; t++)
        {   
        }*/
    }

    if(!(h->mh.flags & 4)) of->numins = 0;
    if(!AllocSamples(of, 1)) return 0;

    q  = of->samples;
    eq = of->extsamples;

    // Load all samples (they're used either way)

    for(t=0; t<h->mh.smpnum; t++)
    {   ITSAMPLE s;

        // seek to sample position (or skip sample is pointer is null)

        if(!h->paraptr[h->mh.insnum+t]) continue;
        _mm_fseek(mmfile,(long)(h->paraptr[h->mh.insnum+t] + 4),SEEK_SET);

        // and load sample info
        _mm_read_string(s.filename,12,mmfile);
        s.zerobyte    = _mm_read_UBYTE(mmfile);
        s.globvol     = _mm_read_UBYTE(mmfile);
        s.flag        = _mm_read_UBYTE(mmfile);
        s.volume      = _mm_read_UBYTE(mmfile);
        _mm_read_string(s.sampname,26,mmfile);
        s.convert     = _mm_read_UBYTE(mmfile);
        s.panning     = _mm_read_UBYTE(mmfile);
        s.length      = _mm_read_I_ULONG(mmfile);
        s.loopbeg     = _mm_read_I_ULONG(mmfile);
        s.loopend     = _mm_read_I_ULONG(mmfile);
        s.c5spd       = _mm_read_I_ULONG(mmfile);
        s.susbegin    = _mm_read_I_ULONG(mmfile);
        s.susend      = _mm_read_I_ULONG(mmfile);
        s.sampoffset  = _mm_read_I_ULONG(mmfile);
        s.vibspeed    = _mm_read_UBYTE(mmfile);
        s.vibdepth    = _mm_read_UBYTE(mmfile);
        s.vibrate     = _mm_read_UBYTE(mmfile);
        s.vibwave     = _mm_read_UBYTE(mmfile);

 
        // Generate an error if c5spd is > 8 megs, or samplelength > 256 megs
        //  (nothing would EVER be that high)

        if(_mm_feof(mmfile))
	    {   _mmlog("load_it > Failure: Unexpected end of file reading sample header %d",t);
            return 0;
        }

		if((s.c5spd > 0x7fffffL) || (s.length > 0xfffffffUL))
	    {   _mmlog("load_it > Failure: Invalid data in sample %d. c5spd = %d; length = %d",t,s.c5spd, s.length);
            return 0;
		}

        q->samplename  = DupStr(of->allochandle, s.sampname,26);
        q->seekpos     = s.sampoffset;

        q->speed       = s.c5spd / 2;
        q->volume      = s.volume * 2;
        if(s.panning & 128)
        {   q->flags   |= PSF_OWNPAN;
            q->panning  = ((s.panning & 127) * IT_PANMUL) + PAN_LEFT;
        }
        q->length      = s.length;
        q->loopstart   = s.loopbeg;
        q->loopend     = s.loopend;
        q->susbegin    = s.susbegin;
        q->susend      = s.susend;

        eq->globvol    = s.globvol;

        if(s.vibrate)
        {   eq->vibflags |= AV_IT;
            switch(s.vibwave)   // convert IT's auto-vibrato types to MikMod's
            {   case 0:  eq->vibtype = 0;  break;
                case 1:  eq->vibtype = 2;  break;
                case 2:  eq->vibtype = 1;  break;
                case 3:  eq->vibtype = 4;  break;
            }
            eq->vibsweep  = s.vibrate;
            eq->vibdepth  = s.vibdepth; //*2;
            eq->vibrate   = s.vibspeed;
        }

        // set looping flags
        if(s.flag & 16)  q->flags |= SL_LOOP;
        if(s.flag & 32)  q->flags |= SL_SUSTAIN_LOOP;
        if(s.flag & 64)  q->flags |= SL_BIDI;
        if(s.flag & 128) q->flags |= SL_SUSTAIN_BIDI;

        // set disk format
        if(s.flag & 8)  q->compress = DECOMPRESS_IT214;
        if(s.flag & 2)  q->format  |= SF_16BITS;
        if(h->mh.cwt >= 0x200)
        {   if(s.convert & 1) q->format |= SF_SIGNED;
            if(s.convert & 4) q->format |= SF_DELTA;   
        }                              
        q++; eq++;
    }

    // Load instruments if instrument mode flag enabled

    if(h->mh.flags & 4)
    {   if(!AllocInstruments(of)) return 0;
        d = of->instruments;
        of->flags |= UF_NNA | UF_INST;

        for(t=0; t<h->mh.insnum; t++)
        {   ITINSTHEADER ih;

            // seek to instrument position (or skip if the pointer is null)

            if(!h->paraptr[t]) continue;
            _mm_fseek(mmfile,h->paraptr[t]+4,SEEK_SET);

            // and load instrument info
            _mm_read_string(ih.filename,12,mmfile);
            ih.zerobyte  = _mm_read_UBYTE(mmfile);
            if(h->mh.cwt < 0x200) // load IT 1.xx inst header
            {   ih.volflg    = _mm_read_UBYTE(mmfile);
                ih.volbeg    = _mm_read_UBYTE(mmfile);
                ih.volend    = _mm_read_UBYTE(mmfile);
                ih.volsusbeg = _mm_read_UBYTE(mmfile);
                ih.volsusend = _mm_read_UBYTE(mmfile);
                _mm_read_I_UWORD(mmfile);
                ih.fadeout   = _mm_read_I_UWORD(mmfile);
                ih.nna       = _mm_read_UBYTE(mmfile);
                ih.dnc       = _mm_read_UBYTE(mmfile);
            } else   // Read IT200+ header
            {   ih.nna       = _mm_read_UBYTE(mmfile);
                ih.dct       = _mm_read_UBYTE(mmfile);
                ih.dca       = _mm_read_UBYTE(mmfile);
                ih.fadeout   = _mm_read_I_UWORD(mmfile);
                ih.ppsep     = _mm_read_UBYTE(mmfile);
                ih.ppcenter  = _mm_read_UBYTE(mmfile);
                ih.globvol   = _mm_read_UBYTE(mmfile);
                ih.chanpan   = _mm_read_UBYTE(mmfile);
                ih.rvolvar   = _mm_read_UBYTE(mmfile);
                ih.rpanvar   = _mm_read_UBYTE(mmfile);
            }

            ih.trkvers   = _mm_read_I_UWORD(mmfile);
            ih.numsmp    = _mm_read_UBYTE(mmfile);
            _mm_read_UBYTE(mmfile);
            _mm_read_string(ih.name,26,mmfile);
            
            ih.cutoff      = _mm_read_UBYTE(mmfile);
            ih.resonance   = _mm_read_UBYTE(mmfile);

            _mm_read_UBYTES(ih.blank01,4,mmfile);
            _mm_read_I_UWORDS(ih.samptable,120,mmfile);

            // Base Header Done... Load ENVELOPES!
            // -----------------------------------

            if(h->mh.cwt < 0x200)  // load IT 1xx volume envelope
            {   _mm_read_UBYTES(ih.volenv,200,mmfile);
                for(lp=0; lp<25; lp++)
                {   ih.oldvoltick[lp] = _mm_read_UBYTE(mmfile);
                    ih.volnode[lp]    = _mm_read_UBYTE(mmfile);
                }
            } else // load IT 2xx vol & chanpan & pitch envs
            {   ih.volflg    = _mm_read_UBYTE(mmfile);
                ih.volpts    = _mm_read_UBYTE(mmfile);
                ih.volbeg    = _mm_read_UBYTE(mmfile);
                ih.volend    = _mm_read_UBYTE(mmfile);
                ih.volsusbeg = _mm_read_UBYTE(mmfile);
                ih.volsusend = _mm_read_UBYTE(mmfile);
                for(lp=0; lp<25; lp++)
                {   ih.volnode[lp] = _mm_read_UBYTE(mmfile);
                    ih.voltick[lp] = _mm_read_I_UWORD(mmfile);
                }
                _mm_read_UBYTE(mmfile);

                ih.panflg    = _mm_read_UBYTE(mmfile);
                ih.panpts    = _mm_read_UBYTE(mmfile);
                ih.panbeg    = _mm_read_UBYTE(mmfile);
                ih.panend    = _mm_read_UBYTE(mmfile);
                ih.pansusbeg = _mm_read_UBYTE(mmfile);
                ih.pansusend = _mm_read_UBYTE(mmfile);
                for(lp=0; lp<25; lp++)
                {   ih.pannode[lp] = _mm_read_SBYTE(mmfile);
                    ih.pantick[lp] = _mm_read_I_UWORD(mmfile);
                }
                _mm_read_UBYTE(mmfile);

                ih.pitflg    = _mm_read_UBYTE(mmfile);
                ih.pitpts    = _mm_read_UBYTE(mmfile);
                ih.pitbeg    = _mm_read_UBYTE(mmfile);
                ih.pitend    = _mm_read_UBYTE(mmfile);
                ih.pitsusbeg = _mm_read_UBYTE(mmfile);
                ih.pitsusend = _mm_read_UBYTE(mmfile);
                for(lp=0; lp<25; lp++)
                {   ih.pitnode[lp] = _mm_read_SBYTE(mmfile);
                    ih.pittick[lp] = _mm_read_I_UWORD(mmfile);
                }
                _mm_read_UBYTE(mmfile);
            }
 
            if(_mm_feof(mmfile))
		    {   _mmlog("load_it > Failure: Unexpected end of file reading instrument header %d",t);
                return 0;
            }

            d->volflg |= EF_VOLENV | EF_INCLUSIVE;
            d->insname = DupStr(of->allochandle, ih.name,26);
            d->nnatype = ih.nna;

            if(h->mh.cwt < 0x200)
            {   d->volfade = ih.fadeout << 6;
                if(ih.dnc)
                {   d->dct = DCT_NOTE;
                    d->dca = DCA_CUT;
                }

                if(ih.volflg & 1) d->volflg |= EF_ON;
                if(ih.volflg & 2) d->volflg |= EF_LOOP;
                if(ih.volflg & 4) d->volflg |= EF_SUSTAIN;
               
                // XM conversion of IT envelope Array

                d->volbeg    = ih.volbeg;   
                d->volend    = ih.volend;
                d->volsusbeg = ih.volsusbeg;
                d->volsusend = ih.volsusend;

                if(ih.volflg & 1)
                {   for(u=0; u<25; u++)
                        if(ih.oldvoltick[d->volpts] != 0xff)
                        {   d->volenv[d->volpts].val = (ih.volnode[d->volpts] << 2);
                            d->volenv[d->volpts].pos = ih.oldvoltick[d->volpts];
                            d->volpts++;
                        } else break;
                }  
            } else
            {   BOOL inuse;      // used to determine if the envelope data is actually worth keeping

                d->panning = ((ih.chanpan & 127) * IT_PANMUL) + PAN_LEFT;
                if(!(ih.chanpan & 128)) d->flags |= IF_OWNPAN;

                if(ih.ppsep)
                {   d->pitpansep    = ih.ppsep * 2;
                    d->pitpancenter = ih.ppcenter;
                }

                d->globvol = ih.globvol >> 1;
                d->volfade = ih.fadeout << 5;
                d->dct     = ih.dct;
                d->dca     = ih.dca;

                if(ih.cutoff & 128)
                {   
                    // I backwards engineered Lim's oddball cutoff scale.
                    // First off, cutoff is a signed value.  Secondly, it is a scaled version
                    // of the mikmod cutoff, whic supports a wider range (or I think it does ;)
                    // Finally, Lim's 127 value is 'special' and is basically minimal cutoff.
                    //  - I determined, through trial and error, that 238 is the proper 
                    //    scale to convert IT's cutoff to mikmod's extended cutoff range!

                    if(ih.cutoff == 255)
                        d->cutoff = 128;
                    else 
                        d->cutoff  = (ih.cutoff & 127);
                    d->flags    |= IF_USE_CUTOFF;
                }

                if(ih.resonance & 128)
                {   d->resonance = (ih.resonance & 127) + 1;
                    d->flags    |= IF_USE_RESONANCE;
                }

                if(h->mh.cwt >= 0x204)
                {   d->rvolvar = ih.rvolvar;
                    d->rpanvar = ih.rpanvar;
                }

                // Note the currently undocumented 'CARRY' flag, which I now support.
                // kudos to me.  rah rah!

                if(ih.volflg & 1 && ih.volpts) d->volflg |= EF_ON;
                if(ih.volflg & 2) d->volflg |= EF_LOOP;
                if(ih.volflg & 4) d->volflg |= EF_SUSTAIN;
                if(ih.volflg & 8) d->volflg |= EF_CARRY;

                d->panflg = EF_INCLUSIVE;
                if(ih.panflg & 1 && ih.panpts) d->panflg |= EF_ON;
                if(ih.panflg & 2) d->panflg |= EF_LOOP;
                if(ih.panflg & 4) d->panflg |= EF_SUSTAIN;
                if(ih.panflg & 8) d->panflg |= EF_CARRY;

                d->volpts    = ih.volpts;
                d->volbeg    = ih.volbeg;
                d->volend    = ih.volend;
                d->volsusbeg = ih.volsusbeg;
                d->volsusend = ih.volsusend;

                for(u=0, inuse=0; u<ih.volpts; u++)
                {   d->volenv[u].val = (ih.volnode[u] << 2);
                    d->volenv[u].pos = ih.voltick[u];
                    //if(ih.volnode[u] != 64) inuse = 1;
                }
                // checking for inuse messes up ones that rely on volfade at the end of an envelope!
                //if(!inuse) d->volpts = 0;

                d->panpts    = ih.panpts;
                d->panbeg    = ih.panbeg;
                d->panend    = ih.panend;
                d->pansusbeg = ih.pansusbeg;
                d->pansusend = ih.pansusend;
 
                for(u=0, inuse=0; u<ih.panpts; u++)
                {   d->panenv[u].val = ih.pannode[u] * IT_PANMUL;
                    d->panenv[u].pos = ih.pantick[u];
                    if(ih.pannode[u] != 0) inuse = 1;
                }
                if(!inuse) d->panpts = 0;

                // ImpulseTracker Pitch / Resonance Filter Envelope.
                // -------------------------------------------------
                // When the flag at bit 7 is set, it's actually a resonance envelope.
                // neat, huh? :)
                
                if(ih.pitflg & 128)
                {   
                    d->resflg = EF_INCLUSIVE;

                    if(ih.pitflg & 1 && ih.pitpts) d->resflg |= EF_ON;
                    if(ih.pitflg & 2) d->resflg |= EF_LOOP;
                    if(ih.pitflg & 4) d->resflg |= EF_SUSTAIN;
                    if(ih.pitflg & 8) d->resflg |= EF_CARRY;

                    d->respts    = ih.pitpts;
                    d->resbeg    = ih.pitbeg;
                    d->resend    = ih.pitend;
                    d->ressusbeg = ih.pitsusbeg;
                    d->ressusend = ih.pitsusend;

                    for(u=0, inuse=0; u<ih.pitpts; u++)
                    {   d->resenv[u].val = (ih.pitnode[u]+32) * 2; // * 256);
                        d->resenv[u].pos = ih.pittick[u];
                        if(ih.pitnode[u] != 0) inuse = 1;
                    }
                    if(!inuse) d->respts = 0;
                
                } else
                {
                    d->pitflg    = EF_INCLUSIVE;

                    if(ih.pitflg & 1 && ih.pitpts) d->pitflg |= EF_ON;
                    if(ih.pitflg & 2) d->pitflg |= EF_LOOP;
                    if(ih.pitflg & 4) d->pitflg |= EF_SUSTAIN;
                    if(ih.pitflg & 8) d->pitflg |= EF_CARRY;

                    d->pitpts    = ih.pitpts;
                    d->pitbeg    = ih.pitbeg;
                    d->pitend    = ih.pitend;
                    d->pitsusbeg = ih.pitsusbeg;
                    d->pitsusend = ih.pitsusend;

                    for(u=0, inuse=0; u<ih.pitpts; u++)
                    {   d->pitenv[u].val = ih.pitnode[u];
                        d->pitenv[u].pos = ih.pittick[u];
                        if(ih.pitnode[u] != 0) inuse = 1;
                    }
                    if(!inuse) d->pitpts = 0;
                }
            }
   
            if(of->flags & UF_LINEAR)
            {   for(u=0; u<120; u++)
                {   d->samplenote[u]   = (ih.samptable[u] & 255);
                    d->samplenumber[u] = (ih.samptable[u] >> 8) ? ((ih.samptable[u] >> 8) - 1) : 255;
                }
            } else
            {   for(u=0; u<120; u++)
                {   d->samplenote[u]   = (ih.samptable[u] & 255);
                    d->samplenumber[u] = (ih.samptable[u] >> 8) ? ((ih.samptable[u] >> 8) - 1) : 255;
                }
            }

            d++;                  
        }
    }

    // Figure out how many channels this blasted song actually uses (what
    // ever happened to common courtesy of storing this simple value
    // somewhere in the damn module, eh!?)

    of->numchn = 0;
    memset(h->remap,-1,64*sizeof(UBYTE));

    for(t=0; t<of->numpat; t++)
    {   
        // seek to pattern position
        // No parapointer = pattern of 64 rows, EMPTY

        if(h->paraptr[h->mh.insnum+h->mh.smpnum+t] != 0)
        {   UWORD packlen;

            _mm_fseek(mmfile,(((long)h->paraptr[h->mh.insnum+h->mh.smpnum+t])),SEEK_SET);
            packlen = _mm_read_I_UWORD(mmfile);
            packlen = _mm_read_I_UWORD(mmfile);    // read pattern length (# of rows)

            if(packlen < 2048)    // check for bad pattern data...
            {
                _mm_read_I_ULONG(mmfile);
                if(IT_GetNumChannels(mmfile, h->mask, h->remap, packlen)) return 0;
            }
        }
    }

    // give each of them a different number

    for(t=0; t<64; t++)
    {   if(h->remap[t]==0)
        {   h->remap[t] = of->numchn;
            of->numchn++;
        }
    }


    // ============================================================
    // set panning positions AFTER building remap chart!

    for(t=0; t<64; t++)
    {   if(h->remap[t] != -1)
        {   if((h->mh.pantable[t]&127) <= 64) of->panning[h->remap[t]] = (h->mh.pantable[t] - 32) * IT_PANMUL;
            else if((h->mh.pantable[t]&127)==100) of->panning[h->remap[t]] = PAN_SURROUND;

            if(h->mh.pantable[t] & 128) of->muted[h->remap[t]] = TRUE;
        }
    }


    of->numtrk = of->numpat*of->numchn;
    if(!AllocPatterns(of)) return 0;
    if(!AllocTracks(of)) return 0;

    h->ut = of->ut = utrk_init(of->numchn, h->allochandle);

    utrk_memory_reset(h->ut);
    utrk_local_memflag(h->ut, PTMEM_PORTAMENTO, TRUE, FALSE);
    utrk_local_memflag(h->ut, STMEM_VOLSLIDE, FALSE, TRUE);

    {
        int    gxx_memory;

        if(h->mh.flags & 32)
        {   // E/F/G share memory, so mark the memory as completely volatile.
            // Same goes for the E/F/G vol-column effects?
            gxx_memory = STMEM_PITCHSLIDE;
            utrk_local_memflag(h->ut, STMEM_PITCHSLIDE, TRUE, TRUE);   // Std. Effects column.
        } else gxx_memory = PTMEM_PORTAMENTO;

        memset(h->last,255,sizeof(ITNOTE) * 64);
        for(t=0; t<of->numpat; t++)
        {   // seek to pattern position
            //_mmlogd1("Loading Pattern %d", t);

            if(h->paraptr[h->mh.insnum+h->mh.smpnum+t] == 0)  // No parapointer = pattern of 64 rows, EMPTY
            {   
                //_mmlogd(" > Blank");
                of->pattrows[t] = 64;
                utrk_reset(h->ut); utrk_dup_pattern(h->ut, of);
            } else
            {   UWORD packlen;

                _mm_fseek(mmfile,(((long)h->paraptr[h->mh.insnum+h->mh.smpnum+t])),SEEK_SET);
                packlen         = _mm_read_I_UWORD(mmfile);
                of->pattrows[t] = _mm_read_I_UWORD(mmfile);

                if(of->pattrows[t] >= 2048)
                {
                    // bad pattern .. make it blank.

                    of->pattrows[t] = 1;
                    utrk_reset(h->ut); utrk_dup_pattern(h->ut, of);
                }

                _mm_read_I_ULONG(mmfile);


                //_mmlogd2(" > Stuff : %d %d", packlen, of->pattrows[t]);

                if(!IT_ReadPattern(mmfile, h, of->pattrows[t], gxx_memory)) return 0;
                utrk_dup_pattern(h->ut, of);
            }
        }
    }

    return 1;
}


// =====================================================================================
    CHAR *IT_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
   CHAR s[26];

   _mm_fseek(mmfile,4,SEEK_SET);
   if(!_mm_read_UBYTES(s,26,mmfile)) return NULL;
   
   return(DupStr(NULL, s,26));
}


// =====================================================================================
    MLOADER load_it =
// =====================================================================================
{
   "IT",
   "Impulsetracker",

    0,                              // default FULL STEREO panning

    NULL,
    IT_Test,
    IT_Init,
    (void (*)(ML_HANDLE *))IT_Cleanup,
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))IT_Load,
    IT_LoadTitle
};

