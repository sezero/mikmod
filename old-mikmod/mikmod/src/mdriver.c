/*

 MikMod Sound System

  By Jake Stine of Hour 13 Studios (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Module:  MDRIVER.C

  These routines are used to access the available soundcard/soundsystem
  drivers and mixers.

 Portability:
  All systems - all compilers

*/

#include "mikmod.h"
#include <string.h>

static MD_DEVICE *firstdriver = NULL;

#define MD_IDLE_THRESHOLD 50000ul  // 500ms

// =====================================================================================
    ulong MD_Player(MDRIVER *drv, ulong timepass)
// =====================================================================================
{
    int          c     = 0xfffffful;
    BOOL         use_c = 0;
    MD_VOICESET *cruise = drv->voiceset;

    while(cruise)
    {   if((cruise->flags & MDVS_PLAYER) && cruise->player)
        {   // the only reason countdown will ever be 0 is if
            // the player has just been activated!
            if(cruise->countdown)
                cruise->countdown -= timepass;

            if(cruise->countdown <= 0)
                cruise->countdown += cruise->player(cruise, cruise->playerinfo);

            if(c > cruise->countdown)
            {   c = cruise->countdown;
                use_c = 1;
            }
        }
        cruise = cruise->next;
    }

    // if C is zero, then no player stuff is active, so set us to be called
    // a while from now.  This brings the player to almost no overhead.  Note
    // that the threshold time is no longer important anyway, since anytime a
    // player's timings are changed, or a new player activated, the mixer is
    // preempted to take care of it in a no-latency fashion.

    // give back the # of 100,000th seconds until the next call

    return use_c ? c : MD_IDLE_THRESHOLD;
}

// =====================================================================================
    int MikMod_GetNumDrivers(void)
// =====================================================================================
{
    int        t;
    MD_DEVICE *l;

    for(t=0,l=firstdriver; l; l=l->next, t++);

    return t;
}

// =====================================================================================
    MD_DEVICE *MikMod_DriverInfo(int driver)
// =====================================================================================
{
    int        t;
    MD_DEVICE *l;

    for(t=driver-1,l=firstdriver; t && l; l=l->next, t--);

    return l;
}


// =====================================================================================
    void MD_RegisterDriver(MD_DEVICE *drv)
// =====================================================================================
{
    MD_DEVICE *cruise = firstdriver;

    if(cruise)
    {   while(cruise->next)  cruise = cruise->next;
        cruise->next = drv;
    } else
        firstdriver = drv; 
}

// Note: 'type' indicates whether the returned value should be for hardware
//       or software use.

// =====================================================================================
    ulong MD_SampleSpace(MDRIVER *md, int type)
// =====================================================================================
{
    return md ? md->device.FreeSampleSpace(&md->device, type) : 0;
}


// =====================================================================================
    ULONG MD_SampleLength(MDRIVER *md, int type, SAMPLOAD *s)
// =====================================================================================
{
    return md ? md->device.RealSampleLength(&md->device, type, s) : 0;
}


/*UWORD MD_SetDMA(int secs)

// Converts the given number of 1/10th seconds into the number of bytes of
// audio that a sample # 1/10th seconds long would require at the current mdrv.*
// settings.

{
    ULONG result;

    result = (mdrv.mixfreq * ((mdrv.mode & DMODE_STEREO) ? 2 : 1) *
             ((mdrv.mode & DMODE_16BITS) ? 2 : 1) * secs) * 10;

    if(result > 32000) result = 32000;
    return(mdrv.dmabufsize = (result & ~3));  // round it off to an 8 byte boundry
}*/

// =====================================================================================
    int MD_SampleLoad(MDRIVER *md, int type, SAMPLOAD *s)
// =====================================================================================
// Generally, this function is only called from SL_LoadSamples.
{
    int result;

    if(!md) return 0;

    SL_SampleDelta(s, FALSE);
    SL_Init(s, md->allochandle);
    result = md->device.SampleLoad(&md->device, s, type);
    SL_Exit(s);

    return result;
}


// =====================================================================================
    void MD_SampleUnload(MDRIVER *md, int handle)
// =====================================================================================
{
    if(md && handle >= 0) md->device.SampleUnLoad(&md->device, handle);
}


// Mikmod Sample Precaching! (Raw Sample Data)
// --------------------------------------------
// Precache a sample from a file pointer or from a memory pointer!
// Assigns the handle for the laoded sample to the pointer provided.
// NOTE:
//  - This procedure merely registers the sample for loading.  Precaching only
//    actually occurs with a call to Mikmod_PrecacheSamples(), and hence the
//    handle will only be assigned following that call!
//  - Precached samples are always static (for now).
//      

// =====================================================================================
    void Mikmod_SamplePrecacheFP(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, FILE *fp, long seekpos)
// =====================================================================================
{
    MMSTREAM *mmf;

    mmf = _mmstream_createfp(fp, 0);
    SL_RegisterSample(md, handle, infmt, length, decompress, mmf, seekpos);
}


// =====================================================================================
    void Mikmod_SamplePrecacheMem(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, void *data)
// =====================================================================================
{
    MMSTREAM *mmf;

    mmf = _mmstream_createmem(data, 0);
    SL_RegisterSample(md, handle, infmt, length, decompress, mmf, 0);
}


// =====================================================================================
    void Mikmod_Update(MDRIVER *md)
// =====================================================================================
{
    if(md && md->isplaying) md->device.Update(md);
}


// ================================
//  Functions prefixed with MikMod
// ================================


// =====================================================================================
    MDRIVER *Mikmod_Init(uint mixspeed, uint latency, void *optstr, uint chanmode, uint cpumode, uint drvmode)
// =====================================================================================
// mixspeed -
//   Anything is valid here, since the driver truncates it to a vaild range.
//   Use 0 for an optimal mixing rate (44100 or 48000).
// latency -
//   desired latency, in milliseconds.  The driver wil attempt to get as close
//   to this as possible.  Big numbers (2000) good for players, small numbers
//   (40) good for games.
// optstr -
//   string is used by certain drivers to get information from the program
//   reguarding things such as custom buffer options and soundcard IRQ/DMA
//   settings (generally only used in DOS and Linux drivers).
//
// Mode settings:
//  Each mode setting is considered as 'mode hints.'  Drivers will at will
//  alter settings if the driver does not support the requested mode.  If you
//  require knowing the modes selected by Mikmod, use the Mikmod_GetMode()
//  function.

{
    UWORD t;
    
    MD_DEVICE *device  = NULL, *cruise;  // always autodetect.. for now!
    MDRIVER   *md;

    // Clear the error handler.
    _mmerr_set(MMERR_NONE, NULL, NULL);

    {
        MM_ALLOC *allochandle = _mmalloc_create("MDRIVER", NULL);
        md = (MDRIVER *)_mm_malloc(allochandle, sizeof(MDRIVER));
        md->allochandle = allochandle;
    }

    // Checks and balances... ?
    if(!mixspeed) mixspeed = 48000;
    if(cpumode == CPU_AUTODETECT)
    {   cpumode = CPU_NONE;   //cpuInit();
    }

    if(!device)
    {   for(t=1,cruise=firstdriver; cruise; cruise=cruise->next, t++)
        {   if(cruise->IsPresent()) break;
        }

        if(!cruise)
        {   _mmerr_setsub(MMERR_DETECTING_DEVICE, "Audio driver autodetect failed.", "The Mikmod Sound System failed to find a suitable or supported audio device.  The application will proceed normally without sound.");
            cruise = &drv_nos;
        }
    } else
    {   // if n>0 use that driver
        cruise = device;
        if(!cruise->IsPresent())
        {   // The user has forced this driver, so we generate a critical error upon failure.
            _mmerr_setsub(MMERR_DETECTING_DEVICE, "Audio driver failure.", "A specific audio driver was selected, but no compatable hardware was found.");
            cruise = &drv_nos;
        }
    }

    // make a duplicate of our driver header
    memcpy(&md->device, cruise, sizeof(md->device));

    if(md->device.Init(md, latency, optstr))
    {
        // switch to the nosound driver so that the program can continue to run
        // without sound, if the user so desires...

        md->device.Exit(md);
        memcpy(&md->device, &drv_nos, sizeof(md->device));
        md->device.Init(md, latency, optstr);
    }

    md->device.SetMode(md, mixspeed, drvmode, chanmode, cpumode);

    //initialized  = 1;

    md->name = md->device.Name;

    md->device.GetMode(md, &md->mixspeed, &md->mode, &md->channels, &md->cpu);

    _mmlog("Mikmod > Initialized %s driver.",md->name);
    _mmlog("       > %dkhz, %s",md->mixspeed, (md->channels >= 2) ? "Stereo" : "Mono");

    md->hardvoices = md->softvoices = 0;
    md->optstr     = optstr;
    md->pan        = 0;
    md->pansep     = 128;
    md->volume     = 128;
    md->latency    = latency;

    md->isplaying  = 1;
    md->voiceset   = NULL;    // no voicesets initialized.
    md->vms = md->vmh = NULL;

    return md;
}

// =====================================================================================
    void Mikmod_Exit(MDRIVER *md)
// =====================================================================================
{
    md->device.Exit(md);
    _mmalloc_close(md->allochandle);
}


// =====================================================================================
    BOOL Mikmod_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels, uint cpumode)
// =====================================================================================
{
    BOOL ok = FALSE;
    
    if(md)
    {
        ok = md->device.SetMode(md, mixspeed, mode, channels, cpumode);
        md->isplaying = 0;
    }

    return ok;
}

// =====================================================================================
    void Mikmod_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels, uint *cpumode)
// =====================================================================================
{
    uint dummy;

    if(!md) return;
    
    // this should set the info block in mdriver (md) too!
    md->device.GetMode(md, mixspeed ? mixspeed : &dummy, mode ? mode : &dummy,
                       channels ? channels : &dummy, cpumode ? cpumode : &dummy);
}


/*void Mikmod_SetVolume(MDRIVER *md, int volume)
// Device-based volume control.  Not recommended this be used (as it may not work!)
{
    //md->device.SetVolume(_mm_boundscheck(volume, 0, 128));
}

uint Mikmod_GetVolume(MDRIVER *md)
{
    return(md->device.GetVolume());
}
*/

// =====================================================================================
    int Mikmod_GetActiveVoices(MDRIVER *md)
// =====================================================================================
{
    return(md ? md->device.GetActiveVoices(&md->device) : 0);
}


// =====================================================================================
    BOOL Mikmod_Active(MDRIVER *md)
// =====================================================================================
// This one could be a macro....
{
    return md ? md->isplaying : 0;
}


// =====================================================================================
    void Mikmod_WipeBuffers(MDRIVER *md)
// =====================================================================================
{
    if(md) md->device.WipeBuffers(md);
}

