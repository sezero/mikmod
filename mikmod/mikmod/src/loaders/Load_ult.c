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
 Module:  LOAD_ULT.C

  Ultratracker (ULT) module loader.  I used to have a couple cool .ULTs, but
  lost them some time ago.  Hope this loader still works! :)

 Portability:
  All systems - all compilers (hopefully)

 If this module is found to not be portable to any particular platform,
 please contact Jake Stine at dracoirs@epix.net (see MIKMOD.TXT for
 more information on contacting the author).

*/

#include <string.h>
#include "mikmod.h"
#include "uniform.h"


#define ULTS_16BITS     4
#define ULTS_LOOP       8
#define ULTS_REVERSE    16


// Raw ULT header struct:

typedef struct ULTHEADER
{   CHAR  id[16];
    CHAR  songtitle[32];
    UBYTE reserved;
} ULTHEADER;


// Raw ULT sampleinfo struct:

typedef struct ULTSAMPLE
{   CHAR   samplename[32];
    CHAR   dosname[12];
    SLONG  loopstart;
    SLONG  loopend;
    SLONG  sizestart;
    SLONG  sizeend;
    UBYTE  volume;
    UBYTE  flags;
    SWORD  finetune;
} ULTSAMPLE;


typedef struct ULTEVENT
{   UBYTE note,sample,eff,dat1,dat2;
} ULTEVENT;


CHAR *ULT_Version[]=
{   "Ultra Tracker V1.3",
    "Ultra Tracker V1.4",
    "Ultra Tracker V1.5",
    "Ultra Tracker V1.6"
};



// =====================================================================================
    BOOL ULT_Test(MMSTREAM *mmfile)
// =====================================================================================
{
    CHAR id[16];

    if(!_mm_read_string(id,15,mmfile)) return 0;
    return(!strncmp(id,"MAS_UTrack_V00",14));
}

typedef struct ULT_HANDLE
{
    ULTHEADER   mh;
    uint        numtrk;
    UBYTE     **globtrack;
    MM_ALLOC   *allochandle;
} ULT_HANDLE;


// =====================================================================================
    void *ULT_Init(void)
// =====================================================================================
{
    ULT_HANDLE   *retval;
    MM_ALLOC     *allochandle;

    allochandle = _mmalloc_create("Load_MTM", NULL);
    retval = (ULT_HANDLE *)_mm_calloc(allochandle, 1,sizeof(ULT_HANDLE));
    retval->allochandle = allochandle;

    return retval;
}


// =====================================================================================
    void ULT_Cleanup(ULT_HANDLE *handle)
// =====================================================================================
{
    if(handle) _mmalloc_close(handle->allochandle);
}


// =====================================================================================
    int ReadUltEvent(ULTEVENT *event, MMSTREAM *mmfile)
// =====================================================================================
{
    UBYTE flag,rep=1;

    flag = _mm_read_UBYTE(mmfile);

    if(flag==0xfc)
    {   rep = _mm_read_UBYTE(mmfile);
        event->note =_mm_read_UBYTE(mmfile);
    } else
        event->note = flag;

    event->sample   =_mm_read_UBYTE(mmfile);
    event->eff      =_mm_read_UBYTE(mmfile);
    event->dat1     =_mm_read_UBYTE(mmfile);
    event->dat2     =_mm_read_UBYTE(mmfile);

    return rep;
}


// =====================================================================================
    BOOL ULT_Load(ULT_HANDLE *handle, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    uint        t,tracks=0;
    uint        u;
    UNISAMPLE  *q;
    ULTSAMPLE   s;
    UBYTE       nos,noc,nop;
    ULTEVENT    ev;

    ULTHEADER  *mh = &handle->mh;

    // try to read module header

    _mm_read_string(mh->id,15,mmfile);
    _mm_read_string(mh->songtitle,32,mmfile);
    mh->reserved = _mm_read_UBYTE(mmfile);

    if(_mm_feof(mmfile))
    {   _mmlogd("load_ult > Unexpected end of file reading header (1)");
        return 0;
    }

    if((mh->id[14]<'1') || (mh->id[14]>'4'))
    {   _mmlogd1("load_ult > Unsupported file type: %d",mh->id[14]);
        return 0;
    }

    of->modtype   = _mm_strdup(of->allochandle, ULT_Version[mh->id[14]-'1']);
    of->initspeed = 6;
    of->inittempo = 125;

    // read songtext
    // -------------
    // ULTs are stored in lines of 32 characters a piece, so we load
    // one line at a time and attach a CR to the end (per unimod format specs).

    if(!(of->comment=(CHAR *)_mm_malloc(of->allochandle, (UWORD)mh->reserved*34))) return 0;

    {    
        CHAR *comt = of->comment;

        for(u=0; u<(UWORD)mh->reserved; u++, comt+=33)
        {   
            _mm_read_UBYTES(comt,32,mmfile);
            comt[32] = 0x0d;
        }
        *comt = 0;
    }

    nos = _mm_read_UBYTE(mmfile);

    if(_mm_feof(mmfile))
    {   _mmlogd("load_ult > Unexpected end of file reading header (2)");
        return 0;
    }

    of->songname = DupStr(of->allochandle, mh->songtitle,32);
    of->numsmp   = nos;

    if(!AllocSamples(of,0)) return 0;

    q = of->samples;

    for(t=0; t<nos; t++)
    {   // try to read sample info

        _mm_read_string(s.samplename,32,mmfile);
        _mm_read_string(s.dosname,12,mmfile);
        s.loopstart     =_mm_read_I_ULONG(mmfile);
        s.loopend       =_mm_read_I_ULONG(mmfile);
        s.sizestart     =_mm_read_I_ULONG(mmfile);
        s.sizeend       =_mm_read_I_ULONG(mmfile);
        s.volume        =_mm_read_UBYTE(mmfile);
        s.flags         =_mm_read_UBYTE(mmfile);
        s.finetune      =_mm_read_I_SWORD(mmfile);

        if(_mm_feof(mmfile))
        {   _mmlogd("load_ult > Unexpected end of file reading header (3)");
            return 0;
        }

        q->samplename = DupStr(of->allochandle, s.samplename,32);

        q->speed      = 8363;

        if(mh->id[14]>='4')
        {   _mm_read_I_UWORD(mmfile);       // read 1.6 extra info(??) word
            q->speed = s.finetune;
        }

        q->length    = s.sizeend-s.sizestart;
        q->volume    = s.volume/2;
        q->loopstart = s.loopstart;
        q->loopend   = s.loopend;

        q->format    = SF_SIGNED;

        if(s.flags & ULTS_LOOP)
            q->flags |= SL_LOOP;

        if(s.flags & ULTS_16BITS)
        {   q->format     |= SF_16BITS;
            q->loopstart >>= 1;
            q->loopend   >>= 1;
        }

        q++;
    }

    if(!AllocPositions(of,256)) return 0;
    for(t=0; t<256; t++)
        of->positions[t] = _mm_read_UBYTE(mmfile);
    for(t=0; t<256; t++)
        if(of->positions[t]==255) break;

    of->numpos = t;

    noc = _mm_read_UBYTE(mmfile);
    nop = _mm_read_UBYTE(mmfile);

    of->numchn = noc+1;
    of->numpat = nop+1;
    of->numtrk = of->numchn*of->numpat;

    if(!AllocTracks(of)) return 0;
    if(!AllocPatterns(of)) return 0;

    for(u=0; u<of->numchn; u++)
    {   for(t=0; t<of->numpat; t++)
            of->patterns[(t*of->numchn)+u] = tracks++;
    }

    // read pan position table for v1.5 and higher

    if(mh->id[14]>='3')
        for(u=0; u<of->numchn; u++) of->panning[u] = (_mm_read_UBYTE(mmfile) * 16) + PAN_LEFT;

    of->ut = utrk_init(1, handle->allochandle);
    utrk_memory_reset(of->ut);
    utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);

    if(!(handle->globtrack = (UBYTE **)_mm_calloc(handle->allochandle, of->numtrk, sizeof(UBYTE *)))) return 0;

    for(t=0; t<of->numtrk; t++)
    {   int rep,s,done;

        utrk_reset(of->ut);
        done = 0;

        while(done < 64)
        {   rep = ReadUltEvent(&ev,mmfile);
            if(_mm_feof(mmfile))
            {   _mmlogd("load_ult > Unexpected end of file reading header (4)");
                return 0;
            }

            for(s=0; s<rep; s++)
            {   uint  eff;

                utrk_write_inst(of->ut, ev.sample);
                if(ev.note) utrk_write_note(of->ut, ev.note+24);

                eff = ev.eff>>4;

                //  ULT panning effect fixed by Alexander Kerkhove :

                if(eff==0xc) pt_write_effect(of->ut, eff,ev.dat2>>2);
                else if(eff==0xb) pt_write_effect(of->ut, 8,ev.dat2*0xf);
                else pt_write_effect(of->ut, eff,ev.dat2);

                eff = ev.eff&0xf;

                if(eff==0xc) pt_write_effect(of->ut, eff,ev.dat1>>2);
                else if(eff==0xb) pt_write_effect(of->ut, 8,ev.dat1*0xf);
                else pt_write_effect(of->ut, eff,ev.dat1);

                done++;
                utrk_newline(of->ut);
            }
        }

        if(!(of->tracks[t] = utrk_dup_track(of->ut, 0, of->allochandle))) return 0;
        handle->globtrack[t] = utrk_dup_global(of->ut, handle->allochandle);
    }

    // Pattern setup is read, so now process global tracks.
    pt_global_consolidate(of,handle->globtrack);

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
    CHAR *ULT_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
   CHAR s[32];

   _mm_fseek(mmfile,15,SEEK_SET);
   if(!_mm_read_UBYTES(s,32,mmfile)) return NULL;
   
   return(DupStr(NULL,s,32));
}

/* Somehow it seems that someone thinks that replacing ML_HANDLE with
   the real contents of what it points to is OK. Well, it compiles OK,
   but I'd rather get rid of those warnings. [JEL] */

MLOADER load_ult =
{   
    "ULT",
    "Ultratracker",

    0,                              // default FULL STEREO panning

    NULL,
    ULT_Test,
    ULT_Init,
    (void (*)(ML_HANDLE *))ULT_Cleanup, /* Another pointer type mismatch. */
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))ULT_Load,
    ULT_LoadTitle
};

