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
 Module: LOAD_S3M.C

  Screamtracker (S3M) module loader.

 Portability:
  All systems - all compilers (hopefully)
*/


#include <string.h>
#include "mikmod.h"
#include "itshare.h"


/**************************************************************************
**************************************************************************/

// Raw S3M header struct:

typedef struct S3MHEADER
{   CHAR  songname[28];
    UBYTE t1a;
    UBYTE type;
    UBYTE unused1[2];
    UWORD ordnum;
    UWORD insnum;
    UWORD patnum;
    UWORD flags;
    UWORD tracker;
    UWORD fileformat;
    CHAR  scrm[4];
    UBYTE mastervol;
    UBYTE initspeed;
    UBYTE inittempo;
    UBYTE mastermult;
    UBYTE ultraclick;
    UBYTE pantable;
    UBYTE unused2[8];
    UWORD special;
    UBYTE channels[32];
} S3MHEADER;


// Raw S3M sampleinfo struct:

typedef struct S3MSAMPLE
{   UBYTE type;
    CHAR  filename[12];
    UBYTE memsegh;
    UWORD memsegl;
    ULONG length;
    ULONG loopbeg;
    ULONG loopend;
    UBYTE volume;
    UBYTE dsk;
    UBYTE pack;
    UBYTE flags;
    ULONG c2spd;
    UBYTE unused[12];
    CHAR  sampname[28];
    CHAR  scrs[4];
} S3MSAMPLE;

/**************************************************************************
**************************************************************************/

typedef struct _ST3HANDLE
{   S3MHEADER   mh;
    SBYTE       remap[64];       // for removing empty channels
    UBYTE       poslookup[256];  // S3M/IT fix - removing blank patterns needs a
                                 // lookup table to fix position-jump commands
    UWORD      *paraptr;
    MM_ALLOC   *allochandle;
} ST3HANDLE;

static CHAR  S3M_Version[] = "ScreamTracker 3.xx";
static CHAR  S3M_Version_IT[] = "ScreamTracker 3.20 (IT x.xx)";

// =====================================================================================
    BOOL S3M_Test(MMSTREAM *mmfile)
// =====================================================================================
{
    UBYTE id[4];
    
    _mm_fseek(mmfile,0x2c,SEEK_SET);
    if(!_mm_read_UBYTES(id,4,mmfile)) return 0;
    if(!memcmp(id,"SCRM",4)) return 1;
    return 0;
}


// =====================================================================================
    void *S3M_Init(void)
// =====================================================================================
{
    ST3HANDLE   *retval;
    MM_ALLOC     *allochandle;

    allochandle = _mmalloc_create("Load_ST3", NULL);
    retval = (ST3HANDLE *)_mm_calloc(allochandle, 1,sizeof(ST3HANDLE));
    retval->allochandle = allochandle;

    return retval;
}


// =====================================================================================
    void S3M_Cleanup(ST3HANDLE *handle)
// =====================================================================================
{
    if(handle) _mmalloc_close(handle->allochandle);
}


// =====================================================================================
    BOOL S3M_GetNumChannels(MMSTREAM *mmfile, SBYTE *remap, UBYTE *channels)
// =====================================================================================
// Because so many s3m files have 16 channels as the set number used, but really
// only use far less (usually 8 to 12 still), I had to make this function,
// which determines the number of channels that are actually USED by a pattern.
//
// For every channel that's used, it sets the appropriate array entry of the
// global varialbe 'isused'
//
// NOTE: You must first seek to the file location of the pattern before calling
//       this procedure.
// Returns 0 on fail.
{
    int row=0,flag,ch;

    while(row < 64)
    {   flag = _mm_read_UBYTE(mmfile);

        if(_mm_feof(mmfile))
		{   _mmlog("load_s3m > Failure: Unexpected end of file reading pattern");
            return 0;
        }

        if(flag)
        {   ch = flag & 31;
            if(channels[ch] < 16) remap[ch] = 0;
            
            if(flag & 32)
            {   _mm_read_UBYTE(mmfile);
                _mm_read_UBYTE(mmfile);
            }

            if(flag & 64)
                _mm_read_UBYTE(mmfile);

            if(flag & 128)
            {   _mm_read_UBYTE(mmfile);
                _mm_read_UBYTE(mmfile);
            }
        } else row++;
    }

    return 1;
}    


// =====================================================================================
    BOOL S3M_ReadPattern(UTRK_WRITER *ut, MMSTREAM *mmfile, UBYTE *poslookup, SBYTE *remap,
			 int effectver)
// =====================================================================================
{
    int row=0,flag,ch;
    UBYTE note,ins,vol,cmd,inf;

    while(row < 64)
    {   flag = _mm_read_UBYTE(mmfile);

        if(flag==EOF)
    	{   _mmlog("load_s3m > Failure: Unexpected end of file loading pattern data.");
            return 0;
        }

        if(flag)
        {   ch  = remap[flag & 31];
            note = ins = vol = cmd = inf = 255;

            if(flag & 32)
            {   note = _mm_read_UBYTE(mmfile);
                ins  = _mm_read_UBYTE(mmfile);
            }

            if(flag & 64)
                vol = _mm_read_UBYTE(mmfile);

            if(flag & 128)
            {   cmd = _mm_read_UBYTE(mmfile);
                inf = _mm_read_UBYTE(mmfile);
            }

            if(ch != -1)
            {   utrk_settrack(ut, ch);
                if(ins  != 255) utrk_write_inst(ut, ins);
                if(note != 255)
                {   if(note==254)
                    {   UNITRK_EFFECT eff = {0, UNI_NOTEKILL, 0};
                        utrk_write_local(ut, &eff, UNIMEM_NONE);
                    } else
                        utrk_write_note(ut, ((note>>4)*12)+(note&0xf) + 1); // <- normal note
                }

                if(vol < 255) pt_write_effect(ut, 0xc,vol);

                S3MIT_ProcessCmd(ut, poslookup, cmd,inf,effectver,PTMEM_PORTAMENTO);
            }
        } else
        {   row++;
            utrk_newline(ut);
        }
    }
    return 1;
}


// =====================================================================================
    BOOL S3M_Load(ST3HANDLE *h, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    uint        t;
    UNISAMPLE  *q;
    UBYTE       pan[32];
    int         effectver=15;

    // try to read module header

    _mm_read_string(h->mh.songname,28,mmfile);
    h->mh.t1a         =_mm_read_UBYTE(mmfile);
    h->mh.type        =_mm_read_UBYTE(mmfile);
    _mm_read_UBYTES(h->mh.unused1,2,mmfile);
    h->mh.ordnum      =_mm_read_I_UWORD(mmfile);
    h->mh.insnum      =_mm_read_I_UWORD(mmfile);
    h->mh.patnum      =_mm_read_I_UWORD(mmfile);
    h->mh.flags       =_mm_read_I_UWORD(mmfile);
    h->mh.tracker     =_mm_read_I_UWORD(mmfile);
    h->mh.fileformat  =_mm_read_I_UWORD(mmfile);
    _mm_read_string(h->mh.scrm,4,mmfile);

    h->mh.mastervol   =_mm_read_UBYTE(mmfile);
    h->mh.initspeed   =_mm_read_UBYTE(mmfile);
    h->mh.inittempo   =_mm_read_UBYTE(mmfile);
    h->mh.mastermult  =_mm_read_UBYTE(mmfile);
    h->mh.ultraclick  =_mm_read_UBYTE(mmfile);
    h->mh.pantable    =_mm_read_UBYTE(mmfile);
    _mm_read_UBYTES(h->mh.unused2,8,mmfile);
    h->mh.special     =_mm_read_I_UWORD(mmfile);
    _mm_read_UBYTES(h->mh.channels,32,mmfile);

    if(_mm_feof(mmfile))
	{   _mmlog("load_s3m > Failure: Unexpected end of file reading module header");
        return 0;
    }

    // set module variables

    of->memsize     = STMEM_LAST;      // Number of memory slots to reserve!
    if((((h->mh.tracker >> 8) &0xf) < 3)||((h->mh.tracker >> 24)==3))
    {   // versions less than 3 = impulse tracker made (v3.2 compliant!)
	/* IT uses tracker number 3 instead of tracker number 1, so
	   we can future-proof this detection mechanism against IT 3.xx
	   by checking the tracker number too.

	   Unfortunately, ModPlug Tracker uses IT-style extended S3M
	   effects such as Mxx, Nxx, Pxx, Wxx and Zxx, even in S3M mode,
	   and identifies itself as ScreamTracker 3.20. This makes it
           impossible to tell the difference between ModPlug Tracker
           S3Ms with "extended" effects and ScreamTracker 3.20 modules
	   without them. 

	   What we can do is check whether IT was used, and if so
	   interpret a few effects like it does when reading/writing
           S3Ms.

	   If we start deactivating IT's extended effects when IT
	   wasn't used to save the file, we end up losing a lot of effects
           in ModPlug-written S3Ms. On the other hand, if we don't, we
           get a few extra effects whenever an undefined effect is
           used in ST3. Personally, I think it's best to process the
	   effects when in doubt, as I believe more ModPlug users have
           used these effects than ScreamTracker users. */

	of->modtype     = _mm_strdup(of->allochandle, S3M_Version_IT);
        of->modtype[23] = ((h->mh.tracker >> 8) &0xf) + 0x30;
        of->modtype[25] = ((h->mh.tracker >> 4)&0xf) + 0x30;
        of->modtype[26] = ((h->mh.tracker)&0xf) + 0x30;
	effectver&=~8;
    } else
    {   
	of->modtype     = _mm_strdup(of->allochandle, S3M_Version);
	of->modtype[14] = ((h->mh.tracker >> 8) &0xf) + 0x30;
        of->modtype[16] = ((h->mh.tracker >> 4)&0xf) + 0x30;
        of->modtype[17] = ((h->mh.tracker)&0xf) + 0x30;
    }
    of->songname    = DupStr(of->allochandle, h->mh.songname,28);
    of->numpat      = h->mh.patnum;
    of->reppos      = 0;
    of->numsmp      = h->mh.insnum;
    of->initspeed   = h->mh.initspeed;
    of->inittempo   = h->mh.inittempo;
    of->initvolume  = (h->mh.mastervol & 127) << 1;

    of->pansep=(h->mh.mastermult&128)?128:0;

    if (h->mh.flags&64) effectver|=16;

    if (h->mh.tracker==0x1300) effectver|=16;

    // read the order data.. if the pattern for an order is invalid (> numpat)
    // then ignore it.

    if(!AllocPositions(of, h->mh.ordnum)) return 0;
    for(t=0; t<h->mh.ordnum; t++)
        of->positions[t] = _mm_read_UBYTE(mmfile);

    of->numpos = 0;
    for(t=0; t<h->mh.ordnum; t++)
    {   if((of->positions[of->numpos] = of->positions[t]) < h->mh.patnum)
        {   h->poslookup[t]   = of->numpos;   // bug fix for FREAKY S3Ms
            if(of->positions[t] < 254) of->numpos++;
        }
    }

    if((h->paraptr = (UWORD *)_mm_malloc(h->allochandle, (of->numsmp+of->numpat)*sizeof(UWORD)))==NULL) return 0;

    // read the instrument+pattern parapointers
    _mm_read_I_UWORDS(h->paraptr,of->numsmp+of->numpat,mmfile);


    if(h->mh.pantable == 252)
    {   // read the panning table (ST 3.2 addition.  See below for further
        // portions of channel panning [past remapper]).

        _mm_read_UBYTES(pan,32,mmfile);
    }


    // now is a good time to check if the header was too short :)

    if(_mm_feof(mmfile))
	{   _mmlog("load_s3m > Failure: Unexpected end of file reading module header");
        return 0;
    }


    // ==============================================
    // Load those darned Samples!  (no insts in ST3)

    if(!AllocSamples(of, 0)) return 0;

    q = of->samples;

    for(t=0; t<of->numsmp; t++)
    {   S3MSAMPLE s;

        // seek to instrument position (or skip sample if pointer is null)

        if(!h->paraptr[t]) continue;
        _mm_fseek(mmfile,((long)h->paraptr[t])<<4,SEEK_SET);

        // and load sample info

        s.type      =_mm_read_UBYTE(mmfile);
        _mm_read_string(s.filename,12,mmfile);
        s.memsegh   =_mm_read_UBYTE(mmfile);
        s.memsegl   =_mm_read_I_UWORD(mmfile);
        s.length    =_mm_read_I_ULONG(mmfile);
        s.loopbeg   =_mm_read_I_ULONG(mmfile);
        s.loopend   =_mm_read_I_ULONG(mmfile);
        s.volume    =_mm_read_UBYTE(mmfile);
        s.dsk       =_mm_read_UBYTE(mmfile);
        s.pack      =_mm_read_UBYTE(mmfile);
        s.flags     =_mm_read_UBYTE(mmfile);
        s.c2spd     =_mm_read_I_ULONG(mmfile);
        _mm_read_UBYTES(s.unused,12,mmfile);
        _mm_read_string(s.sampname,28,mmfile);
        _mm_read_string(s.scrs,4,mmfile);

        if(_mm_feof(mmfile))
	{   _mmlog("load_s3m > Failure: Unexpected end of file reading sample header %d",t);
            return 0;
        }

        q->samplename = DupStr(of->allochandle, s.sampname,28);
        q->seekpos    = (((long)s.memsegh)<<16|s.memsegl)<<4;

        q->speed      = s.c2spd;
        q->length     = s.length;
        q->loopstart  = s.loopbeg;
        q->loopend    = s.loopend;
        q->volume     = s.volume * 2;

        // Enable aggressive declicking for songs that do not loop and that
        // are long enough that they won't be adversely affected.
        
        if(s.flags & 1) q->flags  |= SL_LOOP;
        if(s.flags & 4) q->format |= SF_16BITS;
        if(h->mh.fileformat == 1) q->format |= SF_SIGNED;

        if(!(q->flags & (SL_LOOP | SL_SUSTAIN_LOOP)) && (q->length > 5000))
          q->flags |= SL_DECLICK;

        // DON'T load sample if it doesn't have the SCRS tag
        if((memcmp(s.scrs,"SCRS",4) != 0) || (q->seekpos == 0)) q->length = 0;

        q++;
    }

    // ====================================
    // Determine the number of channels actually used.  (what ever happened
    // to the concept of a single "numchn" variable, eh?!

    of->numchn = 0;
    memset(h->remap,-1,32*sizeof(UBYTE));

    for(t=0; t<of->numpat; t++)
    {   
        // Patterns with no parapointer are 64 rows, empty.
        if(!h->paraptr[of->numsmp+t]) continue;

        // seek to pattern position ( + 2 skip pattern length )
        _mm_fseek(mmfile,(long)((h->paraptr[of->numsmp+t])<<4)+2,SEEK_SET);
        if(!S3M_GetNumChannels(mmfile, h->remap, h->mh.channels)) return 0;
    }
    
    // build the remap array 
    for(t=0; t<32; t++)
    {   if(h->remap[t]==0)
        {   h->remap[t] = of->numchn;
            of->numchn++;
        }
    }

    // ============================================================
    // set panning positions AFTER building remap chart!

    for(t=0; t<32; t++)
    {   if((h->mh.channels[t] < 16) && (h->remap[t] != -1))
        {   if(h->mh.channels[t] < 8)
                of->panning[h->remap[t]] = PAN_LEFT  + load_s3m.defpan;
            else
                of->panning[h->remap[t]] = PAN_RIGHT - load_s3m.defpan;

//                of->panning[h->remap[t]] = -0xd0;     // 0x30 = std s3m val
//            else
//                of->panning[h->remap[t]] = 0xd0;     // 0xc0 = std s3m val
        }
    }

    if(h->mh.pantable == 252)
    {   // set panning positions according to panning table (new for st3.2)

        for(t=0; t<32; t++)
        {   if((pan[t] & 0x20) && (h->mh.channels[t] < 16) && (h->remap[t] != -1))
                of->panning[h->remap[t]] = ((pan[t] & 0xf) <= 0x7) ? (PAN_LEFT + (pan[t] & 0xf)*16) : (((pan[t] & 0xf)*17) - PAN_RIGHT);
        }
    }


    // ==============================
    // Load the pattern info now!
    
    of->numtrk = of->numpat*of->numchn;
    if(!AllocTracks(of)) return 0;
    if(!AllocPatterns(of)) return 0;

    of->ut = utrk_init(of->numchn, h->allochandle);
    utrk_memory_reset(of->ut);
    utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);

    for(t=0; t<of->numpat; t++)
    {   
        utrk_reset(of->ut);

        if(h->paraptr[of->numsmp+t])
        {
            // seek to pattern position ( + 2 skip pattern length )
            _mm_fseek(mmfile,(((long)h->paraptr[of->numsmp+t])<<4)+2,SEEK_SET);
            if(!S3M_ReadPattern(of->ut, mmfile, h->poslookup, h->remap,effectver))
	      return 0;
        }

        if(!utrk_dup_pattern(of->ut,of)) return 0;
    }

    return 1;
}

         
// =====================================================================================
    CHAR *S3M_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
   CHAR s[28];

   _mm_fseek(mmfile,0,SEEK_SET);
   if(!_mm_read_UBYTES(s,28,mmfile)) return NULL;
   
   return(DupStr(NULL, s,28));
}


MLOADER load_s3m =
{
    "S3M",
    "Screamtracker 3",

    0x30,

    NULL,
    S3M_Test,
    S3M_Init,
    (void (*)(ML_HANDLE *))S3M_Cleanup,

    /* Every single loader seems to need one of these! */
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))S3M_Load,
    S3M_LoadTitle
};

