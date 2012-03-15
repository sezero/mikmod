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
 Module: LOAD_DSM.C

  DSIK Internal Format (DSM) module loader.  Developed by Pelusa and used by
  Epic Megagames, if I recall correctly (don't take my word on that!).

 Portability:
  All systems - all compilers (hopefully)
*/

#include <string.h>
#include "mikmod.h"


typedef struct DSMINST
{   CHAR  filename[13];
    UWORD flags;
    UBYTE volume;
    ULONG length;
    ULONG loopstart;
    ULONG loopend;
    ULONG reserved1;
    UWORD c2spd;
    UWORD reserved2;
    CHAR  samplename[28];
} DSMINST;


typedef struct DSMSONG
{  CHAR  songname[28];
   UWORD reserved1;
   UWORD flags;
   ULONG reserved2;
   UWORD numord;
   UWORD numsmp;
   UWORD numpat;
   UWORD numtrk;
   UBYTE globalvol;
   UBYTE mastervol;
   UBYTE speed;
   UBYTE bpm;
   UBYTE panpos[16];
   UBYTE orders[128];
} DSMSONG;



static CHAR  *SONGID = "SONG";
static CHAR  *INSTID = "INST";
static CHAR  *PATTID = "PATT";


static UBYTE blockid[4];
static ULONG blockln;
static ULONG blocklp;
static DSMSONG *mh = NULL;

static CHAR  DSM_Version[] = "DSIK DSM-format";


BOOL DSM_Test(void)
{
    UBYTE id[12];

    if(_mm_read_UBYTES((UBYTE *)id,12,modfp)) return 0;
    if(!memcmp(id,"RIFF",4) && !memcmp(&id[8],"DSMF",4)) return 1;

    return 0;
}


BOOL DSM_Init(void)
{
    if(!(mh=(DSMSONG *)_mm_calloc(1,sizeof(DSMSONG)))) return 0;
    return 1;
}


void DSM_Cleanup(void)
{
   if(mh) _mm_free(mh);

   mh     = NULL;
}


BOOL GetBlockHeader(void)
{
   // make sure we're at the right position for reading the
   // next riff block, no matter how many bytes read

   _mm_fseek(modfp, blocklp+blockln, SEEK_SET);
   
   while(1)
   {   _mm_read_UBYTES(blockid,4,modfp);
       blockln = _mm_read_I_ULONG(modfp);
       if(feof(modfp))
       {   _mm_errno = MMERR_LOADING_HEADER;
           return 0;
       }
       
       if(memcmp(blockid,SONGID,4) && memcmp(blockid,INSTID,4) && memcmp(blockid,PATTID,4))
       {   //printf("Skipping unknown block type %4.4s\n",&blockid);
           _mm_fseek(modfp, blockln, SEEK_CUR);
       } else break;
   }

   blocklp = _mm_ftell(modfp);
   return 1;
}


BOOL DSM_ReadPattern(void)
{
    int          row=0,flag, ch;
    unsigned int note,ins,vol,cmd,inf;

    _mm_read_UBYTE(modfp);
    _mm_read_UBYTE(modfp);

    while(row<64)
    {   flag = _mm_read_UBYTE(modfp);
        if(feof(modfp))
        {   _mm_errno = MMERR_LOADING_PATTERN;
            return 0;
        }

        if(flag)
        {   ch = flag & 0xf;
            if(flag & 0x80) note = _mm_read_UBYTE(modfp);
            if(flag & 0x40) ins  = _mm_read_UBYTE(modfp);
            if(flag & 0x20) vol  = _mm_read_UBYTE(modfp);
            if(flag & 0x10)
            {   cmd = _mm_read_UBYTE(modfp);
                inf = _mm_read_UBYTE(modfp);
            }

            utrk_settrack(ch);
            if(ins!=0 && ins!=255) utrk_write_inst(ins);
            if(note!=255) utrk_write_note(note+1);                // <- normal note
            if(vol<65) pt_write_effect(0xc, vol);
    
            if(cmd!=255)
            {   // Panning effect 0x8 (0 = left 0x80 = right).
                // Pattern Jump effect 0xb
                if((cmd==0x8 && inf<=0x80) || (cmd==0xb && inf<=0x7f))
                    pt_write_effect(cmd, inf);
                else
                {   // Convert pattern jump from Dec to Hex
                    if(cmd == 0xd)
                        inf = (((inf&0xf0)>>4)*10) + (inf&0xf);
                    pt_write_effect(cmd,inf);
                }
            }
        } else row++;
        utrk_newline();
    }
    return 1;
}

BOOL DSM_Load(void)
{
       int     t;
       DSMINST s;
       SAMPLE *q;
       int    cursmp = 0, curpat = 0;

       blocklp = 0;
       blockln = 12;

       if(!GetBlockHeader()) return 0;
       if(memcmp(blockid,SONGID,4))
       {   _mm_errno = MMERR_LOADING_HEADER;
           return 0;
       }

       _mm_read_UBYTES(mh->songname,28,modfp);
       mh->reserved1   = _mm_read_I_UWORD(modfp);
       mh->flags       = _mm_read_I_UWORD(modfp);
       mh->reserved2   = _mm_read_I_ULONG(modfp);
       mh->numord      = _mm_read_I_UWORD(modfp);
       mh->numsmp      = _mm_read_I_UWORD(modfp);
       mh->numpat      = _mm_read_I_UWORD(modfp);
       mh->numtrk      = _mm_read_I_UWORD(modfp);
       mh->globalvol   = _mm_read_UBYTE(modfp);
       mh->mastervol   = _mm_read_UBYTE(modfp);
       mh->speed       = _mm_read_UBYTE(modfp);
       mh->bpm         = _mm_read_UBYTE(modfp);
       _mm_read_UBYTES(mh->panpos,16,modfp);
       _mm_read_UBYTES(mh->orders,128,modfp);

       // set module variables
       of.initspeed = mh->speed;
       of.inittempo = mh->bpm;
       of.modtype   = _mm_strdup(DSM_Version);
       of.numchn    = mh->numtrk;
       of.numpat    = mh->numpat;
       of.numtrk    = of.numchn*of.numpat;
       of.songname  = DupStr(mh->songname,28);    // make a cstr of songname

       for(t=0; t<16; t++)
          of.panning[t] = mh->panpos[t]<0x80 ? (mh->panpos[t]<<1) : 255;

       if(!AllocPositions(mh->numord)) return 0;
       of.numpos = 0;
       for(t=0; t<mh->numord; t++)
       {   of.positions[of.numpos] = mh->orders[t];
           if(mh->orders[t]<254) of.numpos++;
       }

       of.numsmp = mh->numsmp;

       if(!AllocSamples(0)) return 0;
       if(!AllocTracks()) return 0;
       if(!AllocPatterns()) return 0;

       utrk_init(of.numchn);

       while(cursmp < of.numsmp || curpat < of.numpat)
       {   if(!GetBlockHeader()) return 0;
           if(!memcmp(blockid,INSTID,4) && cursmp < of.numins)
           {   q = &of.samples[cursmp];
               
               // try to read sample info
               _mm_read_UBYTES(s.filename,13,modfp);
               s.flags     = _mm_read_I_UWORD(modfp);
               s.volume    = _mm_read_UBYTE(modfp);
               s.length    = _mm_read_I_ULONG(modfp);
               s.loopstart = _mm_read_I_ULONG(modfp);
               s.loopend   = _mm_read_I_ULONG(modfp);
               s.reserved1 = _mm_read_I_ULONG(modfp);
               s.c2spd     = _mm_read_I_UWORD(modfp);
               s.reserved2 = _mm_read_I_UWORD(modfp);
               _mm_read_UBYTES(s.samplename,28,modfp);

               q->samplename = DupStr(s.samplename,28);
               q->seekpos    = _mm_ftell(modfp);
               q->speed      = s.c2spd;
               q->length     = s.length;
               q->loopstart  = s.loopstart;
               q->loopend    = s.loopend;
               q->volume     = s.volume;

               if(s.flags & 1) q->flags |= SF_LOOP;
               if(s.flags & 2) q->flags |= SF_SIGNED;
               cursmp++;
           } else if(!memcmp(blockid,PATTID,4) && curpat<of.numpat)
           {   DSM_ReadPattern();
               if(!utrk_dup_pattern(&of)) return 0;
               curpat++;
           }
       }

       return 1;
}


#ifndef __MM_WINAMP__
CHAR *DSM_LoadTitle(void)
{
   CHAR s[28];

   _mm_fseek(modfp,12,SEEK_SET);
   if(!fread(s,28,1,modfp)) return NULL;
   
   return(DupStr(s,28));
}
#endif

MLOADER load_dsm =
{   "DSM",
    "Portable DSM loader v0.1",

    NULL,
    DSM_Init,
    DSM_Test,
    DSM_Load,
    DSM_Cleanup,
#ifndef __MM_WINAMP__
    DSM_LoadTitle
#endif
};

