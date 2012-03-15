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
 Module: LOAD_FAR.C

  Farandole (FAR) module loader.  As I recall this one works well (cept I
  haven't had a .FAR module to test it with for years).

 Portability:
  All systems - all compilers (hopefully)
*/

#include <string.h>
#include "mikmod.h"


typedef struct FARSAMPLE
{   CHAR  samplename[32];
    ULONG length;
    UBYTE finetune;
    UBYTE volume;
    ULONG reppos;
    ULONG repend;
    UBYTE type;
    UBYTE loop;
} FARSAMPLE;



typedef struct FARHEADER1
{   UBYTE id[4];                        // file magic
    CHAR  songname[40];                 // songname
    CHAR  blah[3];                      // 13,10,26
    UWORD headerlen;                    // remaining length of header in bytes
    UBYTE version;
    UBYTE onoff[16];
    UBYTE edit1[9];
    UBYTE speed;
    UBYTE panning[16];
    UBYTE edit2[4];
    UWORD stlen;
} FARHEADER1;


typedef struct FARHEADER2
{   UBYTE orders[256];
    UBYTE numpat;
    UBYTE snglen;
    UBYTE loopto;
    UWORD patsiz[256];
} FARHEADER2;


static CHAR FAR_Version[] = "Farandole";
static FARHEADER1 *mh1 = NULL;
static FARHEADER2 *mh2 = NULL;


BOOL FAR_Test(void)
{
    UBYTE id[4];

    if(!_mm_read_UBYTES(id,4,modfp)) return 0;
    return(!memcmp(id,"FAR=",4));
}


BOOL FAR_Init(void)
{
    if(!(mh1 = (FARHEADER1 *)_mm_malloc(sizeof(FARHEADER1)))) return 0;
    if(!(mh2 = (FARHEADER2 *)_mm_malloc(sizeof(FARHEADER2)))) return 0;

    return 1;
}


void FAR_Cleanup(void)
{
    if(mh1) _mm_free(mh1);
    if(mh2) _mm_free(mh2);

    mh1 = NULL;
    mh2 = NULL;
}


BOOL FAR_Load(void)
{
    int t,u;
    SAMPLE     *q;
    FARSAMPLE  s;
    UBYTE      smap[8];
    
    // try to read module header (first part)
    _mm_read_UBYTES(mh1->id,4,modfp);
    _mm_read_SBYTES(mh1->songname,40,modfp);
    _mm_read_SBYTES(mh1->blah,3,modfp);
    mh1->headerlen = _mm_read_I_UWORD (modfp);
    mh1->version   = _mm_read_UBYTE (modfp);
    _mm_read_UBYTES(mh1->onoff,16,modfp);
    _mm_read_UBYTES(mh1->edit1,9,modfp);
    mh1->speed     = _mm_read_UBYTE(modfp);
    _mm_read_UBYTES(mh1->panning,16,modfp);
    _mm_read_UBYTES(mh1->edit2,4,modfp);
    mh1->stlen     = _mm_read_I_UWORD (modfp);
    
    
    // init modfile data
    
    of.modtype   = _mm_strdup(FAR_Version);
    of.songname  = DupStr(mh1->songname,40);
    of.numchn    = 16;
    of.initspeed = mh1->speed;
    of.inittempo = 99;
    
    for(t=0; t<16; t++) of.panning[t] = mh1->panning[t]<<4;
    
    // read songtext into comment field
    //if(!ReadComment(mh1->stlen)) return 0;
    _mm_fseek(modfp,mh1->stlen,SEEK_CUR);
    
    // try to read module header (second part)
    _mm_read_UBYTES(mh2->orders,256,modfp);
    mh2->numpat        = _mm_read_UBYTE(modfp);
    mh2->snglen        = _mm_read_UBYTE(modfp);
    mh2->loopto        = _mm_read_UBYTE(modfp);
    _mm_read_I_UWORDS(mh2->patsiz,256,modfp);
    
    //      of.numpat=mh2->numpat;
    of.numpos = mh2->snglen;
    if(!AllocPositions(of.numpos)) return 0;
    for(t=0; t<of.numpos; t++)
    {   if(mh2->orders[t]==0xff) break;
       of.positions[t] = mh2->orders[t];
    }
    
    // count number of patterns stored in file
    of.numpat = 0;
    for(t=0; t<256; t++)
      if(mh2->patsiz[t]) if((t+1)>of.numpat) of.numpat=t+1;
    
    of.numtrk = of.numpat*of.numchn;
    
    // seek across eventual new data
    _mm_fseek(modfp,mh1->headerlen-(869+mh1->stlen),SEEK_CUR);
    
    // alloc track and pattern structures
    if(!AllocTracks()) return 0;
    if(!AllocPatterns()) return 0;
    
    utrk_init(16);
    
    for(t=0; t<of.numpat; t++)
    {  unsigned int rows=0, tempo;
       int   ch;
    
       if(mh2->patsiz[t])
       {   rows  = _mm_read_UBYTE(modfp);
           tempo = _mm_read_UBYTE(modfp);
    
           utrk_reset();
           for(ch=0,u=mh2->patsiz[t]-2; u; u--, ch++)
           {   unsigned int  note, ins, vol, eff;
               ch &= 15;
               note = _mm_read_UBYTE(modfp);
               ins  = _mm_read_UBYTE(modfp);
               vol  = _mm_read_UBYTE(modfp);
               eff  = _mm_read_UBYTE(modfp);
    
               utrk_settrack(ch);
               if(note)
               {   utrk_write_inst(ins+1);
                   utrk_write_note(note+24+12);
               }
    
               if(vol & 0xf) pt_write_effect(0xc,(vol&0xf)<<2);
               switch(eff>>4)
               {   case 0xf:
                      pt_write_effect(0xf,eff&0xf);
                   break;
    
                   // others not yet implemented
               }
           }
           
           if(feof(modfp))
           {   _mm_errno = MMERR_LOADING_PATTERN;
               return 0;
           }
    
           of.pattrows[t] = rows+2;
           if(!utrk_dup_pattern(&of)) return 0;
       }
    }
    
    // read sample map
    if(!_mm_read_UBYTES(smap,8,modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
       return 0;
    }
    
    // count number of samples used
    
    of.numsmp = 0;
    for(t=0; t<64; t++)
      if(smap[t>>3] & (1 << (t&7)))  of.numsmp++;
    
    // alloc sample structs
    
    if(!AllocSamples(0)) return 0;
    q = of.samples;
    
    for(t=0; t<64; t++)
    {  if(smap[t>>3] & (1 << (t&7)))
       {  _mm_read_SBYTES(s.samplename,32,modfp);
          s.length   = _mm_read_I_ULONG(modfp);
          s.finetune = _mm_read_UBYTE(modfp);
          s.volume   = _mm_read_UBYTE(modfp);
          s.reppos   = _mm_read_I_ULONG(modfp);
          s.repend   = _mm_read_I_ULONG(modfp);
          s.type     = _mm_read_UBYTE(modfp);
          s.loop     = _mm_read_UBYTE(modfp);
    
          q->samplename = DupStr(s.samplename,32);
          q->length     = s.length;
          q->loopstart  = s.reppos;
          q->loopend    = s.repend;
          q->volume     = 64;
          q->speed      = 8363;
    
          q->flags = SF_SIGNED;
          if(s.type & 1) q->flags |= SF_16BITS;
          if(s.loop)     q->flags |= SF_LOOP;
    
          q->seekpos = _mm_ftell(modfp);
          _mm_fseek(modfp,q->length,SEEK_CUR);
       }
       q++;
    }
    return 1;
}


#ifndef __MM_WINAMP__
CHAR *FAR_LoadTitle(void)
{
   CHAR s[40];

   _mm_fseek(modfp,4,SEEK_SET);
   if(!fread(s,40,1,modfp)) return NULL;
   
   return(DupStr(s,40));
}
#endif

MLOADER load_far =
{   "FAR",
    "Portable FAR loader v0.1",

    NULL,
    FAR_Init,
    FAR_Test,
    FAR_Load,
    FAR_Cleanup,
#ifndef __MM_WINAMP__
    FAR_LoadTitle
#endif
};

