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
 Module: LOAD_STM.C

  ScreamTracker 2 (STM) module Loader - Version 1.oOo Release 2 
  A Coding Nightmare by Rao and Air Richter of HaRDCoDE
  You can now play all of those wonderful old C.C. Catch STM's!

 Portability:
  All systems - all compilers (hopefully)

*/

#include <string.h>
#include <ctype.h>
#include "mikmod.h"
#include "itshare.h"

#ifndef WIN32
#include <ctype.h>
void strlwr(char *z)
{
  while(*z)
    {
      *z=(char)tolower((int)*z);
      z++;
    }
}
#endif

/* strlwr seems to be a Win32 library function; Mingw32 has it, but not
   Linux glibc. */

typedef struct STMNOTE
{   UBYTE note,insvol,volcmd,cmdinf;
} STMNOTE;


// Raw STM sampleinfo struct:

typedef struct STMSAMPLE
{  CHAR  filename[12];   // Can't have long comments - just filename comments :)
   UBYTE unused;         // 0x00
   UBYTE instdisk;       // Instrument disk
   UWORD reserved;       // ISA in memory when in ST 2
   UWORD length;         // Sample length
   UWORD loopbeg;        // Loop start point
   UWORD loopend;        // Loop end point
   UBYTE volume;         // Volume
   UBYTE reserved2;      // More reserved crap
   UWORD c2spd;          // Good old c2spd
   UBYTE reserved3[4];   // Yet more of PSi's reserved crap
   UWORD isa;            // Internal Segment Address ->
                         //    contrary to the tech specs, this is NOT actually
                         //    written to the stm file.
} STMSAMPLE;

// Raw STM header struct:

typedef struct STMHEADER
{  CHAR  songname[20];
   CHAR  trackername[8];   // !SCREAM! for ST 2.xx 
   UBYTE unused;           // 0x1A 
   UBYTE filetype;         // 1=song, 2=module (only 2 is supported, of course) :) 
   UBYTE ver_major;        // Like 2 
   UBYTE ver_minor;        // "ditto" 
   UBYTE inittempo;        // initspeed= stm inittempo>>4 
   UBYTE  numpat;          // number of patterns 
   UBYTE   globalvol;      // <- WoW! a RiGHT TRiANGLE =8*) 
   UBYTE    reserved[13];  // More of PSi's internal crap 
   STMSAMPLE sample[31];   // STM sample data
   UBYTE patorder[128];    // Docs say 64 - actually 128
} STMHEADER;


static CHAR  STM_Version[] = "Screamtracker 2";

// =====================================================================================
    BOOL STM_Test(MMSTREAM *mmfile)
// =====================================================================================
{
   CHAR  str[9];
   UBYTE filetype;

   _mm_fseek(mmfile,21,SEEK_SET);
   _mm_read_UBYTES(str,9,mmfile);
   str[8]=0; strlwr(str);
   filetype = _mm_read_UBYTE(mmfile);
   if(!memcmp(str,"!scream!",8) || (filetype!=2)) // STM Module = filetype 2
      return 0;
   return 1;
}


// =====================================================================================
    void *STM_Init(void)
// =====================================================================================
{
    return _mm_calloc(NULL, 1,sizeof(STMHEADER));
}

// =====================================================================================
    void STM_Cleanup(void *handle)
// =====================================================================================
{
    _mm_free(NULL, handle);
}


// =====================================================================================
    void STM_ConvertNote(UTRK_WRITER *ut, STMNOTE *n)
// =====================================================================================
{
    uint note,ins,vol,cmd,inf;

    // extract the various information from the 4 bytes that
    //  make up a single note

    note = n->note;
    ins  = n->insvol>>3;
    vol  = (n->insvol&7)+(n->volcmd>>1);
    cmd  = n->volcmd&15;
    inf  = n->cmdinf;

    if(ins<32)  utrk_write_inst(ut, ins);

    // special values of [SBYTE0] are handled here ->
    // we have no idea if these strange values will ever be encountered.
    // but it appears as though stms sound correct.
    if(note==254 || note==252) pt_write_effect(ut, 0xc,0); // <- note off command (???)
    else
    // if note < 251, then all three bytes are stored in the file
    if(note<251) utrk_write_note(ut, (((note>>4)+2)*12)+(note&0xf) + 1);      // <- normal note and up the octave by two

    if(vol<65) pt_write_effect(ut, 0xc, vol);

    if(cmd!=255)
    {   if(cmd==1)
        {   pt_write_effect(ut, 0xf,inf>>4);
        } else S3MIT_ProcessCmd(ut, NULL, cmd, inf, 3,PTMEM_PORTAMENTO);
    }
}


// =====================================================================================
    BOOL STM_LoadPatterns(UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    uint     t,s;
    STMNOTE  n;

    if(!AllocTracks(of)) return 0;
    if(!AllocPatterns(of)) return 0;
    of->ut = utrk_init(4, of->allochandle);
    utrk_memory_reset(of->ut);
    utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);

    // Allocate temporary buffer for loading
    // and converting the patterns

    for(t=0; t<of->numpat; t++)
    {   utrk_reset(of->ut);
        for(s=0; s<(64*4); s++)
        {   n.note   = _mm_read_UBYTE(mmfile);
            n.insvol = _mm_read_UBYTE(mmfile);
            n.volcmd = _mm_read_UBYTE(mmfile);
            n.cmdinf = _mm_read_UBYTE(mmfile);

            utrk_settrack(of->ut,s & 3);
            STM_ConvertNote(of->ut,&n);
            if((s & 3) == 3) utrk_newline(of->ut);
        }
  
        if(_mm_feof(mmfile))
	    {   _mmlog("load_stm > Failure: Unexpected end of file reading pattern %d",t);
            return 0;
        }

        if(!utrk_dup_pattern(of->ut,of)) return 0;
    }

    return 1;
}


// =====================================================================================
    BOOL STM_Load(STMHEADER *mh, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    uint       t; 
    ULONG      MikMod_ISA; // We MUST generate our own ISA - NOT stored in the stm
    UNISAMPLE *q;

    // try to read stm header

    _mm_read_string(mh->songname,20,mmfile);
    _mm_read_string(mh->trackername,8,mmfile);
    mh->unused      =_mm_read_UBYTE(mmfile);
    mh->filetype    =_mm_read_UBYTE(mmfile);
    mh->ver_major   =_mm_read_UBYTE(mmfile);
    mh->ver_minor   =_mm_read_UBYTE(mmfile);
    mh->inittempo   =_mm_read_UBYTE(mmfile);
    mh->numpat      =_mm_read_UBYTE(mmfile);
    mh->globalvol   =_mm_read_UBYTE(mmfile);
    _mm_read_UBYTES(mh->reserved,13,mmfile);

    for(t=0;t<31;t++)
    {   STMSAMPLE *s = &mh->sample[t];  // STM sample data

        _mm_read_string(s->filename,12,mmfile);
        s->unused    =_mm_read_UBYTE(mmfile);
        s->instdisk  =_mm_read_UBYTE(mmfile);
        s->reserved  =_mm_read_I_UWORD(mmfile);
        s->length    =_mm_read_I_UWORD(mmfile);
        s->loopbeg   =_mm_read_I_UWORD(mmfile);
        s->loopend   =_mm_read_I_UWORD(mmfile);
        s->volume    =_mm_read_UBYTE(mmfile);
        s->reserved2 =_mm_read_UBYTE(mmfile);
        s->c2spd     =_mm_read_I_UWORD(mmfile);
        _mm_read_UBYTES(s->reserved3,4,mmfile);
        s->isa       =_mm_read_I_UWORD(mmfile);
    }
    _mm_read_UBYTES(mh->patorder,128,mmfile);

    if(_mm_feof(mmfile))
    {   _mmlog("load_stm > Failure: Unexpected end of file reading module header");
        return 0;
    }

    // set module variables

    of->memsize   = STMEM_LAST;              // Number of memory slots to reserve!
    of->modtype   = _mm_strdup(of->allochandle, STM_Version);
    of->songname  = DupStr(of->allochandle, mh->songname,20); // make a cstr of songname
    of->numpat    = mh->numpat;
    of->inittempo = 125;                     // mh->inittempo+0x1c;
    of->initspeed = mh->inittempo>>4;
    of->numchn    = 4;                       // get number of channels

    t=0;
    if(!AllocPositions(of, 0x80)) return 0;
    while(mh->patorder[t]!=99)    // 99 terminates the patorder list
    {   of->positions[t] = mh->patorder[t];
        t++;
    }
    of->numpos = --t;
    of->numtrk = of->numpat*of->numchn;

    // Finally, init the sampleinfo structures
    of->numsmp = 31;    // always this

    if(!AllocSamples(of, 0))        return 0;
    if(!STM_LoadPatterns(of,mmfile)) return 0;

    q = of->samples;

    MikMod_ISA = _mm_ftell(mmfile);
    MikMod_ISA = (MikMod_ISA+15) & 0xfffffff0;

    for(t=0; t<of->numsmp; t++)
    {   // load sample info

        q->samplename   = DupStr(of->allochandle, mh->sample[t].filename,12);
        q->seekpos      = MikMod_ISA;

        q->speed      = mh->sample[t].c2spd;
        q->volume     = mh->sample[t].volume * 2;
        q->length     = mh->sample[t].length;
        if (/*!mh->sample[t].volume || */q->length==1) q->length = 0; // if vol = 0 or length = 1, then no sample
        q->loopstart  = mh->sample[t].loopbeg;
        q->loopend    = mh->sample[t].loopend;

        MikMod_ISA += q->length;
        MikMod_ISA  = (MikMod_ISA+15) & 0xfffffff0;

        // Once again, contrary to the STM specs, all the sample data is
        // actually SIGNED! Sheesh

        q->format  = SF_SIGNED;

        if((q->loopstart<q->length) && (q->loopend>0) && (q->loopend != 0xffff)) q->flags |= SL_LOOP;

        // fix replen if repend>length

        if(q->loopend > q->length) q->loopend = q->length;

        // Enable aggressive declicking for songs that do not loop and that
        // are long enough that they won't be adversely affected.
        
        if(!(q->flags & (SL_LOOP | SL_SUSTAIN_LOOP)) && (q->length > 5000))
            q->flags |= SL_DECLICK;

        q++;
    }

    return 1;
}


// =====================================================================================
    CHAR *STM_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
    CHAR s[20];

    _mm_fseek(mmfile,0,SEEK_SET);
    if(!_mm_read_UBYTES(s,20,mmfile)) return NULL;

    return(DupStr(NULL,s,20));
}


MLOADER load_stm =
{   "STM",
    "Screamtracker 2",

    0,                              // default FULL STEREO panning

    NULL,
    STM_Test,
    STM_Init,
    STM_Cleanup,

    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))STM_Load,/* Not again. */
    STM_LoadTitle
};

