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
 Name: LOAD_M15.C

  15 instrument MOD loader.
  Also supports Ultimate Sound Tracker (old M15 format).

 Portability:
  All systems - all compilers (hopefully)

*/

#include <string.h>
#include "mikmod.h"
#include "uniform.h"


/*************************************************************************
*************************************************************************/

typedef struct MODNOTE
{   UBYTE a,b,c,d;
} MODNOTE;

typedef struct MSAMPINFO       // sample header as it appears in a module
{   CHAR  samplename[22];
    UWORD length;
    UBYTE finetune;
    UBYTE volume;
    UWORD reppos;
    UWORD replen;
} MSAMPINFO;


typedef struct MODULEHEADER          // verbatim module header
{   CHAR       songname[20];         // the songname..
    MSAMPINFO  samples[15];          // all sampleinfo
    UBYTE      songlength;           // number of patterns used
    UBYTE      magic1;               // should be 127
    UBYTE      positions[128];       // which pattern to play at pos
    MODNOTE    *patdat;              // buffer containing all pattern data
} MODULEHEADER;


/*************************************************************************
*************************************************************************/
static CHAR nulls[3]    = { 0, 0, 0 };

typedef struct _H_M15
{   MODULEHEADER  mh;              // raw as-is module header
    //BOOL          ust_loader;      // if TRUE, load as a ust module.
} H_M15;


// =====================================================================================
    void *M15_Init(void)
// =====================================================================================
{
    return _mm_calloc(NULL, 1,sizeof(MODULEHEADER));
}


// =====================================================================================
    void M15_Cleanup(void *handle)
// =====================================================================================
{
    _mm_free(NULL, handle);
}


// =====================================================================================
    static BOOL LoadModuleHeader(MODULEHEADER *mh, MMSTREAM *mmfile)
// =====================================================================================
{
    int t;

    _mm_read_string(mh->songname,20,mmfile);

    for(t=0; t<15; t++)
    {   MSAMPINFO *s = &mh->samples[t];
        _mm_read_string(s->samplename,22,mmfile);
        s->length   =_mm_read_M_UWORD(mmfile);
        s->finetune =_mm_read_UBYTE(mmfile);
        s->volume   =_mm_read_UBYTE(mmfile);
        s->reppos   =_mm_read_M_UWORD(mmfile);
        s->replen   =_mm_read_M_UWORD(mmfile);
    }

    mh->songlength  =_mm_read_UBYTE(mmfile);
    mh->magic1      =_mm_read_UBYTE(mmfile);               // should be 127
    _mm_read_UBYTES(mh->positions,128,mmfile);

    return(!_mm_feof(mmfile));
}


// =====================================================================================
    static int CheckPatternType(MODNOTE *patdat, int numpat)
// =====================================================================================
// Checks the patterns in the modfile for UST / 15-inst indications.  For example, if an
// effect 3xx is found, it is assumed that the song is 15-inst.  If a 1xx effect has dat
// greater than 0x20, it is UST.
//  Returns:  0 indecisive; 1 = UST; 2 = 15-inst
{
    int   t;
    UBYTE eff, dat;

    for(t=0; t<numpat*(64*4); t++, patdat++)
    {   eff = patdat->c & 15;
        dat = patdat->d;
        
        if(eff >= 2)
            return 2;

        if(eff==1)
        {   if(dat > 0x1f) return 1;
            if(dat < 0x3)
                return 2;
        }
        if((eff==2) && (dat > 0x1f)) return 1;
    }

    return 0;
}


// =====================================================================================
    static BOOL M15_DetectType(MODULEHEADER *hdr)
// =====================================================================================
{
    BOOL ust = 0;
    int  t;

    for(t=0; t<15; t++)
    {   // failsafe, UST didn't support samples over 5000 bytes!
        if(hdr->samples[t].length > 4999) return 0;

        // all instrument names should begin with s, st-, or a number
        if(hdr->samples[t].samplename[0] == 's')
        {   if((memcmp(hdr->samples[t].samplename,"st-",3) != 0) &&
               (memcmp(hdr->samples[t].samplename,"ST-",3) != 0) &&
               (memcmp(hdr->samples[t].samplename,nulls,3) != 0))
                ust = 1;
        } else if((hdr->samples[t].samplename[0] < '0') || (hdr->samples[t].samplename[0] > '9')) ust = 1;

        if(((hdr->samples[t].reppos) + hdr->samples[t].replen) > (hdr->samples[t].length + 10)) ust = 1;
    }

    return ust;
}


// =====================================================================================
    BOOL M15_Test(MMSTREAM *mmfile)
// =====================================================================================
{
    int          t;
    MODULEHEADER mh;

    if(!LoadModuleHeader(&mh, mmfile)) return 0;
    if(mh.magic1>127) return 0;

    for(t=0; t<15; t++)
    {   // all finetunes should be zero
        if(mh.samples[t].finetune != 0) return 0;

        // all volumes should be <= 64
        if(mh.samples[t].volume > 64) return 0;
        if(mh.samples[t].length > 32768) return 0;
    }

    return 1;
}


/*
Old (amiga) noteinfo:

 _____byte 1_____   byte2_    _____byte 3_____   byte4_
/                \ /      \  /                \ /      \
0000          0000-00000000  0000          0000-00000000

Upper four    12 bits for    Lower four    Effect command.
bits of sam-  note period.   bits of sam-
ple number.                  ple number.

*/


// =====================================================================================
    static void M15_ConvertNote(UTRK_WRITER *ut, MODNOTE *n, BOOL ust_loader)
// =====================================================================================
{
    unsigned int instrument,effect,effdat,note;
    UWORD        period;

    // extract the various information from the 4 bytes that
    // make up a single note

    instrument = (n->a&0x10) | (n->c>>4);
    period     = (((UWORD)n->a&0xf)<<8)+n->b;
    effect     = n->c&0xf;
    effdat     = n->d;

    // Convert the period to a note number

    note=0;
    if(period != 0)
    {   for(note=0; note<60; note++)
            if(period >= npertab[note]) break;
        note++;
        if(note==61) note = 0;
    }

    if(instrument) utrk_write_inst(ut, instrument);
    if(note)       utrk_write_note(ut, note+24);

    // Convert pattern jump from Dec to Hex
    if(effect == 0xd)
        effdat = (((effdat&0xf0)>>4)*10)+(effdat&0xf);

    if(ust_loader)
    {   switch(effect)
        {   case 0:  break;
            case 1:
                pt_write_effect(ut, 0,effdat);
            break;

            case 2:  
                if(effdat&0xf) pt_write_effect(ut, 1,effdat&0xf);
                if(effdat>>2)  pt_write_effect(ut, 2,effdat>>2);
            break;

            case 3:  break;

            default:
                pt_write_effect(ut, effect,effdat);
            break;
        }
    } else pt_write_effect(ut, effect,effdat);
}


// =====================================================================================
    static BOOL M15_LoadPatterns(UNIMOD *of, MODNOTE *patdat, BOOL ust_loader)
// =====================================================================================
//  Loads all patterns of a modfile and converts them into the
//  3 byte format.
{
    uint t,s;

    if(!AllocPatterns(of)) return 0;
    if(!AllocTracks(of)) return 0;

    // Allocate temporary buffer for loading
    // and converting the patterns

    for(t=0; t<of->numpat; t++)
    {   // Load the pattern into the temp buffer
        // and convert it

        utrk_reset(of->ut);
        for(s=0; s<(64*4); s++, patdat++)
        {   utrk_settrack(of->ut, s & 3);
            M15_ConvertNote(of->ut, patdat, ust_loader);
            if((s & 3) == 3) utrk_newline(of->ut);   // newline every 4 channels!
        }

        if(!utrk_dup_pattern(of->ut, of)) return 0;
    }

    return 1;
}


// =====================================================================================
    BOOL M15_Load(MODULEHEADER *mh, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    uint         t;
    UNISAMPLE   *q;
    MSAMPINFO   *s;           // old module sampleinfo
    BOOL         ust_loader;

    // try to read module header

    if(!LoadModuleHeader(mh, mmfile))
	{   _mmlog("load_m15 > Failure: Unexpected end of file reading module header");
        return 0;
    }

    // set module variables

    of->initspeed = 6;
    of->inittempo = 125;
    of->numchn    = 4;                           // get number of channels
    of->songname  = DupStr(of->allochandle, mh->songname,20);     // make a cstr of songname
    of->numpos    = mh->songlength;              // copy the songlength

    if(!AllocPositions(of, of->numpos)) return 0;
    for(t=0; t<of->numpos; t++)
        of->positions[t] = mh->positions[t];

    // Count the number of patterns

    of->numpat = 0;

    for(t=0; t<128; t++)
    {   if(mh->positions[t] > of->numpat)
            of->numpat = mh->positions[t];
    }
    of->numpat++;
    of->numtrk = of->numpat*4;

    // Load the pattern data into a buffer

    if((mh->patdat = _mm_calloc(NULL, of->numpat*64*4, sizeof(MODNOTE))) == NULL) return 0;
    for(t=0; t<(of->numpat*64*4); t++)
    {   mh->patdat[t].a = _mm_read_UBYTE(mmfile);
        mh->patdat[t].b = _mm_read_UBYTE(mmfile);
        mh->patdat[t].c = _mm_read_UBYTE(mmfile);
        mh->patdat[t].d = _mm_read_UBYTE(mmfile);
    }
    
    // find out if this is a ust or a std soundtracker module...

    ust_loader = M15_DetectType(mh);

    switch(CheckPatternType(mh->patdat, of->numpat))
    {   case 0:   // indecisive, so assume current pick
        break;

        case 1:  ust_loader = 1;   break;
        case 2:  ust_loader = 0;   break;
    }

    if(ust_loader)
        of->modtype = _mm_strdup(of->allochandle, "Ultimate Soundtracker");
    else
        of->modtype = _mm_strdup(of->allochandle, "Soundtracker");


    // Finally, init the sampleinfo structures

    of->numsmp = 15;
    if(!AllocSamples(of, 0)) return 0;

    s = mh->samples;          // init source pointer
    q = of->samples;
    
    for(t=0; t<of->numsmp; t++)
    {   
        // convert the samplename
        q->samplename = DupStr(of->allochandle, s->samplename,22);

        // init the sampleinfo variables and
        // convert the size pointers to longword format

        q->speed     = finetune[s->finetune & 0xf];
        q->volume    = s->volume * 2;

        if(ust_loader)
        {   q->flags     = PSF_UST_LOOP;
            q->loopstart = s->reppos;
        } else
            q->loopstart = s->reppos<<1;

        q->loopend   = q->loopstart+(s->replen<<1);
        q->length    = s->length<<1;

        // Flags?  FLAGS?  Flags!!
        q->format = SF_SIGNED;
        if(s->replen>1 && (q->loopstart < q->length)) q->flags |= SL_LOOP;

        // fix replen if repend>length
        if(q->loopend > q->length) q->loopend = q->length;

        // Check for invalid modules
        if((q->length > 0x7ffff) || (q->loopstart > 0x7ffff) || (q->length > 0x7ffff))
    	{   _mmlog("load_m15 > Failure: Invalid data in module -- length = %d; loopstart = %d", q->length, q->loopstart);
            return 0;
        }
        
        // Enable aggressive declicking for songs that do not loop and that
        // are long enough that they won't be adversely affected.

        if(!(q->flags & (SL_LOOP | SL_SUSTAIN_LOOP)) && (q->length > 5000))
            q->flags |= SL_DECLICK;

        s++;    // point to next source sampleinfo
        q++;
    }

    of->ut = utrk_init(of->numchn, of->allochandle);

    utrk_memory_reset(of->ut);
    utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);

    if(!M15_LoadPatterns(of, mh->patdat, ust_loader)) return 0;

    {
        long  seekpos = _mm_ftell(mmfile);

        q = of->samples;
        for(t=0; t<of->numsmp; t++, q++)
        {
            q->seekpos = seekpos;
            seekpos   += q->length;
        }
    }

    return 1;
}


// =====================================================================================
    CHAR *M15_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
   CHAR s[20];

   _mm_fseek(mmfile,0,SEEK_SET);
   if(!_mm_read_UBYTES(s,22,mmfile)) return NULL;

   return(DupStr(NULL,s,20));
}


MLOADER load_m15 =
{
    "M15",
    "Soundtracker (15-inst)",

    0,                              // default FULL STEREO panning

    NULL,
    M15_Test,
    M15_Init,
    M15_Cleanup,

    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))M15_Load,
    M15_LoadTitle
};
