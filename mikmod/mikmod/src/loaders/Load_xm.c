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
 Module: LOAD_XM.C

  Fasttracker (XM) module loader.  This is my second favorite loader, because
  there are so many good .XMs out there.  Plus, it's a solid disk-format, func-
  tional, and relatively easy to load and support (unlike.. Cough *IT* cough).

 Portability:
  All systems - all compilers (hopefully)
*/

#include "mikmod.h"
#include "uniform.h"

#include <string.h>


/**************************************************************************
**************************************************************************/

enum
{   FT2MEM_VOLSLIDEUP = PTMEM_LAST,
    FT2MEM_VOLSLIDEDN,          // Fine Volume slide (PT Exx effect w/ memory)
    FT2MEM_PITCHSLIDEUP,        // Fine pitch slide (PT Exx effect w/ memory)
    FT2MEM_PITCHSLIDEDN,
    FT2MEM_EFSLIDEUP,           // Extra Fine Pitch Slide memory!
    FT2MEM_EFSLIDEDN,
    FT2MEM_GLOB_VOLSLIDE,
    FT2MEM_TREMOR,
    FT2MEM_RETRIG,
    FT2MEM_VOLSLIDE,
    FT2MEM_LAST
};

typedef struct XMHEADER
{   CHAR  id[17];                   // ID text: 'Extended module: '
    CHAR  songname[21];             // Module name, padded with zeroes and 0x1a at the end
    CHAR  trackername[20];          // Tracker name
    UWORD version;                  // (word) Version number, hi-byte major and low-byte minor
    ULONG headersize;               // Header size
    UWORD songlength;               // (word) Song length (in patten order table)
    UWORD restart;                  // (word) Restart position
    UWORD numchn;                   // (word) Number of channels (2,4,6,8,10,...,32)
    UWORD numpat;                   // (word) Number of patterns (max 256)
    UWORD numins;                   // (word) Number of instruments (max 128)
    UWORD flags;                    // (word) Flags: bit 0: 0 = Amiga frequency table (see below) 1 = Linear frequency table
    UWORD tempo;                    // (word) Default tempo
    UWORD bpm;                      // (word) Default BPM
    UBYTE orders[256];              // (byte) Pattern order table 
} XMHEADER;


typedef struct XMINSTHEADER
{   ULONG size;                     // (dword) Instrument size
    CHAR  name[22];                 // (char) Instrument name
    UBYTE type;                     // (byte) Instrument type (always 0)
    UWORD numsmp;                   // (word) Number of samples in instrument
    ULONG ssize;                    //
} XMINSTHEADER;


typedef struct XMPATCHHEADER
{   UBYTE what[96];         // (byte) Sample number for all notes
    ENVPT volenv[24];       // (byte) Points for volume envelope
    ENVPT panenv[24];       // (byte) Points for panning envelope
    UBYTE volpts;           // (byte) Number of volume points
    UBYTE panpts;           // (byte) Number of panning points
    UBYTE volsus;           // (byte) Volume sustain point
    UBYTE volbeg;           // (byte) Volume loop start point
    UBYTE volend;           // (byte) Volume loop end point
    UBYTE pansus;           // (byte) Panning sustain point
    UBYTE panbeg;           // (byte) Panning loop start point
    UBYTE panend;           // (byte) Panning loop end point
    UBYTE volflg;           // (byte) Volume type: bit 0: On; 1: Sustain; 2: Loop
    UBYTE panflg;           // (byte) Panning type: bit 0: On; 1: Sustain; 2: Loop
    UBYTE vibflg;           // (byte) Vibrato type
    UBYTE vibsweep;         // (byte) Vibrato sweep
    UBYTE vibdepth;         // (byte) Vibrato depth
    UBYTE vibrate;          // (byte) Vibrato rate
    UWORD volfade;          // (word) Volume fadeout
    UWORD reserved[11];     // (word) Reserved
} XMPATCHHEADER;


typedef struct XMWAVHEADER
{   ULONG length;           // (dword) Sample length
    ULONG loopstart;        // (dword) Sample loop start
    ULONG looplength;       // (dword) Sample loop length
    UBYTE volume;           // (byte) Volume 
    SBYTE finetune;         // (byte) Finetune (signed byte -128..+127)
    UBYTE type;             // (byte) Type: Bit 0-1: 0 = No loop, 1 = Forward loop,
                            //                       2 = Ping-pong loop;
                            //                    4: 16-bit sampledata
    UBYTE panning;          // (byte) Panning (0-255)
    SBYTE relnote;          // (byte) Relative note number (signed byte)
    UBYTE reserved;         // (byte) Reserved
    CHAR  samplename[22];   // (char) Sample name

    UBYTE vibtype;          // (byte) Vibrato type
    UBYTE vibsweep;         // (byte) Vibrato sweep
    UBYTE vibdepth;         // (byte) Vibrato depth
    UBYTE vibrate;          // (byte) Vibrato rate
} XMWAVHEADER;


typedef struct XMPATHEADE
{   ULONG size;                     // (dword) Pattern header length 
    UBYTE packing;                  // (byte) Packing type (always 0)
    UWORD numrows;                  // (word) Number of rows in pattern (1..256)
    UWORD packsize;                 // (word) Packed patterndata size
} XMPATHEADER;

typedef struct XMNOTE
{    UBYTE note,ins,vol,eff,dat;
}XMNOTE;

/**************************************************************************
**************************************************************************/


// =====================================================================================
    BOOL XM_Test(MMSTREAM *mmfile)
// =====================================================================================
{
    UBYTE id[17];
    
    if(!_mm_read_UBYTES(id,17,mmfile)) return 0;
    if(!memcmp(id,"Extended Module: ",17)) return 1;
    return 0;
}


// =====================================================================================
    void *XM_Init(void)
// =====================================================================================
{
    return _mm_calloc(NULL, 1,sizeof(XMHEADER));
}


// =====================================================================================
    void XM_Cleanup(void *handle)
// =====================================================================================
{
    _mm_free(NULL, handle);
}


// =====================================================================================
    void XM_ReadNote(UTRK_WRITER *ut, XMNOTE *ninf, MMSTREAM *mmfile)
// =====================================================================================
{
    uint          cmp;
    uint          hi, lo;
    UNITRK_EFFECT effdat;

    effdat.framedly = 0;

    cmp = _mm_read_UBYTE(mmfile);

    if(cmp & 0x80)
    {   if(cmp &  1) ninf->note = _mm_read_UBYTE(mmfile);
        if(cmp &  2) ninf->ins  = _mm_read_UBYTE(mmfile);
        if(cmp &  4) ninf->vol  = _mm_read_UBYTE(mmfile);
        if(cmp &  8) ninf->eff  = _mm_read_UBYTE(mmfile);
        if(cmp & 16) ninf->dat  = _mm_read_UBYTE(mmfile);
    } else
    {   ninf->note = cmp;
        ninf->ins  = _mm_read_UBYTE(mmfile);
        ninf->vol  = _mm_read_UBYTE(mmfile);
        ninf->eff  = _mm_read_UBYTE(mmfile);
        ninf->dat  = _mm_read_UBYTE(mmfile);
    }

    if(ninf->note==97) 
    {   effdat.effect  = UNI_KEYOFF;
        effdat.param.u = 0;
        utrk_write_local(ut, &effdat, UNIMEM_NONE);
    } else
        utrk_write_note(ut, ninf->note);

    utrk_write_inst(ut, ninf->ins);

    // Process volume column effects!

    hi = ninf->vol>>4;
    lo = ninf->vol & 15;

    switch(hi)
    {   case 0x6:                   // volslide down
            effdat.effect   = UNI_VOLSLIDE;
            effdat.param.s  = (0 - lo) * 2;
            effdat.framedly = 1;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 0x7:                   // volslide up
            effdat.effect   = UNI_VOLSLIDE;
            effdat.param.s  = lo * 2;
            effdat.framedly = 1;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        // volume-row fine volume slide is compatible with protracker
        // EBx and EAx effects i.e. a zero nibble means DO NOT SLIDE, as
        // opposed to 'take the last sliding value'.

        case 0x8:                       // finevol down
            effdat.effect   = UNI_VOLSLIDE;
            effdat.param.s  = (0 - lo) * 2;
            effdat.framedly = UFD_RUNONCE;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 0x9:                       // finevol up
            effdat.effect   = UNI_VOLSLIDE;
            effdat.param.s  = lo * 2;
            effdat.framedly = UFD_RUNONCE;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 0xa:                       // set vibrato speed
            // is this supposed to actually vibrato, using memory?
            if(lo)
            {   effdat.effect   = UNI_VIBRATO_SPEED;
                effdat.param.s  = lo*4;
                effdat.framedly = UFD_RUNONCE;
                utrk_write_local(ut, &effdat, PTMEM_VIBRATO_SPEED);
            }
        break;

        case 0xb:                       // vibrato
            if(lo)
            {   effdat.param.u   = lo*16;
                effdat.framedly  = 0;
                effdat.effect    = UNI_VIBRATO_DEPTH;
                utrk_write_local(ut, &effdat, PTMEM_VIBRATO_DEPTH);
            } else utrk_memory_local(ut, NULL, PTMEM_VIBRATO_DEPTH, 0);
        break;

        case 0xc:                       // set panning
            pt_write_exx(ut,0x8,lo);
        break;

        case 0xd:                       // panning slide left
            effdat.effect  = UNI_PANSLIDE;
            effdat.param.s = (0 - lo);
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 0xe:                       // panning slide right
            effdat.effect  = UNI_PANSLIDE;
            effdat.param.s = lo;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 0xf:                       // tone porta
            effdat.effect   = UNI_PORTAMENTO;

            if(lo)
            {   effdat.framedly = 0;
                effdat.param.u  = lo<<6;
                utrk_write_local(ut, &effdat, PTMEM_PORTAMENTO);
            } else utrk_memory_local(ut, &effdat, PTMEM_PORTAMENTO, 0);
        break;

        default:
            if(ninf->vol>=0x10 && ninf->vol<=0x50)
                pt_write_effect(ut,0xc,ninf->vol-0x10);
    }

    hi = ninf->dat>>4;
    lo = ninf->dat & 15;

    switch(ninf->eff)
    {   
        case 0x3:                       // Effect 3: Portamento
            effdat.effect   = UNI_PORTAMENTO;

            if(ninf->dat)
            {   effdat.framedly = 0;
                effdat.param.u  = ninf->dat<<4;
                utrk_write_local(ut, &effdat, PTMEM_PORTAMENTO);
            } else utrk_memory_local(ut, &effdat, PTMEM_PORTAMENTO, 0);
        break;

        case 0x4:                       // Effect 4: Vibrato
            // this is different from PT in that the depth nor speed are updated
            // until the second tick?  Least the speed is right... (will anyone
            // notice depth? :)
            pt_write_effect(ut,0x4, ninf->dat);
        break;

        case 0x5:                       // Portamento + volume slide!
            if(ninf->dat)
            {   effdat.effect    = UNI_VOLSLIDE;
                effdat.framedly  = 1;
                effdat.param.s   = (hi ? hi : (0 - lo)) * 2;
                utrk_write_local(ut, &effdat, FT2MEM_VOLSLIDE);
            } else utrk_memory_local(ut, &effdat, FT2MEM_VOLSLIDE, 0);

            effdat.effect   = UNI_PORTAMENTO_LEGACY;
            utrk_memory_local(ut, &effdat, PTMEM_PORTAMENTO, 0);
        break;

        case 0x6:                    // Vibrato + Volume Slide
            if(ninf->dat)
            {   effdat.effect    = UNI_VOLSLIDE;
                effdat.framedly  = 1;
                effdat.param.s   = (hi ? hi : (0 - lo)) * 2;
                utrk_write_local(ut, &effdat, FT2MEM_VOLSLIDE);
            } else utrk_memory_local(ut, &effdat, FT2MEM_VOLSLIDE, 0);

            utrk_memory_local(ut, NULL, PTMEM_VIBRATO_DEPTH, 0);
            utrk_memory_local(ut, NULL, PTMEM_VIBRATO_SPEED, 0);
        break;

        case 0xa:
            if(ninf->dat)
            {   effdat.effect    = UNI_VOLSLIDE;
                effdat.framedly  = 1;
                effdat.param.s   = (hi ? hi : (0 - lo)) * 2;
                utrk_write_local(ut, &effdat, FT2MEM_VOLSLIDE);
            } else utrk_memory_local(ut, &effdat, FT2MEM_VOLSLIDE, 0);
        break;

        case 0xe:
            switch(hi)
            {   case 0x1:      // XM fine porta up
                    if(lo)
                    {   effdat.effect    = UNI_PITCHSLIDE;
                        effdat.framedly  = UFD_RUNONCE;
                        effdat.param.s   = lo * 4;
                        utrk_write_local(ut, &effdat, FT2MEM_PITCHSLIDEUP);
                    } else
                        utrk_memory_local(ut, &effdat, FT2MEM_PITCHSLIDEUP, 0);
                break;

                case 0x2:      // XM fine porta down
                    if(lo)
                    {   effdat.effect    = UNI_PITCHSLIDE;
                        effdat.framedly  = UFD_RUNONCE;
                        effdat.param.s   = 0 - (lo * 4);
                        utrk_write_local(ut, &effdat, FT2MEM_PITCHSLIDEDN);
                    } else utrk_memory_local(ut, &effdat, FT2MEM_PITCHSLIDEDN, 0);
                break;

                case 0x5:      // Set finetune
                    effdat.param.u   = (lo*16)-128;
                    effdat.effect    = UNI_SETSPEED;
                    effdat.framedly  = UFD_RUNONCE;
                    utrk_write_local(ut, &effdat, UNIMEM_NONE);
                break;

                case 0xa:      // XM fine volume up
                    if(lo)
                    {   effdat.effect    = UNI_VOLSLIDE;
                        effdat.framedly  = UFD_RUNONCE;
                        effdat.param.s   = lo * 2;
                        utrk_write_local(ut, &effdat, FT2MEM_VOLSLIDEUP);
                    } else utrk_memory_local(ut, &effdat, FT2MEM_VOLSLIDEUP, 0);
                break;

                case 0xb:      // XM fine volume down
                    if(lo)
                    {   effdat.effect    = UNI_VOLSLIDE;
                        effdat.framedly  = UFD_RUNONCE;
                        effdat.param.s   = (0 - lo) * 2;
                        utrk_write_local(ut, &effdat, FT2MEM_VOLSLIDEDN);
                    } else utrk_memory_local(ut, &effdat, FT2MEM_VOLSLIDEDN, 0);
                break;

                default:
                    pt_write_effect(ut,0x0e,ninf->dat);
            }
        break;

        case 'G'-55:                    // G - set global volume
            effdat.effect   = UNI_GLOB_VOLUME;
            effdat.param.u  = (ninf->dat > 0x40) ? 0x80 : (ninf->dat << 1);
            effdat.framedly = UFD_RUNONCE;
            utrk_write_global(ut, &effdat, UNIMEM_NONE);
        break;

        case 'H'-55:                    // H - global volume slide
            if(ninf->dat)
            {   effdat.effect   = UNI_GLOB_VOLSLIDE;
                effdat.param.s  = (hi ? hi : (0 - lo)) * 2;
                effdat.framedly = 1;
                utrk_write_global(ut, &effdat, FT2MEM_GLOB_VOLSLIDE);
            } else utrk_memory_global(ut, &effdat, FT2MEM_GLOB_VOLSLIDE);
        break;

        case 'K'-55:                    // K - keyOff and KeyFade
            effdat.effect   = UNI_KEYOFF;
            effdat.framedly = ninf->dat;
            effdat.param.u  = 0;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 'L'-55:                    // L - set envelope position
            effdat.param.hiword.u = ninf->dat;
            effdat.param.byte_b   = TRUE;
	    effdat.param.byte_a   = 0; /* Works better like this! */
            effdat.effect         = UNI_ENVELOPE_CONTROL;
            effdat.framedly       = UFD_RUNONCE;
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
            // do exact same thing to panning envelope too ?
/*            effdat.param.byte_a   = 1;
              utrk_write_local(ut, &effdat, UNIMEM_NONE);
	      No. */
        break;

        case 'P'-55:                    // P - panning slide
            effdat.effect  = UNI_PANSLIDE;
            effdat.param.s = hi ? hi : (0-lo);
            utrk_write_local(ut, &effdat, UNIMEM_NONE);
        break;

        case 'R'-55:                    // R - multi retrig note
            if(ninf->dat)
            {   effdat.param.loword.u = lo;
                effdat.param.hiword.u = hi;
                effdat.effect   = UNI_RETRIG;
                effdat.framedly = 0;
                utrk_write_local(ut, &effdat, FT2MEM_RETRIG);
            } else utrk_memory_local(ut, &effdat, FT2MEM_RETRIG, 0);
        break;

        case 'T'-55:                    // T - Tremor !! (== S3M effect I)
            effdat.param.loword.u = lo + 1;
            effdat.param.hiword.u = hi + 1;
            effdat.effect   = UNI_TREMOR;
            effdat.framedly = 1;
            utrk_write_local(ut, &effdat, FT2MEM_TREMOR);
        break;

        case 'X'-55:
            if((ninf->dat>>4) == 1)           // X1 - Extra Fine Porta up
            {   if(ninf->dat & 0xf)
                {   effdat.effect    = UNI_PITCHSLIDE;
                    effdat.framedly  = UFD_RUNONCE;
                    effdat.param.s   = (ninf->dat&0xf) * 2;
                    utrk_write_local(ut, &effdat, FT2MEM_EFSLIDEUP);
                } else utrk_memory_local(ut, &effdat, FT2MEM_EFSLIDEUP, 0);
            } else if((ninf->dat>>4) == 2)    // X2 - Extra Fine Porta down
            {   if(ninf->dat & 0xf)
                {   effdat.effect    = UNI_PITCHSLIDE;
                    effdat.framedly  = UFD_RUNONCE;
                    effdat.param.s   = 0 - (ninf->dat&0xf) * 2;
                    utrk_write_local(ut, &effdat, FT2MEM_EFSLIDEDN);
                } else utrk_memory_local(ut, &effdat, FT2MEM_EFSLIDEDN, 0);
            }
        break;

        default:
            if(ninf->eff <= 0xf)
            {   // Convert pattern jump from Dec to Hex
                if(ninf->eff == 0xd)
                    ninf->dat = (((ninf->dat&0xf0)>>4)*10)+(ninf->dat&0xf);
                pt_write_effect(ut, ninf->eff,ninf->dat);
            }
        break;
    }

}


// =====================================================================================
    BOOL XM_Load(XMHEADER *mh, UNIMOD *of, MMSTREAM *mmfile)
// =====================================================================================
{
    INSTRUMENT  *d;
    UNISAMPLE   *q;
    EXTSAMPLE   *eq;
    XMWAVHEADER *wh, *s;
    uint         t;
    long         next;
    ULONG        nextwav[2048];
    BOOL         dummypat = 0;
 
    // try to read module header

    _mm_read_string(mh->id,17,mmfile);
    _mm_read_string(mh->songname,21,mmfile);
    _mm_read_string(mh->trackername,20,mmfile);
    mh->version     =_mm_read_I_UWORD(mmfile);
    mh->headersize  =_mm_read_I_ULONG(mmfile);
    mh->songlength  =_mm_read_I_UWORD(mmfile);
    mh->restart     =_mm_read_I_UWORD(mmfile);
    mh->numchn      =_mm_read_I_UWORD(mmfile);
    mh->numpat      =_mm_read_I_UWORD(mmfile);
    mh->numins      =_mm_read_I_UWORD(mmfile);
    mh->flags       =_mm_read_I_UWORD(mmfile);
    mh->tempo       =_mm_read_I_UWORD(mmfile);
    mh->bpm         =_mm_read_I_UWORD(mmfile);
    _mm_read_UBYTES(mh->orders,256,mmfile);

    if(_mm_feof(mmfile))
    {   _mmlogd("load_xm > Failure: Unexpected end of file reading module header (1)");
        return 0;
    }

    // set module variables
    of->memsize   = FT2MEM_LAST;      // Number of memory slots to reserve!
    of->initspeed = mh->tempo;
    of->inittempo = mh->bpm;
    of->modtype   = DupStr(of->allochandle, mh->trackername,20);
    of->numchn    = mh->numchn;
    of->numpat    = mh->numpat;
    of->numtrk    = (UWORD)of->numpat*of->numchn;   // get number of channels
    of->songname  = DupStr(of->allochandle, mh->songname,20);      // make a cstr of songname
    of->numpos    = mh->songlength;               // copy the songlength
    of->reppos    = mh->restart;
    of->numins    = mh->numins;
    of->flags |= UF_XMPERIODS | UF_INST;
    if(mh->flags&1) of->flags |= UF_LINEAR;

    if(!AllocPositions(of, of->numpos+3)) return 0;
    for(t=0; t<of->numpos; t++)
        of->positions[t] = mh->orders[t];

    // check for XM pattern discrepency:
    // If there are blank patterns at the end of the pattern data, XM doesn't count them,
    // but it WILL reference them from the order list.  So we have to check the orders
    // for any pattern references that are too high, and reference them to a legal blank
    // pattern.

    for(t=0; t<of->numpos; t++)
    {   if(of->positions[t] >= of->numpat)
        {  of->positions[t] = of->numpat;
           dummypat = 1;
        }
    }      

    if(dummypat) { of->numpat++; of->numtrk+=of->numchn; }

    if(!AllocTracks(of)) return 0;
    if(!AllocPatterns(of)) return 0;

    of->ut = utrk_init(of->numchn, of->allochandle);
    utrk_memory_reset(of->ut);
    utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);

    for(t=0; t<mh->numpat; t++)
    {   XMPATHEADER ph;
        long        seekme;

        ph.size     =_mm_read_I_ULONG(mmfile);
        ph.packing  =_mm_read_UBYTE(mmfile);
        ph.numrows  =_mm_read_I_UWORD(mmfile);
        ph.packsize =_mm_read_I_UWORD(mmfile);

        if(ph.packing || (ph.numrows > 16384)) 
        {   _mmlogd2("load_xm > Failure: Invalid packing (%d) and/or numrows (%d)", ph.packing, ph.numrows);
            return 0;
        }

        of->pattrows[t] = ph.numrows;
        seekme = _mm_ftell(mmfile);

        //  Gr8.. when packsize is 0, don't try to load a pattern.. it's empty.
        //  This bug was discovered thanks to Khyron's module..

        utrk_reset(of->ut);
        if(ph.packsize > 0)
        {   uint  u,v;
            for(u=0; u<ph.numrows; u++)
            {   for(v=0; v<of->numchn; v++)
                {   XMNOTE ndat = {0};
                    utrk_settrack(of->ut,v);
                    XM_ReadNote(of->ut, &ndat, mmfile);
                }
                utrk_newline(of->ut);
            }
        }

        if(_mm_feof(mmfile))
	    {   _mmlogd1("load_xm > Failure: Unexpected end of file reading pattern %d",t);
            return 0;
        }

        utrk_dup_pattern(of->ut,of);
        _mm_fseek(mmfile, seekme, SEEK_SET);
        _mm_fseek(mmfile, ph.packsize, SEEK_CUR);
    }

    if(dummypat)
    {   of->pattrows[t] = 64;
        utrk_reset(of->ut);
        utrk_dup_pattern(of->ut,of);
    }

    if(!AllocInstruments(of)) return 0;
    if((wh = (XMWAVHEADER *)_mm_calloc(NULL, 2048,sizeof(XMWAVHEADER))) == NULL) return 0;
    d = of->instruments;
    s = wh;

    for(t=0; t<of->numins; t++)
    {   XMINSTHEADER ih;
        int          headend;

        memset(d->samplenumber,255,120);

        // read instrument header

        headend     = _mm_ftell(mmfile);
        ih.size     = _mm_read_I_ULONG(mmfile);
        headend    += ih.size;
        if(ih.size > 4) 
        {   _mm_read_string(ih.name, 22, mmfile);
            d->insname  = DupStr(of->allochandle, ih.name,22);
        }
        if(ih.size > 26) ih.type     = _mm_read_UBYTE(mmfile);
        if(ih.size > 28) ih.numsmp   = _mm_read_I_UWORD(mmfile);

        if((ih.type == 255) && (ih.numsmp > 32))
        {   _mmlogd3("load_xm > Found Invalid sample header at %d (type=%d numsmp=%d)",t, ih.type, ih.numsmp);
            of->numins = t+1;
            break;
        }

        if(ih.size > 29)
        {
            ih.ssize    = _mm_read_I_ULONG(mmfile);
            if(ih.numsmp > 0)
            {   uint          u,p;
                XMPATCHHEADER pth;
                BOOL          inuse;        // used when checking vol/pan envelopes

                _mm_read_UBYTES (pth.what, 96, mmfile);

                for(p=0; p<12; p++)
                {   pth.volenv[p].pos = _mm_read_I_UWORD(mmfile);
                    pth.volenv[p].val = _mm_read_I_SWORD(mmfile);
                }

                for(p=0; p<12; p++)
                {   pth.panenv[p].pos = _mm_read_I_UWORD(mmfile);
                    pth.panenv[p].val = _mm_read_I_SWORD(mmfile);
                }

                pth.volpts      =  _mm_read_UBYTE(mmfile);
                pth.panpts      =  _mm_read_UBYTE(mmfile);
                pth.volsus      =  _mm_read_UBYTE(mmfile);
                pth.volbeg      =  _mm_read_UBYTE(mmfile);
                pth.volend      =  _mm_read_UBYTE(mmfile);
                pth.pansus      =  _mm_read_UBYTE(mmfile);
                pth.panbeg      =  _mm_read_UBYTE(mmfile);
                pth.panend      =  _mm_read_UBYTE(mmfile);
                pth.volflg      =  _mm_read_UBYTE(mmfile);
                pth.panflg      =  _mm_read_UBYTE(mmfile);
                pth.vibflg      =  _mm_read_UBYTE(mmfile);
                pth.vibsweep    =  _mm_read_UBYTE(mmfile);
                pth.vibdepth    =  _mm_read_UBYTE(mmfile);
                pth.vibrate     =  _mm_read_UBYTE(mmfile);
                pth.volfade     =  _mm_read_I_UWORD(mmfile);
    
                // read the remainder of the header
                for(u=headend-_mm_ftell(mmfile); u; u--)  _mm_read_UBYTE(mmfile);    

                if(_mm_feof(mmfile))
        	    {   _mmlogd1("load_xm > Failure: Unexpected end of file reading instrument header %d",t);
                    return 0;
                }
    
                for(u=0; u<96; u++)         
                   d->samplenumber[u] = pth.what[u] + of->numsmp;
    
                d->volfade = pth.volfade;
    
                // Notes on FT2 Envelopes
                // ----------------------
                //  - We set FT2 envelope carry to be 'ON' by default.  Special-case logic (ouch)
                //    in mplayer.c will disable the carrys for the appropriate situations.
                //
                //  - FT2, interestingly, ignores envelope loops which are a single point.  Makes
                //    you wonder.  Doesn't it?

                d->volflg    = EF_CARRY;
                d->panflg    = EF_CARRY;

                if(pth.volflg & 1)  d->volflg |= EF_ON;
                if(pth.volflg & 2)  d->volflg |= EF_SUSTAIN;
                if((pth.volflg & 4) && (pth.volbeg != pth.volend))  d->volflg |= EF_LOOP;

                d->volsusbeg = d->volsusend = pth.volsus;
                d->volbeg    = pth.volbeg;
                d->volend    = pth.volend;
                d->volpts    = (pth.volpts > 12) ? 12 : pth.volpts;

                // scale volume envelope:

                inuse = 0;
                for(p=0; p<12; p++)
                {   if(pth.volenv[p].val != 64) inuse = 1;
                    d->volenv[p].val = pth.volenv[p].val << 2;
                    d->volenv[p].pos = pth.volenv[p].pos;
                }

                if(!inuse) d->volpts = 0;

                // Bugfix?  Old code just did a stright assignment.  something tells
                // me that wasn't how it should be.

                if(pth.panflg & 1)  d->panflg |= EF_ON;
                if(pth.panflg & 2)  d->panflg |= EF_SUSTAIN;
                if((pth.panflg & 4) && (pth.panbeg != pth.panend))  d->panflg |= EF_LOOP;

                d->pansusbeg = d->pansusend = pth.pansus;
                d->panbeg    = pth.panbeg;
                d->panend    = pth.panend;
                d->panpts    = (pth.panpts > 12) ? 12 : pth.panpts;

                // scale panning envelope:

                inuse = 0;
                for(p=0; p<12; p++)
                {
                    d->panenv[p].val = (pth.panenv[p].val-32) * 4;
		    if(d->panenv[p].val != 0) inuse = 1; /* Should work better here. */
                    d->panenv[p].pos = pth.panenv[p].pos;
                }

                if(!inuse) d->panpts = 0;

                next = 0;
    
                //  Samples are stored outside the instrument struct now, so we have
                //  to load them all into a temp area, count the of->numsmp along the
                //  way and then do an AllocSamples() and move everything over 

                for(u=0; u<ih.numsmp; u++,s++)
                {
                    s->length       =_mm_read_I_ULONG(mmfile);
                    s->loopstart    =_mm_read_I_ULONG(mmfile);
                    s->looplength   =_mm_read_I_ULONG(mmfile);
                    s->volume       =_mm_read_UBYTE(mmfile);
                    s->finetune     =_mm_read_SBYTE(mmfile);
                    s->type         =_mm_read_UBYTE(mmfile);
                    s->panning      =_mm_read_UBYTE(mmfile);
                    s->relnote      =_mm_read_SBYTE(mmfile);
                    s->vibtype      = pth.vibflg;
                    s->vibsweep     = pth.vibsweep;
                    s->vibdepth     = pth.vibdepth*4;
                    s->vibrate      = pth.vibrate;
                    s->reserved     =_mm_read_UBYTE (mmfile);

                    _mm_read_string(s->samplename, 22, mmfile);

                    // ADPCM Support Explaination
                    // --------------------------
                    // Modplug uses the sample's reserved byte and sets it to 0xAD if
                    // the sample is compressed.
                    // ADPCM rules : 16 + (samplelen / 2) ROUNDED UP 

                    nextwav[of->numsmp+u] = next;
                    next += (s->reserved==0xAD) ? (((s->length+1)/2)+16) : s->length;

                    if(_mm_feof(mmfile))
                	{   _mmlogd1("load_xm > Failure: Unexpected end of file loading sample header %d.",u);
                        return 0;
                    }
                }

                for(u=0; u<ih.numsmp; u++) nextwav[of->numsmp++] += _mm_ftell(mmfile);
                _mm_fseek(mmfile,next,SEEK_CUR);
            } else
            {   uint  u;
                for(u=headend-_mm_ftell(mmfile); u; u--) _mm_read_UBYTE(mmfile);
            }
        }

        d++;
    }

    if(!AllocSamples(of, 1)) return 0;
    q  = of->samples;
    eq = of->extsamples;
    s  = wh;

    for(t=0; t<of->numsmp; t++,q++,eq++,s++)
    {   q->samplename   = DupStr(of->allochandle, s->samplename,22);
        q->seekpos      = nextwav[t];

        q->length       = s->length;
        q->loopstart    = s->loopstart;
        q->loopend      = s->loopstart+s->looplength;
        q->volume       = s->volume*2;
        q->speed        = s->finetune+128;
        q->panning      = s->panning+PAN_LEFT;
        eq->vibtype     = s->vibtype;
        eq->vibsweep    = s->vibsweep;
        eq->vibdepth    = s->vibdepth;
        eq->vibrate     = s->vibrate;

        if(s->type & 0x10)
        {   q->length    >>= 1;
            q->loopstart >>= 1;
            q->loopend   >>= 1;
            q->format     |= SF_16BITS;
        }

        q->flags |= PSF_OWNPAN;
        if((s->type & 0x3) && s->looplength) q->flags |= SL_LOOP;
        if(s->type & 0x2) q->flags |= SL_BIDI;

        q->format |= SF_DELTA | SF_SIGNED; 
        if(s->reserved == 0xad) q->compress = DECOMPRESS_ADPCM;
    }

    d = of->instruments;
    s = wh;

    for(t=0; t<of->numins; t++, d++)
    {   uint  u;
        for(u=0; u<96; u++)
            d->samplenote[u] = (d->samplenumber[u]==of->numsmp) ? 255 : (u+s[d->samplenumber[u]].relnote);
    }

    _mm_free(NULL, wh);

    return 1;
}


// =====================================================================================
    CHAR *XM_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
    CHAR s[21];

    _mm_fseek(mmfile,17,SEEK_SET);
    if(!_mm_read_UBYTES(s,21,mmfile)) return NULL;
  
    return(DupStr(NULL, s,21));
}


MLOADER load_xm =
{   
    "XM",
    "Fasttracker 2",

    0,                              // default FULL STEREO panning

    NULL,
    XM_Test,
    XM_Init,
    XM_Cleanup,

    /* The first argument seems to be of the wrong type. As everything
       seems to work, I assume this is merely lazy type conversion. */
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))XM_Load,
    XM_LoadTitle
};
