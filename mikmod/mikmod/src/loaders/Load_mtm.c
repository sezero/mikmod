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
 Module:  LOAD_MTM.C

  MTM module loader.  Multi-Tracker is actually a continuation of Tran's
  .669 tracker, by StarScream.  It was made more PT-style, and hence a lot
  easier to support.

 Portability:
  All systems - all compilers (hopefully)

*/

#include <string.h>
#include "mikmod.h"
#include "uniform.h"


/**************************************************************************
**************************************************************************/


typedef struct MTMSAMPLE
{   
    CHAR  samplename[22];
    ULONG length;
    ULONG reppos;
    ULONG repend;
    UBYTE finetune;
    UBYTE volume;
    UBYTE attribute;
} MTMSAMPLE;



typedef struct MTMHEADER
{   
    UBYTE id[3];                    // MTM file marker
    UBYTE version;                  // upper major, lower nibble minor version number
    char  songname[20];             // ASCIIZ songname
    UWORD numtracks;                // number of tracks saved
    UBYTE lastpattern;              // last pattern number saved
    UBYTE lastorder;                // last order number to play (songlength-1)
    UWORD commentsize;              // length of comment field
    UBYTE numsamples;               // number of samples saved 
    UBYTE attribute;                // attribute byte (unused)
    UBYTE beatspertrack;            //
    UBYTE numchannels;              // number of channels used 
    UBYTE panpos[32];               // voice pan positions
} MTMHEADER;


/**************************************************************************
**************************************************************************/

static char MTM_Version[] = "MTM";

typedef struct _MTM_HANDLE
{   
    MTMHEADER   mh;
    UWORD       pat[32];
    UBYTE     **globtrack;
    int         numtrk;
    MM_ALLOC   *allochandle;
} MTM_HANDLE;


// =====================================================================================
    BOOL MTM_Test(MMSTREAM *mmfile)
// =====================================================================================
{
    UBYTE id[3];
    if(!_mm_read_UBYTES(id,3,mmfile)) return 0;
    if(!memcmp(id,"MTM",3)) return 1;
    return 0;
}


// =====================================================================================
    void *MTM_Init(void)
// =====================================================================================
{
    MTM_HANDLE   *retval;
    MM_ALLOC     *allochandle;

    allochandle = _mmalloc_create("Load_MTM", NULL);
    retval = (MTM_HANDLE *)_mm_calloc(allochandle, 1,sizeof(MTM_HANDLE));
    retval->allochandle = allochandle;

    return retval;
}


// =====================================================================================
    void MTM_Cleanup(MTM_HANDLE *handle)
// =====================================================================================
{
    if(handle) _mmalloc_close(handle->allochandle);
}


// =====================================================================================
    BOOL MTM_Load(MTM_HANDLE *h, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    MTMSAMPLE   s;
    UNISAMPLE  *q;

    uint        t;

    // try to read module header 

    _mm_read_UBYTES(h->mh.id,3,mmfile);
    h->mh.version       =_mm_read_UBYTE(mmfile);
    _mm_read_string(h->mh.songname,20,mmfile);
    h->mh.numtracks     =_mm_read_I_UWORD(mmfile);
    h->mh.lastpattern   =_mm_read_UBYTE(mmfile);
    h->mh.lastorder     =_mm_read_UBYTE(mmfile);
    h->mh.commentsize   =_mm_read_I_UWORD(mmfile);
    h->mh.numsamples    =_mm_read_UBYTE(mmfile);
    h->mh.attribute     =_mm_read_UBYTE(mmfile);
    h->mh.beatspertrack =_mm_read_UBYTE(mmfile);
    h->mh.numchannels   =_mm_read_UBYTE(mmfile);
    _mm_read_UBYTES(h->mh.panpos,32,mmfile);

    if(_mm_feof(mmfile))
	{   _mmlog("load_mtm > Failure: Unexpected end of file reading module header");
        return 0;
    }

    // set module variables

    of->initspeed = 6;
    of->inittempo = 125;
    of->modtype   = _mm_strdup(of->allochandle, MTM_Version);
    of->numchn    = h->mh.numchannels;
    of->numtrk    = h->mh.numtracks+1;         // unique (or maybe not) tracks in song
    of->songname  = DupStr(of->allochandle, h->mh.songname,20); // make a cstr of songname
    of->numpos    = h->mh.lastorder+1;         // copy the songlength
    of->numpat    = h->mh.lastpattern+1;
    for(t=0; t<32; t++) of->panning[t] = (h->mh.panpos[t] * 16) + PAN_LEFT;

    of->numsmp = h->mh.numsamples;
    if(!AllocSamples(of, 0)) return 0;
    
    q = of->samples;
    
    for(t=0; t<of->numsmp; t++)
    {   // try to read sample info
        _mm_read_string(s.samplename,22,mmfile);
        s.length    =_mm_read_I_ULONG(mmfile);
        s.reppos    =_mm_read_I_ULONG(mmfile);
        s.repend    =_mm_read_I_ULONG(mmfile);
        s.finetune  =_mm_read_UBYTE(mmfile);
        s.volume    =_mm_read_UBYTE(mmfile);
        s.attribute =_mm_read_UBYTE(mmfile);

        if(_mm_feof(mmfile))
		{   _mmlog("load_mtm > Failure: Unexpected end of file reading sample header %d",t);
            return 0;
        }

        q->samplename = DupStr(of->allochandle, s.samplename,22);

        q->speed      = finetune[s.finetune];
        q->length     = s.length;
        q->loopstart  = s.reppos;
        q->loopend    = s.repend;
        q->volume     = s.volume * 2;

        if((s.repend-s.reppos) > 2) q->flags |= SL_LOOP;

        if(s.attribute & 1)
        {   // If the sample is 16-bits, convert the length
            // and replen byte-values into sample-values

            q->format     |= SF_16BITS;
            q->length    >>= 1;
            q->loopstart >>= 1;
            q->loopend   >>= 1;
        }

        q++;
    }

    if(!AllocPositions(of, of->numpos)) return 0;
    for(t=0; t<of->numpos; t++)
        of->positions[t] = _mm_read_UBYTE(mmfile);

    for(; t<128; t++)  _mm_read_UBYTE(mmfile);

    if(_mm_feof(mmfile))
  	{   _mmlog("load_mtm > Failure: Unexpected end of file loading module header.");
        return 0;
    }

    if(!AllocTracks(of)) return 0;
    if(!AllocPatterns(of)) return 0;

    // MTMs store pattens per-track like unimod!
    // Which also means we have to do some evil stuff to extract the global
    // effects from the track data properly.  What we do is extract all
    // global effects into their own track (ie, there are of->numtrk global
    // tracks), then combine those tracks as they are ordered for each
    // pattern.  Sucks, huh?

    of->ut = utrk_init(1, h->allochandle);
    utrk_memory_reset(of->ut);
    utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);

    if(!(h->globtrack = (UBYTE **)_mm_calloc(h->allochandle, h->numtrk = of->numtrk, sizeof(UBYTE *)))) return 0;

    of->tracks[0] = utrk_blanktrack;

    for(t=1; t<of->numtrk; t++)
    {   int        s;
        UBYTE      a,b,c;
        uint       inst,note,eff,dat;

        utrk_reset(of->ut);
        for(s=0; s<64; s++)
        {   a = _mm_read_UBYTE(mmfile);
            b = _mm_read_UBYTE(mmfile);
            c = _mm_read_UBYTE(mmfile);
            inst = ((a&0x3)<<4) | (b>>4);
            note = a>>2;

            eff = b & 0xf;
            dat = c;

            utrk_write_inst(of->ut, inst);
            if(note)  utrk_write_note(of->ut, note+25);

            // mtm bug bugfix: when the effect is volslide,
            // slide-up _always_ overrides slide-dn.

            if((eff == 0xa) && (dat & 0xf0)) dat &= 0xf0;

            // Convert pattern jump from Dec to Hex
            // and convert tempo (subtract 20)

            if(eff == 0xd)
                dat = (((dat&0xf0)>>4)*10) + (dat&0xf);

            // sample offset gets special treatment:
            // MTM sample offsets-without-note is not ignored like in most everything else.
            // So, we use the new-and-improved sample offset shizat!

            if(eff == 0x9)
            {   UNITRK_EFFECT  effdat;

                if(dat)
                {   effdat.effect    = UNI_OFFSET;
                    effdat.framedly  = UFD_RUNONCE;
                    effdat.param.s   = dat<<8;
                    utrk_write_local(of->ut, &effdat, PTMEM_OFFSET);
                } else utrk_memory_local(of->ut, &effdat, PTMEM_OFFSET, 0);
            } else pt_write_effect(of->ut, eff,dat);
            utrk_newline(of->ut);
        }

        if(_mm_feof(mmfile))
		{   _mmlog("load_mtm > Failure: Unexpected end of file reading track data",t);
            return 0;
        }

        of->tracks[t]   = utrk_dup_track(of->ut, 0, of->allochandle);
        h->globtrack[t] = utrk_dup_global(of->ut, h->allochandle);
    }

    for(t=0; t<of->numpat; t++)
    {   uint uscrap;   // That's unsigned crap to you too fella.
        _mm_read_I_UWORDS(h->pat,32,mmfile);
        for(uscrap=0; uscrap<of->numchn; uscrap++)
            of->patterns[(t*of->numchn)+uscrap] = (h->pat[uscrap] <= of->numtrk) ? h->pat[uscrap] : 0;
    }

    // Pattern setup is read, so now process global tracks.
    pt_global_consolidate(of, h->globtrack);

    // read comment field
    
    if(h->mh.commentsize)
    {   if(!(of->comment=(CHAR *)_mm_malloc(of->allochandle, h->mh.commentsize+1))) return 0;
        _mm_read_UBYTES(of->comment,h->mh.commentsize,mmfile);
        of->comment[h->mh.commentsize] = 0;
    }


    {
        long  seekpos = _mm_ftell(mmfile);

        q = of->samples;
        for(t=0; t<of->numsmp; t++, q++)
        {
            q->seekpos = seekpos;
            seekpos   += q->length * ((q->format & SF_16BITS) ? 2 : 1);
        }
    }

    return 1;
}


// =====================================================================================
    CHAR *MTM_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
    CHAR s[20];

    _mm_fseek(mmfile,4,SEEK_SET);
    if(!_mm_read_UBYTES(s,20,mmfile)) return NULL;
 
    return(DupStr(NULL, s,20));
}
                

MLOADER load_mtm =
{   "MTM",
    "Multitracker",

    0,                              // default FULL STEREO panning

    NULL,
    MTM_Test,
    MTM_Init,
    (void (*)(ML_HANDLE *))MTM_Cleanup,

    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))MTM_Load,
    MTM_LoadTitle
};

