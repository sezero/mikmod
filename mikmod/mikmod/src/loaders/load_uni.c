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
 Module: LOAD_UNI.C

  UNIMOD (mikmod's internal format) module loader.
  Currently isn't supported at all.  Kinda funny eh? :)

 Portability:
  All systems - all compilers (hopefully)
*/

#include <string.h>
#include "mikmod.h"


BOOL UNI_Test(void)
{
    UBYTE id[4];

    _mm_read_UBYTES(id,4,modfp);
    if(!memcmp(id, "UN07", 4)) return 1;

    return 0;
}


BOOL UNI_Init(void)
{
    return 1;
}


void UNI_Cleanup(void)
{
}


UBYTE *TrkRead(void)
{
    UBYTE  *t;
    UWORD  len;

    len = _mm_read_M_UWORD(modfp);
    t   = (UBYTE *)malloc(len);
    _mm_read_UBYTES(t,len,modfp);

    return t;
}


BOOL UNI_Load(void)
{
    int        t, v, w;
    INSTRUMENT *i;
    SAMPLE     *s;
    EXTSAMPLE  *es;

    // UNI format version 3.01(#7)
    
    of.modtype    = _mm_strdup("MikMod UniFormat 3.01");

    _mm_fseek(modfp, 5, SEEK_SET);    // skip the header
    
    of.flags      = _mm_read_M_UWORD(modfp);
    of.numchn     = _mm_read_UBYTE(modfp);
    of.numvoices  = _mm_read_UBYTE(modfp);
    of.numpos     = _mm_read_M_UWORD(modfp);
    of.numpat     = _mm_read_M_UWORD(modfp);
    of.numtrk     = _mm_read_M_UWORD(modfp);
    if(of.flags & UF_INST) of.numins = _mm_read_M_UWORD(modfp);
    of.numsmp     = _mm_read_M_UWORD(modfp);
    of.reppos     = _mm_read_M_UWORD(modfp);
    of.initspeed  = _mm_read_UBYTE(modfp);
    of.inittempo  = _mm_read_UBYTE(modfp);
    of.initvolume = _mm_read_UBYTE(modfp);

    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    of.songname = StringRead(modfp);
    of.composer = StringRead(modfp);
    of.comment  = StringRead(modfp);

    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    if(!AllocSamples(of.flags & 16)) return 0;
    if(!AllocTracks())               return 0;
    if(!AllocPatterns())             return 0;
    if(!AllocPositions(of.numpos))   return 0;

    _mm_read_M_UWORDS(of.positions, of.numpos, modfp);
    _mm_read_M_SWORDS(of.panning, of.numchn, modfp);
    _mm_read_UBYTES(of.chanvol, of.numchn, modfp);


    // Load sample headers

    s  = of.samples;
    es = of.extsamples;

    for(v=0; v<of.numsmp; v++, s++, es++)
    {   s->flags      = _mm_read_M_UWORD(modfp) | SF_SIGNED | SF_BIG_ENDIAN | SF_DELTA;
        s->speed      = _mm_read_M_ULONG(modfp);
        s->length     = _mm_read_M_ULONG(modfp);

        if(s->flags & SF_LOOP)
        {   s->loopstart  = _mm_read_M_ULONG(modfp);
            s->loopend    = _mm_read_M_ULONG(modfp);
        }

        if(s->flags & SF_SUSTAIN)
        {   s->susbegin   = _mm_read_M_ULONG(modfp);
            s->susend     = _mm_read_M_ULONG(modfp);
        }
        
        s->volume = _mm_read_UBYTE(modfp);
        if(s->flags & SF_OWNPAN) s->panning = _mm_read_M_UWORD(modfp);

        // here begins 'like' information (both samples and insts have the
        // same block of data types at the end of each header).

        if(of.flags & 16)
        {   es->globvol = _mm_read_UBYTE(modfp);

            if(s->flags & 4096)
            {   es->pitpansep    = _mm_read_UBYTE(modfp);
                es->pitpancenter = _mm_read_UBYTE(modfp);
            }
    
            es->rvolvar = _mm_read_UBYTE(modfp);
            es->rpanvar = _mm_read_UBYTE(modfp);
            es->volfade = _mm_read_M_UWORD(modfp);
    
            if(s->flags & 2048)
            {   es->vibflags   = _mm_read_UBYTE(modfp);
                es->vibtype    = _mm_read_UBYTE(modfp);
                es->vibsweep   = _mm_read_UBYTE(modfp);
                es->vibdepth   = _mm_read_UBYTE(modfp);
                es->vibrate    = _mm_read_UBYTE(modfp);
            }
    
            // Load in envelope data (where present!)
    
            if(s->flags & 8192)
            {   // Load the volume envelope
    
                es->volflg       = _mm_read_UBYTE(modfp);
                es->volpts       = _mm_read_UBYTE(modfp);
                es->volsusbeg    = _mm_read_UBYTE(modfp);
                es->volsusend    = _mm_read_UBYTE(modfp);
                es->volbeg       = _mm_read_UBYTE(modfp);
                es->volend       = _mm_read_UBYTE(modfp);
        
                for(w=0; w<es->volpts; w++)
                {   es->volenv[w].pos = _mm_read_M_UWORD(modfp);
                    es->volenv[w].val = _mm_read_UBYTE(modfp);
                }
            }
    
            if(s->flags & 16384l)
            {   // Load the panning envelope
                
                es->panflg    = _mm_read_UBYTE(modfp);
                es->panpts    = _mm_read_UBYTE(modfp);
                es->pansusbeg = _mm_read_UBYTE(modfp);
                es->pansusend = _mm_read_UBYTE(modfp);
                es->panbeg    = _mm_read_UBYTE(modfp);
                es->panend    = _mm_read_UBYTE(modfp);
        
                for(w=0; w<es->panpts; w++)
                {   es->panenv[w].pos = _mm_read_M_SWORD(modfp);
                    es->panenv[w].val = _mm_read_M_SWORD(modfp);
                }
            }
    
            if(s->flags & 32768l)
            {   // Load the pitch envelope
    
                es->pitflg    = _mm_read_UBYTE(modfp);
                es->pitpts    = _mm_read_UBYTE(modfp);
                es->pitsusbeg = _mm_read_UBYTE(modfp);
                es->pitsusend = _mm_read_UBYTE(modfp);
                es->pitbeg    = _mm_read_UBYTE(modfp);
                es->pitend    = _mm_read_UBYTE(modfp);
        
                for(w=0; w<es->pitpts; w++)
                {   es->pitenv[w].pos = _mm_read_M_SWORD(modfp);
                    es->pitenv[w].val = _mm_read_SBYTE(modfp);
                }
            }
        }

        s->samplename = StringRead(modfp);

        if(feof(modfp))
        {   _mm_errno = MMERR_LOADING_HEADER;
            return 0;
        }

    }

    // Load instruments

    if(of.flags & UF_INST)
    {   if(!AllocInstruments()) return 0;
        i = of.instruments;

        for(v=0; v<of.numins; v++, i++)
        {   i->flags        = _mm_read_UBYTE(modfp);
            i->nnatype      = _mm_read_UBYTE(modfp);
            i->dct          = _mm_read_UBYTE(modfp);
            i->dca          = _mm_read_UBYTE(modfp);

            _mm_read_UBYTES(i->samplenumber, 120, modfp);
            _mm_read_UBYTES(i->samplenote, 120, modfp);
    
            if(i->flags & IF_OWNPAN) i->panning = _mm_read_M_UWORD(modfp);

            // here begins 'like' information (both samples and insts have the
            // same block of data types at the end of each header).

            i->globvol      = _mm_read_UBYTE(modfp);

            if(i->flags & 4)
            {   i->pitpansep    = _mm_read_UBYTE(modfp);
                i->pitpancenter = _mm_read_UBYTE(modfp);
            }

            i->rvolvar      = _mm_read_UBYTE(modfp);
            i->rpanvar      = _mm_read_UBYTE(modfp);
            i->volfade      = _mm_read_M_UWORD(modfp);
    
            if(i->flags & 2)
            {   i->vibflags   = _mm_read_UBYTE(modfp);
                i->vibtype    = _mm_read_UBYTE(modfp);
                i->vibsweep   = _mm_read_UBYTE(modfp);
                i->vibdepth   = _mm_read_UBYTE(modfp);
                i->vibrate    = _mm_read_UBYTE(modfp);
            }

            if(i->flags & 8)
            {   // Load the volume envelope

                i->volflg       = _mm_read_UBYTE(modfp);
                i->volpts       = _mm_read_UBYTE(modfp);
                i->volsusbeg    = _mm_read_UBYTE(modfp);
                i->volsusend    = _mm_read_UBYTE(modfp);
                i->volbeg       = _mm_read_UBYTE(modfp);
                i->volend       = _mm_read_UBYTE(modfp);
        
                for(w=0; w<i->volpts; w++)
                {   i->volenv[w].pos = _mm_read_M_SWORD(modfp);
                    i->volenv[w].val = _mm_read_M_UWORD(modfp);
                }
            }
            
            if(i->flags & 16)
            {   // Load the panning envelope

                i->panflg    = _mm_read_UBYTE(modfp);
                i->panpts    = _mm_read_UBYTE(modfp);
                i->pansusbeg = _mm_read_UBYTE(modfp);
                i->pansusend = _mm_read_UBYTE(modfp);
                i->panbeg    = _mm_read_UBYTE(modfp);
                i->panend    = _mm_read_UBYTE(modfp);
        
                for(w=0; w<i->panpts; w++)
                {   i->panenv[w].pos = _mm_read_M_SWORD(modfp);
                    i->panenv[w].val = _mm_read_M_SWORD(modfp);
                }
            }

            if(i->flags & 32)
            {   // Load the pitch envelope
            
                i->pitflg    = _mm_read_UBYTE(modfp);
                i->pitpts    = _mm_read_UBYTE(modfp);
                i->pitsusbeg = _mm_read_UBYTE(modfp);
                i->pitsusend = _mm_read_UBYTE(modfp);
                i->pitbeg    = _mm_read_UBYTE(modfp);
                i->pitend    = _mm_read_UBYTE(modfp);
        
                for(w=0; w<i->pitpts; w++)
                {   i->pitenv[w].pos = _mm_read_M_SWORD(modfp);
                    i->pitenv[w].val = _mm_read_SBYTE(modfp);
                }
            }

            i->insname = StringRead(modfp);

            if(feof(modfp))
            {   _mm_errno = MMERR_LOADING_HEADER;
                return 0;
            }
        }
    }

    // Read patterns

    _mm_read_M_UWORDS(of.pattrows, of.numpat, modfp);
    _mm_read_M_UWORDS(of.patterns, of.numpat * of.numchn, modfp);

    // Read tracks

    for(t=0; t<of.numtrk; t++)
        of.tracks[t] = TrkRead();

    return 1;
}


CHAR *UNI_LoadTitle(void)
{
    _mm_fseek(modfp,24,SEEK_SET);
    return(StringRead(modfp));
}


MLOADER load_uni =
{   "UNI",
    "Portable UniFormat 3.0 Loader",

    NULL,
    UNI_Init,
    UNI_Test,
    UNI_Load,
    UNI_Cleanup,
    UNI_LoadTitle
};

