/*

 MikMod Sound System

  By Jake Stine of Divine Entertainment (1996-1999)

 Support:
  If you find problems with this code, send mail to:
    air@divent.simplenet.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Module: LOAD_MED.C

 Description:
  Amiga MED module loader.  This piece of junk doesn't work at all.  I'd love
  make it work.  Someone send me ebtetr docs on this format!

 Portability:
  All systems - all compilers (hopefully)

*/

#include <string.h>
#include "mikmod.h"

#define MMD0_string 0x4D4D4430
#define MMD1_string 0x4D4D4431

typedef struct MMD0
{   ULONG   id;
    ULONG   modlen;
    ULONG   MMD0songP;        // struct MMD0song *song;
    UWORD   psecnum;          // for the player routine, MMD2 only
    UWORD   pseq;             //  "   "   "   "
    ULONG   MMD0BlockPP;      // struct MMD0Block **blockarr;
    ULONG   reserved1;
    ULONG   InstrHdrPP;       // struct InstrHdr **smplarr;
    ULONG   reserved2;
    ULONG   MMD0expP;         // struct MMD0exp *expdata;
    ULONG   reserved3;
    UWORD   pstate;           // some data for the player routine
    UWORD   pblock;
    UWORD   pline;
    UWORD   pseqnum;
    SWORD   actplayline;
    UBYTE   counter;
    UBYTE   extra_songs;      // number of songs - 1
} MMD0;


typedef struct MMD0sample
{   UWORD rep,replen;        // offs: 0(s), 2(s)
    UBYTE midich;            // offs: 4(s)
    UBYTE midipreset;        // offs: 5(s)
    UBYTE svol;              // offs: 6(s)
    SBYTE strans;            // offs: 7(s)
} MMD0sample;


typedef struct MMD0song
{   MMD0sample sample[63];   // 63 * 8 bytes = 504 bytes
    UWORD   numblocks;       // offs: 504
    UWORD   songlen;         // offs: 506
    UBYTE   playseq[256];    // offs: 508
    UWORD   deftempo;        // offs: 764
    SBYTE   playtransp;      // offs: 766
    UBYTE   flags;           // offs: 767
    UBYTE   flags2;          // offs: 768
    UBYTE   tempo2;          // offs: 769
    UBYTE   trkvol[16];      // offs: 770
    UBYTE   mastervol;       // offs: 786
    UBYTE   numsamples;      // offs: 787
} MMD0song;


typedef struct MMD0NOTE
{  UBYTE a,b,c;
} MMD0NOTE;


typedef struct MMD1NOTE
{   UBYTE a,b,c,d;
} MMD1NOTE;


typedef struct InstrHdr
{   ULONG   length;
    SWORD   type;
    // Followed by actual data
} InstrHdr;


static MMD0 *mh     = NULL;
static MMD0song *ms = NULL;
static ULONG *ba    = NULL;

static CHAR MED_Version[] = "MED";


BOOL MED_Test(void)
{
    UBYTE id[4];

    if(!_mm_read_UBYTES(id,4,modfp)) return 0;
    if(!memcmp(id,"MMD0",4)) return 1;
    if(!memcmp(id,"MMD1",4)) return 1;
    return 0;
}


BOOL MED_Init(void)
{
    if(!(mh=(MMD0 *)_mm_calloc(1,sizeof(MMD0)))) return 0;
    if(!(ms=(MMD0song *)_mm_calloc(1,sizeof(MMD0song)))) return 0;
    return 1;
}


void MED_Cleanup(void)
{
    if(mh) _mm_free(mh);
    if(ms) _mm_free(ms);
    if(ba) _mm_free(ba);

    mh = NULL;
    ms = NULL;
    ba = NULL;        // blockarr
}


void EffectCvt(UBYTE eff,UBYTE dat)
{
    switch(eff)
    {   // 0x0 0x1 0x2 0x3 0x4      // PT effects
        case 0x5:       // PT vibrato with speed/depth nibbles swapped
           UniPTEffect(0x4,(dat>>4) | ((dat&0xf)<<4) );
        break;

        case 0x6:       // not used
        case 0x7:       // not used
        case 0x8:       // midi hold/decay
        break;

        case 0x9:
           if(dat<0x20) UniPTEffect(0xf,dat);
        break;

        // 0xa 0xb 0xc all PT effects

        case 0xd:       // same as PT volslide
           UniPTEffect(0xa,dat);
        break;

        case 0xe:       // synth jmp - midi
        break;

        case 0xf:
           // F00 does patternbreak with med
           if(dat==0) UniPTEffect(0xd,0);
           else if(dat<=0xa) UniPTEffect(0xf,dat);
           else if(dat<0xf1) UniPTEffect(0xf,((UWORD)dat*125)/33);
           else if(dat==0xff) UniPTEffect(0xc,0);  // stop note
        break;

        default:        // all normal PT effects are handled here :)
           // Convert pattern jump from Dec to Hex
           if(eff == 0xd)
               dat = (((dat&0xf0)>>4)*10)+(dat&0xf);
           UniPTEffect(eff,dat);
        break;
    }
}

BOOL LoadMMD0Patterns(void)
{
    int      t,row,col;
    UWORD    numtracks,numlines,maxlines=0,track=0;
   
    // first, scan patterns to see how many channels are used
    for(t=0; t<of.numpat; t++)
    {   _mm_fseek(modfp,ba[t],SEEK_SET);
        numtracks = _mm_read_UBYTE(modfp);
        numlines  = _mm_read_UBYTE(modfp);

        if(numtracks>of.numchn) of.numchn = numtracks;
        if(numlines>maxlines) maxlines = numlines;
    }

    of.numtrk = of.numpat*of.numchn;
    if(!AllocTracks()) return 0;
    if(!AllocPatterns()) return 0;

    // second read: no more mr. nice guy,
    // really read and convert patterns

    for(t=0; t<of.numpat; t++)
    {   _mm_fseek(modfp,ba[t],SEEK_SET);
        numtracks = _mm_read_UBYTE(modfp);
        numlines  = _mm_read_UBYTE(modfp);

        of.pattrows[t] = numlines+1;
        UniReset();
        for(row=numlines+1; row; row--)
        {   UBYTE a,b,c,inst,note,eff,dat;
            for(col; col<numtracks; col++)
            {   UniSetTrack(col);
                a = _mm_read_UBYTE(modfp);
                b = _mm_read_UBYTE(modfp);
                c = _mm_read_UBYTE(modfp);

                note = a & 0x3f;
                a  >>= 6;
                a    = ((a & 1) << 1) | (a >> 1);

                inst = (b >> 4) | (a << 4);
                eff  = b & 0xf;
                dat  = c;

                if(inst!=0) UniInstrument(inst-1);
                if(note!=0) UniNote(note+35);

                EffectCvt(eff,dat);
            }
            UniNewline();
        }

        for(col=0; col<of.numchn; col++)
        {   of.tracks[track] = UniDup(col);
            track++;
        }
     }
     return 1;
}


BOOL LoadMMD1Patterns(void)
{
    int      t,row,col;
    UWORD    numtracks,numlines,maxlines=0,track=0;

    // first, scan patterns to see how many channels are used
    for(t=0; t<of.numpat; t++)
    {  _mm_fseek(modfp,ba[t],SEEK_SET);
       numtracks = _mm_read_M_UWORD(modfp);
       numlines  = _mm_read_M_UWORD(modfp);

       _mm_fseek(modfp,sizeof(ULONG),SEEK_CUR);
       if(numtracks>of.numchn) of.numchn = numtracks;
       if(numlines>maxlines) maxlines = numlines;
    }

    of.numtrk = of.numpat*of.numchn;
    if(!AllocTracks()) return 0;
    if(!AllocPatterns()) return 0;

    // second read: no more mr. nice guy, really read and convert patterns
    for(t=0; t<of.numpat; t++)
    {   _mm_fseek(modfp,ba[t],SEEK_SET);
        numtracks = _mm_read_M_UWORD(modfp);
        numlines  = _mm_read_M_UWORD(modfp);

        _mm_fseek(modfp,sizeof(ULONG),SEEK_CUR);
        of.pattrows[t] = numlines;

        UniReset();
        for(row=numlines+1; row; row--)
        {   UBYTE a,b,c,d,inst,note,eff,dat;
            for(col=0; col<numtracks; col++)
            {   UniSetTrack(col);
                a = _mm_read_UBYTE(modfp);
                b = _mm_read_UBYTE(modfp);
                c = _mm_read_UBYTE(modfp);
                d = _mm_read_UBYTE(modfp);

                note = a & 0x7f;
                inst = b & 0x3f;
                eff  = c & 0xf;
                dat  = d;

                if(inst!=0) UniInstrument(inst-1);
                if(note!=0) UniNote(note+23);

                EffectCvt(eff,dat);
            }
            UniNewline();
        }

        for(col=0;col<of.numchn;col++)
        {   of.tracks[track]=UniDup(col);
            track++;
        }
    }
    return 1;
}



BOOL MED_Load(void)
{
    int        t;
    ULONG      sa[64];
    InstrHdr   s;
    SAMPLE     *q;
    MMD0sample *mss;

    // try to read module header

    mh->id          = _mm_read_M_ULONG(modfp);
    mh->modlen      = _mm_read_M_ULONG(modfp);
    mh->MMD0songP   = _mm_read_M_ULONG(modfp);
    mh->psecnum     = _mm_read_M_UWORD(modfp);
    mh->pseq        = _mm_read_M_UWORD(modfp);
    mh->MMD0BlockPP = _mm_read_M_ULONG(modfp);
    mh->reserved1   = _mm_read_M_ULONG(modfp);
    mh->InstrHdrPP  = _mm_read_M_ULONG(modfp);
    mh->reserved2   = _mm_read_M_ULONG(modfp);
    mh->MMD0expP    = _mm_read_M_ULONG(modfp);
    mh->reserved3   = _mm_read_M_ULONG(modfp);
    mh->pstate      = _mm_read_M_UWORD(modfp);
    mh->pblock      = _mm_read_M_UWORD(modfp);
    mh->pline       = _mm_read_M_UWORD(modfp);
    mh->pseqnum     = _mm_read_M_UWORD(modfp);
    mh->actplayline = _mm_read_M_SWORD(modfp);
    mh->counter     = _mm_read_UBYTE(modfp);
    mh->extra_songs = _mm_read_UBYTE(modfp);

    // Seek to MMD0song struct
    _mm_fseek(modfp,mh->MMD0songP,SEEK_SET);


    // Load the MMD0 Song Header

    mss = ms->sample;     // load the sample data first
    for(t=63; t; t--, mss++)
    {   mss->rep        = _mm_read_M_UWORD(modfp);
        mss->replen     = _mm_read_M_UWORD(modfp);
        mss->midich     = _mm_read_UBYTE(modfp);
        mss->midipreset = _mm_read_UBYTE(modfp);
        mss->svol       = _mm_read_UBYTE(modfp);
        mss->strans     = _mm_read_SBYTE(modfp);
    }

    ms->numblocks  = _mm_read_M_UWORD(modfp);
    ms->songlen    = _mm_read_M_UWORD(modfp);
    _mm_read_UBYTES(ms->playseq,256,modfp);
    ms->deftempo   = _mm_read_M_UWORD(modfp);
    ms->playtransp = _mm_read_SBYTE(modfp);
    ms->flags      = _mm_read_UBYTE(modfp);
    ms->flags2     = _mm_read_UBYTE(modfp);
    ms->tempo2     = _mm_read_UBYTE(modfp);
    _mm_read_UBYTES(ms->trkvol,16,modfp);
    ms->mastervol  = _mm_read_UBYTE(modfp);
    ms->numsamples = _mm_read_UBYTE(modfp);

    // check for a bad header
    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    // seek to and read the samplepointer array
    _mm_fseek(modfp,mh->InstrHdrPP,SEEK_SET);
    if(!_mm_read_M_ULONGS(sa,ms->numsamples,modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    // alloc and read the blockpointer array
    if(!(ba=(ULONG *)_mm_calloc(ms->numblocks, sizeof(ULONG)))) return 0;
    _mm_fseek(modfp,mh->MMD0BlockPP,SEEK_SET);
    if(!_mm_read_M_ULONGS(ba,ms->numblocks,modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }


    // copy song positions
    if(!AllocPositions(ms->songlen)) return 0;
    for(t=0; t<ms->songlen; t++)
       of.positions[t] = ms->playseq[t];

    of.initspeed = 6;
    of.inittempo = 125;
//    of.inittempo = ((UWORD)ms->deftempo*125)/33;
    of.modtype   = _mm_strdup(MED_Version);
    of.numchn    = 0;                         // will be counted later
    of.numpat    = ms->numblocks;
    of.numpos    = ms->songlen;
    of.numsmp    = ms->numsamples;

    if(!AllocSamples(0)) return 0;
    q = of.samples;
   
    for(t=0; t<of.numsmp; t++)
    {   _mm_fseek(modfp,sa[t],SEEK_SET);
        s.length = _mm_read_M_ULONG(modfp);
        s.type   = _mm_read_M_SWORD(modfp);
       
        if(feof(modfp))
        {   _mm_errno = MMERR_LOADING_SAMPLEINFO;
            return 0;
        }

        q->samplename = NULL;
        q->length     = s.length;
        q->seekpos    = _mm_ftell(modfp);
        q->loopstart  = ms->sample[t].rep<<1;
        q->loopend    = q->loopstart+(ms->sample[t].replen<<1);
        q->flags      = SF_SIGNED;
        q->speed      = 8363;
        q->volume     = 64;

        if(ms->sample[t].replen > 1) q->flags|=SF_LOOP;

        // don't load sample if length>='MMD0' hah.. hah.. very funny.. NOT!
        if(q->length >= MMD0_string) q->length = 0;

        q++;
    }


    if(mh->id==MMD0_string)
    {   if(!LoadMMD0Patterns()) return 0;
    } else if(mh->id==MMD1_string)
    {   if(!LoadMMD1Patterns()) return 0;
    } else
    {   _mm_errno = MMERR_NOT_A_MODULE;
        return 0;
    }
   
    return 1;
}


MLOADER load_med =
{   "MED",
    "MED loader v0.1",

    NULL,
    MED_Init,
    MED_Test,
    MED_Load,
    MED_Cleanup,
    NULL
};


