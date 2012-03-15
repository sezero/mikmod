/*

 MikMod Sound System

  By Jake Stine of Divine Entertainment (1996-1999)
  Updated and fixed by Jan Lönnberg 2001.

 Support:
  If you find problems with this code, send mail to:
    air@divent.simplenet.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------

 Module: LOAD_669.C
 
  Tran's 669 module loader.  It doesn't work very well because Tran's brain
  has a method of twisted logic beyond the extraordinary, and a lacking a-
  bility to document it.

 Portability:
   All systems - all compilers (hopefully)


*/

#include <string.h>
#include "mikmod.h"
#include "uniform.h"

// Raw 669 header struct:

typedef struct S69HEADER
{   UBYTE  marker[2];
    CHAR   message[108];
    UBYTE  nos;
    UBYTE  nop;
    UBYTE  looporder;
    UBYTE  orders[0x80];
    UBYTE  tempos[0x80];
    UBYTE  breaks[0x80];
} S69HEADER;


// Raw 669 sampleinfo struct:

typedef struct S69SAMPLE
{   CHAR   filename[13];
    SLONG  length;
    SLONG  loopbeg;
    SLONG  loopend;
} S69SAMPLE;


//static S69HEADER *mh   = NULL;

static CHAR *S69_Version[] =
{   "669",
    "Extended 669"
};

enum {
  TRANMEM=PTMEM_LAST,
  TRANMEM_LAST
};

BOOL S69_Test(MMSTREAM *modfp)
{
    UBYTE id[2];

    if(!_mm_read_UBYTES(id,2,modfp)) return 0;
    if(!memcmp(id,"if",2) || !memcmp(id,"JN",2))
    {   _mm_fseek(modfp,108,SEEK_CUR);
        if(_mm_read_UBYTE(modfp) > 64) return 0;
        if(_mm_read_UBYTE(modfp) > 128) return 0;
        if(_mm_read_UBYTE(modfp) > 120) return 0;
        return 1;
    }
    return 0;
}


void *S69_Init(void)
{
    return _mm_calloc(NULL, 1,sizeof(S69HEADER));
}


void S69_Cleanup(void *mh)
{
    if(mh) _mm_free(NULL,mh);

    mh   = NULL;
}


static BOOL S69_LoadPatterns(UNIMOD *of, MMSTREAM *modfp,S69HEADER *mh)
{
    int     t,t2,t3;
    unsigned int note,inst,vol,lo;
    UBYTE   a,b,c;
    int effw;
    UNITRK_EFFECT effdat;

    if(!AllocPatterns(of)) return 0;
    if(!AllocTracks(of))   return 0;

    of->ut=utrk_init(of->numchn,of->allochandle);
    utrk_memory_reset(of->ut);

    for(t=0; t<of->numpat; t++)
    {   of->pattrows[t] = mh->breaks[t]+1;

        utrk_reset(of->ut);
	utrk_settrack(of->ut,0);
	pt_write_effect(of->ut,0xf,mh->tempos[t]);
        for(t2=64; t2; t2--)
        {   for(t3=0; t3<8; t3++)
            {   a = _mm_read_UBYTE(modfp);
                b = _mm_read_UBYTE(modfp);
                c = _mm_read_UBYTE(modfp);

		effw=1;

                note = a >> 2;
                inst = ((a & 0x3) << 4) | ((b & 0xf0) >> 4);
                vol  = b & 0xf;

                utrk_settrack(of->ut,t3);

                if(a < 0xfe)
                {   utrk_write_inst(of->ut,inst+1);
                    utrk_write_note(of->ut,note+25);
                    effdat.effect   = 0;
                    effdat.framedly = 0;
                    utrk_write_local(of->ut, &effdat, TRANMEM);
		    pt_write_effect(of->ut,0xc,vol<<2);
		    effw=0;
                }

                if(a < 0xff) pt_write_effect(of->ut,0xc,vol<<2);

                lo = c & 0xf;
                switch(c >> 4)
                {
		case 0: /* Pitch slide up... */
		  effdat.param.s  = -((lo*157)>>2);
                  effdat.effect   = UNI_PITCHSLIDE;
                  effdat.framedly = 0;
                  utrk_write_local(of->ut, &effdat, TRANMEM);
		  break;
		  
		case 1: /* Pitch slide down... */
		  effdat.param.s  = ((lo*157)>>2);
                  effdat.effect   = UNI_PITCHSLIDE;
                  effdat.framedly = 0;
                  utrk_write_local(of->ut, &effdat, TRANMEM);
		  break;
		  
		case 2:
                  effdat.effect   = UNI_PORTAMENTO_TRAN;
                  effdat.framedly = 0;
		  effdat.param.u  = (lo*157);
	          utrk_write_local(of->ut, &effdat, TRANMEM);
		  break;
		  
		case 3:
                  effdat.param.u   = 8066+10*lo;
                  effdat.effect    = UNI_SETSPEED_TRAN; /* Seems to increase by approx. 10 Hz/step. */
                  effdat.framedly  = UFD_RUNONCE;
                  utrk_write_local(of->ut, &effdat, UNIMEM_NONE);
		  break;

		case 4:
                  effdat.framedly = 0;
		  effdat.effect  = UNI_VIBRATO_SPEED;
                  effdat.param.u = 64;
                  utrk_write_local(of->ut, &effdat, UNIMEM_NONE);
		  effdat.effect  = UNI_VIBRATO_DEPTH;
                  effdat.param.u = (lo*157)>>1;
                  utrk_write_local(of->ut, &effdat, TRANMEM);
		  break;
		  
		case 5:
                  effdat.effect   = 0;
                  effdat.framedly = 0;
                  utrk_write_local(of->ut, &effdat, TRANMEM);
		  pt_write_effect(of->ut,0xf,lo);
		  break;
		case 6:
                  effdat.effect   = 0;
                  effdat.framedly = 0;
                  utrk_write_local(of->ut, &effdat, TRANMEM);
		  if (lo>1) break;
              	  effdat.effect  = UNI_PANSLIDE;
            	  effdat.param.s = lo?17:-17;
                  effdat.framedly  = UFD_RUNONCE;
            	  utrk_write_local(of->ut, &effdat, UNIMEM_NONE);
		  break;
		case 7:
                  effdat.effect   = 0;
                  effdat.framedly = 0;
                  utrk_write_local(of->ut, &effdat, TRANMEM);
		  break; /* Some sort of weird retrig goes here. */
		default:
		  if (effw) {
                    effdat.framedly = 0;
		    effdat.effect  = UNI_VIBRATO_SPEED;
                    effdat.param.u = 64;
                    utrk_write_local(of->ut, &effdat, UNIMEM_NONE);
		    utrk_memory_local(of->ut, NULL, TRANMEM, 0);
		  }
                }
            }
            utrk_newline(of->ut);
        }

        if(_mm_feof(modfp))
	  {   //_mm_errno = MMERR_LOADING_PATTERN;
	    _mmlog("load_669 > Failure: Unexpected end of file reading patterns.");
            return 0;
	  }

        if(!utrk_dup_pattern(of->ut,of)) return 0;
    }
    return 1;
}


BOOL S69_Load(S69HEADER *mh, UNIMOD *of, MMSTREAM *modfp)
{
    int         t;
    S69SAMPLE   s;
    UNISAMPLE     *q;

    // try to read module header       

    _mm_read_UBYTES(mh->marker,2,modfp);
    _mm_read_UBYTES((UBYTE *)mh->message,108,modfp);
    mh->nos = _mm_read_UBYTE(modfp);
    mh->nop = _mm_read_UBYTE(modfp);
    mh->looporder = _mm_read_UBYTE(modfp);
    _mm_read_UBYTES(mh->orders,0x80,modfp);
    _mm_read_UBYTES(mh->tempos,0x80,modfp);
    _mm_read_UBYTES(mh->breaks,0x80,modfp);

    // set module variables

    of->initspeed = 6;
    of->inittempo = 78;
    of->songname  = DupStr(NULL,mh->message,36);
    of->comment   = DupStr(NULL,mh->message,108);
    of->modtype   = _mm_strdup(of->allochandle,S69_Version[memcmp(mh->marker,"JN",2)==0]);
    of->numchn    = 8;
    of->numpat    = mh->nop;
    of->numsmp    = mh->nos;
    of->numtrk    = of->numchn*of->numpat;
    of->flags     = UF_LINEAR_FREQ; /* Remember, Tran can't have seen FT2 when
				       he wrote 669. "Linear" means "every pitch
				       step is an equally large frequency change",
				       not the proportional system in FT2. */
    of->memsize=TRANMEM_LAST;
    of->reppos=mh->looporder;

    /* Panning in UNIS669 has 16 steps (0 left - 15 right).
       Panning defaults to 4 and 11.
	
	0 1 2 3 4 5 6 7 8 9 A B C D E F
     -128              0              128

	We make each step 17 MikMod units, making the initial
	values (4-7.5)*32=-60 and (11-7.5)*32=60.

	*/

    for(t=0;t<8;t++)
      of->panning[t] = (t&1) ? (60) : (-60);
 
    if(!AllocPositions(of,0x80)) return 0;
    for(t=0; t<0x80; t++)
    {   if(mh->orders[t]==0xff) break;
        of->positions[t] = mh->orders[t];
    }

    of->numpos = t;

    if(!AllocSamples(of,0)) return 0;
    q = of->samples;

    for(t=0; t<of->numsmp; t++)
    {   // try to read sample info

        _mm_read_UBYTES((UBYTE *)s.filename,13,modfp);
        s.length   = _mm_read_I_SLONG(modfp);
        s.loopbeg  = _mm_read_I_SLONG(modfp);
        s.loopend  = _mm_read_I_SLONG(modfp);

        if((s.length < 0) || (s.loopbeg < -1) || (s.loopend < -1))
        {   _mmlog("load_669 > Failure: Unexpected end of file reading module header");
            return 0;
        }

        q->samplename = DupStr(NULL,s.filename,13);

        q->seekpos   = 0;
        q->speed     = 8066;   /* Experiments show that UNIS669 is tuned to roughly 8097 Hz
				+/- 2 Hz. 
				A pitch bend of 1 up from this is 8098 Hz +/- 2 Hz, a
				A pitch bend of 4 up from this is 8185 Hz +/- 2 Hz, a
				pitch bend of 8 up from this is 8349 Hz +/- 2 Hz. 
				(warning: GUS frequency accuracy approx. 40 Hz)
				
				Pitch bend of 16 is 8694 Hz +/- 2 Hz
				Pitch bend of 32 is 9310 Hz +/- 2 Hz
				Pitch bend of 64 is 10601 Hz +/- 2 Hz.
				Pitch bend of 128 is 13087 Hz  +/- 2 Hz.
				Pitch bend of 256 is 18176 Hz +/- 5 Hz.
				Pitch bend of 512 is 28149 Hz +/- 10 Hz.
				Pitch bend of 1024 is 48268 Hz  +/- 20 Hz.

				WTF? Does "linear pitch slides" mean that each
				slide step is a constant frequency amount?

				Those numbers seem to suggest a frequency step of about
				39.25 Hz... Specifically, 8066 Hz middle C (C-3 in 669
				parlance) and 39.26 Hz frequency step. The deviations
				appear to be caused by quantisation in the GUS routines.

				I have no idea what sort of numbers the original Composer
				669 uses, 'cos it doesn't run on my system.
				
				*/ 
        q->length    = s.length;
        q->loopstart = s.loopbeg;
        q->loopend   = (s.loopend<=s.length) ? s.loopend : (s.loopbeg?s.length:0);
        q->flags     = (s.loopbeg+4<s.loopend) ? SL_LOOP : 0;
        q->volume    = 64;

        q++;
    }

    if(!S69_LoadPatterns(of,modfp,mh)) return 0;

    return 1;
}


// =====================================================================================
    CHAR *S69_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
   CHAR s[20];

   _mm_fseek(mmfile,2,SEEK_SET);
   if(!_mm_read_UBYTES(s,108,mmfile)) return NULL;
   
   return(DupStr(NULL,s,36)); /* Use first line of message as title. */
}


MLOADER load_669 =
{   "669",
    "Composer 669",

    112,

    NULL,
    S69_Test,
    S69_Init,
    S69_Cleanup,
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))S69_Load,
    S69_LoadTitle
};


