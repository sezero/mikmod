
#include <ios>
#include "unimod.h"
#include "fileio.h"
#include "types.h"
#include "MODLoader.h"

#include "fileio.h"

using namespace MikMod;

namespace
{
    u16 FineTune(int i)
    {
        static u16 finetune[16] =
        {   8363,   8413,   8463,   8529,   8581,   8651,   8723,   8757,
            7895,   7941,   7985,   8046,   8107,   8169,   8232,   8280
        };

        return finetune[i];
    }

    // MOD specific headers and crap

    struct SampleInfo
    {
        char    sName[22];
        u16     nLength;
        u8      finetune;
        u8      nVolume;
        u16     nReppos;
        u16     nReplen;

        SampleInfo()
            : nLength(0),finetune(0),nVolume(0),nReppos(0),nReplen(0)
        {}
    };

    BinaryStream& operator >> (BinaryStream& stream,SampleInfo& samp)
    {
        stream.read(samp.sName,22);
        //*
            stream >>
            samp.nLength >>
            samp.finetune >>
            samp.nVolume >>
            samp.nReppos >>
            samp.nReplen;
        /*/
        stream.read(&samp.nLength,2);
        stream.read(&samp.finetune,1);
        stream.read(&samp.nVolume,1);
        stream.read(&samp.nReppos,2);
        stream.read(&samp.nReplen,2);*/

        return stream;
    }

    struct Header
    {
        char        sName[20];
        SampleInfo  samples[31];
        u8          nLength;
        u8          magic1;             // should be 127
        u8          nPosition[128];     // The order in which patterns should be played.
        string      sID;                // id string.  4 bytes long
        //u8          magic2[4];          // string "M.K.", "FLT4" or "FLT8"
    };

    BinaryStream& operator >> (BinaryStream& stream,Header& h)
    {
        stream.read(h.sName,20);
        for (int i=0; i<31; i++)
            stream >> h.samples[i];
        
        stream >> h.nLength;
        stream >> h.magic1;               // eat up magic bytes
        stream.read((char*)h.nPosition,128);
        
        char sTemp[5];
        sTemp[4]=0;
        stream.read(sTemp,4);
        h.sID=sTemp;

        return stream;
    }

    const int nHeadersize=1084;

    struct ModType
    {
        char    id[5];
        u8      nChannels;
        string  sName;

        ModType(){}

        ModType(char bleh[5],int c,const char* s)
        {
            for (int i=0; i<5; i++) id[i]=bleh[i];
            nChannels=c;
            sName=s;
        }
    };

    struct ModNote
    {
        u8 a,b,c,d;
    };

    static char protracker []  = "Protracker";
    static char startrekker[]  = "Startrekker";
    static char fasttracker[]  = "Fasttracker";
    static char oktalyzer  []  = "Oktalyzer";
    static char taketracker[]  = "TakeTracker";

    ModType modtypes[] =
    {  ModType("M.K.",4,protracker   ),   // protracker 4 channel
       ModType("M!K!",4,protracker   ),   // protracker 4 channel
       ModType("FLT4",4,startrekker  ),   // startracker 4 channel
       ModType("2CHN",2,fasttracker  ),   // fasttracker 2 channel
       ModType("4CHN",4,fasttracker  ),   // fasttracker 4 channel
       ModType("6CHN",6,fasttracker  ),   // fasttracker 6 channel
       ModType("8CHN",8,fasttracker  ),   // fasttracker 8 channel
       ModType("10CH",10,fasttracker ),   // fasttracker 10 channel
       ModType("12CH",12,fasttracker ),   // fasttracker 12 channel
       ModType("14CH",14,fasttracker ),   // fasttracker 14 channel
       ModType("16CH",16,fasttracker ),   // fasttracker 16 channel
       ModType("18CH",18,fasttracker ),   // fasttracker 18 channel
       ModType("20CH",20,fasttracker ),   // fasttracker 20 channel
       ModType("22CH",22,fasttracker ),   // fasttracker 22 channel
       ModType("24CH",24,fasttracker ),   // fasttracker 24 channel
       ModType("26CH",26,fasttracker ),   // fasttracker 26 channel
       ModType("28CH",28,fasttracker ),   // fasttracker 28 channel
       ModType("30CH",30,fasttracker ),   // fasttracker 30 channel
       ModType("CD81",8,oktalyzer    ),   // atari oktalyzer 8 channel
       ModType("OKTA",8,oktalyzer    ),   // atari oktalyzer 8 channel
       ModType("16CN",16,taketracker ),   // taketracker 16 channel
       ModType("32CN",32,taketracker ),   // taketracker 32 channel
    };

    const int nModtypes=23;

};

Note MODLoader::ConvertNote(const ModNote& n)
{
    // Table directly lifted from the original.  Algorithm almost directly lifted from the original.
    static u16 notexlat[60] =
    {   // -> Tuning 0
        1712,1616,1524,1440,1356,1280,1208,1140,1076,1016 ,960 ,906,
        856 ,808 ,762 ,720 ,678 ,640 ,604 ,570 ,538 ,508  ,480 ,453,
        428 ,404 ,381 ,360 ,339 ,320 ,302 ,285 ,269 ,254  ,240 ,226,
        214 ,202 ,190 ,180 ,170 ,160 ,151 ,143 ,135 ,127  ,120 ,113,
        107 ,101 ,95  ,90  ,85  ,80  ,75  ,71  ,67  ,63   ,60  ,56
    };

    Note nprime;
    int period;

    nprime.instrument=(n.a & 0x10) | (n.c >> 4);
    nprime.effect=n.c&0x0F;
    nprime.effectdata=n.d;
    period=((n.a & 0x0F)<<8) + n.b;

    if (period!=0)
    {
        // Get the note closest to the period given.  (actually, this returns the first one bigger than the period specified)
        for (int i=0; i<60; i++)
            if (period>=notexlat[i])
            {
                nprime.note=i;
                break;
            }

        nprime.note++;

        // Really low period?
        if (i==61)
            nprime.note=0;
    }
    else
        nprime.note=0;

    if (nprime.effect==0x0D)    // convert jump thingie from dec to hex
        nprime.effectdata=(((nprime.effectdata&0xf0)>>4)*10)+(nprime.effectdata&0xf);

    return nprime;
}

void MODLoader::LoadPatterns(BinaryStream& stream,Unimod* mod)
{
    // Heh, lots of sizing vectors going on here.

    ModNote n;

    mod->patterns.resize(mod->numpatterns);

    for (int nPattern=0; nPattern<mod->numpatterns; nPattern++)
    {
        Pattern& pattern=mod->patterns[nPattern];
        pattern.channel.resize(mod->numchannels);

        for (int nTrack=0; nTrack<64; nTrack++)
        {
            for (int nChan=0; nChan<mod->numchannels; nChan++)
            {
                pattern.channel[nChan].notes.resize(64);

                stream >> n.a >> n.b >> n.c >> n.d;

                pattern.channel[nChan].notes[nTrack]=ConvertNote(n);
            }
        }
    }
}

void MODLoader::LoadSamples(BinaryStream& stream,Unimod* mod)
{
    // No support for much of anything remotely cool.

    for (int i=0; i<mod->numsamples; i++)
    {
        Sample& s=mod->samples[i];

        delete[] s.pSampledata;         // paranoia.  This should always be null.

        s.pSampledata=new u16[s.length];

        if (!s.format.at(Sample::sf_16bit))
        {
            // decode 8 bit crap into 16 bit!
            u8* pTemp=new u8[s.length];

            stream.read(pTemp,s.length);

            for (int j=0; j<s.length; j++)
                s.pSampledata[j]=pTemp[j]<<8;

            delete[] pTemp;
        }
        else
            // It's already 16 bits.  Woo.
            stream.read(s.pSampledata,s.length);
    }
}

Unimod* MODLoader::Load(std::istream& _stream)
{
    Unimod* mod=new Unimod;

    BinaryStream stream(_stream);
    stream.SetBigEndian(true);

    int i;              // omnipurpose loop iterator

    Header header;

    stream >> header;

    // "Fix for FREAKY mods..  Not sure if it works for all of 'em tho.. ?"
    // Directly copied from the original.  No idea what it means.
    if (header.nLength>=129)    header.nLength=header.magic1;

    int nModtype=0;

    for (i=0; i<nModtypes; i++)
    {
        if (header.sID==modtypes[i].sName)
        {
            nModtype=i;
            break;
        }
    }

    mod->initspeed=6;
    mod->inittempo=125;
    mod->numchannels=modtypes[nModtype].nChannels;
    mod->modtype=modtypes[nModtype].sName;
    mod->songname=header.sName;
    mod->numpositions=header.nLength;

    mod->positions.resize(mod->numpositions);
    for (i=0; i<mod->numpositions; i++)
        mod->positions[i]=header.nPosition[i];

    // No idea.  Something about counting the number of patterns
    mod->numpatterns=0;

    for (i=0; i<128; i++)
    {
        if (header.nPosition[i]>mod->numpatterns)
            mod->numpatterns=header.nPosition[i];
    }

    mod->numpatterns++;
    mod->numtracks=mod->numpatterns*mod->numchannels;

    // sample info stuffs
    mod->numsamples=31;
    mod->samples.resize(mod->numsamples+1);

    for (i=0; i<mod->numsamples; i++)
    {
        Sample& unismp=mod->samples[i];
        SampleInfo& modsmp=header.samples[i];

        unismp.sName=modsmp.sName;
        
        unismp.speed     = FineTune(modsmp.finetune);
        unismp.volume    = modsmp.nVolume*2;
        unismp.loopstart = modsmp.nReppos*2;
        unismp.loopend   = unismp.loopstart + modsmp.nReplen*2;
        unismp.length    = modsmp.nLength*2;
        unismp.format    = Sample::sf_signed;

        if (modsmp.nReplen>1)
            unismp.loopflags.set(Sample::sl_loop);

        // Enable aggressive declicking for songs that do not loop and that
        // are long enough that they won't be adversely affected.
        if (! (unismp.loopflags.at(Sample::sl_loop) || unismp.loopflags.at(Sample::sl_sustain_loop)) )
            unismp.loopflags.set(Sample::sl_declick);

        if (unismp.loopend>unismp.length)
            unismp.loopend=unismp.length;
    }

    LoadPatterns(stream,mod);

    LoadSamples(stream,mod);

    return mod;
}