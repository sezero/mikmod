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
 Module:  VOICESET.C

  The Voiceset API - This set of functions allows the user awesome power and control
  over the allocation and attributes of object-oriented groups of voices.

  All voiceset and encapsulated voice control functions.  These have been
  separated from MDRIVER.C to keep the two modules smaller and somewhat
  more structured (and most importantly, less cluttered).

 Portability:
  All systems - all compilers

*/

#include "mikmod.h"
#include <string.h>


// ===========================================================================
// Local (Private) Procedures
//
// allocvoices, freevoices:
//   Voice allocation and deallocation.  Called by the voiceset initialization
//   and free functions.
// ===========================================================================

// =====================================================================================
    static int allocvoices(MDRIVER *md, int mixmode, int num)
// =====================================================================================
// Allocates additional voices into the specified mixing mode.  Returns the actual
// number allocated (is often greater than the value supplied).  Especially in
// hardware mixing mode this function may fail and return fewer than desired, or
// none at all.

{
    int   voices, i;
    int   voicelimit;

    if(mixmode == MM_STATIC)
    {   voices     = md->hardvoices;
        voicelimit = md->device.HardVoiceLimit;
    } else
    {   voices     = md->softvoices;
        voicelimit = md->device.SoftVoiceLimit;
    }

    if((voices + num) > voicelimit)
    {   // adjust voices to fit within hardware limitations...
        num = voicelimit - voices;
    }

    if(num)
    {   voices += num;
        if(mixmode == MM_STATIC)
        {   md->vmh = (MD_VOICESET **)_mm_realloc(md->allochandle, md->vmh, sizeof(MD_VOICESET *) * voices);
            //memset(&md->vmh[md->hardvoices], num*sizeof(MD_VOICESET *), 0);
            md->hardvoices = voices;
            md->device.SetHardVoices(md, voices);
        } else
        {   md->vms = (MD_VOICESET **)_mm_realloc(md->allochandle, md->vms, sizeof(MD_VOICESET *) * voices);
            //memset(&md->vms[md->softvoices], num*sizeof(MD_VOICESET *), 0);
            for(i=md->softvoices; i<voices; i++)
                md->vms[i] = NULL;
            md->softvoices = voices;
            md->device.SetSoftVoices(md, voices);
        }
    }
    return num;
}

// =====================================================================================
    static void freevoices(MDRIVER *md, int mixmode, int num)
// =====================================================================================
// Frees the desired number of voices from the the specified mixing mode.  If the
// value is greater than the number of voices currently allocated, then all voices
// for the mixing mode are deallocated.
{
    if(mixmode == MM_STATIC)
    {   if(md->hardvoices <= num)
        {   md->device.SetHardVoices(md, md->hardvoices = 0);
            _mm_free(md->allochandle, md->vmh);
        } else
        {   md->hardvoices -= num;
            md->device.SetHardVoices(md, md->hardvoices);
            // Don't bother reallocating - just a waste of cycles with no
            // memory gain (unless someone deallocated like 5000 voices...)
        }
    } else
    {   if(md->softvoices <= num)
        {   md->device.SetSoftVoices(md, md->softvoices = 0);
            _mm_free(md->allochandle, md->vms);
        } else
        {   md->softvoices -= num;
            md->device.SetSoftVoices(md, md->softvoices);
            // See above.. :)
        }
    }
}


// =====================================================================================
    MD_VOICESET *Voiceset_Create(MDRIVER *md, MD_VOICESET *owner, uint voices, uint flags)
// =====================================================================================
// Creates a new voice set.  Makes the voiceset the child of the given 'owner.'  The 
// voiceset has no parent if owner is null.
// Returns a handle to the new voice set.   returns -1 on error!
{
    MD_VOICESET *cruise, *vs;

    if(!md) return NULL;

    vs  = _mm_calloc(md->allochandle, 1, sizeof(MD_VOICESET));

    if(!owner) flags &= ~MDVS_INHERITED_VOICES;

    if(cruise = md->voiceset)
    {   while(cruise->next)
            cruise = cruise->next;

        cruise->next = vs;
    } else
        md->voiceset = vs;

    if(owner)
    {
        vs->owner = owner;
        if(cruise = owner->children)
        {   while(cruise->nextchild)
                cruise = cruise->nextchild;

            cruise->nextchild = vs;
        } else
            owner->children = vs;
    }
        

    // Make sure the specified mixing mode is available

    if(flags & MDVS_STATIC)
    {   if(md->hardvoices == -1)
            flags &= ~MDVS_STATIC;
    } else
    {   if(md->softvoices == -1)
            flags |= MDVS_STATIC;
    }

    vs->flags   = flags;
    vs->md      = md;

    Voiceset_SetNumVoices(vs, voices);

	//lets initialize its volumes!
	Voiceset_SetVolume(vs,128);

    return vs;
}
    

// =====================================================================================
    void Voiceset_SetPlayer(MD_VOICESET *vs, uint (*player)(struct MD_VOICESET *voiceset, void *playerinfo), void *playerinfo)
// =====================================================================================
// playerinfo is only set if not NULL, else the current playerinfo is left untouched.
{
    vs->player = player;
    if(playerinfo) vs->playerinfo = playerinfo;

}


// =====================================================================================
    MD_VOICESET *Voiceset_CreatePlayer(MDRIVER *md, MD_VOICESET *owner, uint voices, uint (*player)(struct MD_VOICESET *voiceset, void *playerinfo), void *playerinfo, BOOL hardware)
// =====================================================================================
{
    MD_VOICESET *vs;
    
    vs = Voiceset_Create(md, owner, voices, MDVS_PLAYER | (hardware ? MDVS_STATIC : 0));
    Voiceset_SetPlayer(vs, player, playerinfo);

    return vs;
}


// =====================================================================================
    void Voiceset_Free(MD_VOICESET *vs)
// =====================================================================================
{
    if(vs)
    {
        MDRIVER *md = vs->md;

        if(vs->vdesc && !(vs->flags & MDVS_INHERITED_VOICES))
        {
            MD_VOICESET **cruise = (vs->flags & MDVS_STATIC) ? md->vmh : md->vms;
            uint          i;

            for(i=0; i<vs->voices; i++)
            {   md->device.VoiceStop(&md->device, vs->vdesc[i].voice);
                cruise[vs->vdesc[i].voice] = NULL;
            }

            _mm_free(md->allochandle, vs->vdesc);

            // Check for excess voices at the end of md_vm to free.  This is very ugly
            // code, I know.. but it works.  I can fix it up later.

            if(vs->flags & MDVS_STATIC)
            {   i = md->hardvoices;
                while(i) if(md->vmh[--i]) break;
                if((md->hardvoices - i) > 5) freevoices(md, MM_STATIC, md->hardvoices - i);
            } else
            {   //i = md->softvoices;
                //while(i) if(md->vms[--i]) break;
                //if((md->softvoices - i) > 10) freevoices(md, MM_DYNAMIC, md->softvoices - i);
            }
        }

        // remove this voiceset from the 'LISTS' (eh heheh!)

        if(vs->owner)
        {
            // make sure our parent disowns us
            if(vs == vs->owner->children)
            {   vs->owner->children = vs->nextchild;
            } else
            {   MD_VOICESET *cruise = vs->owner->children;
                while(vs != cruise->next)
                    cruise = cruise->nextchild;

                cruise->nextchild = vs->nextchild;
            }
        }

        if(vs == md->voiceset)
        {   md->voiceset = vs->next;
        } else
        {   MD_VOICESET *cruise = md->voiceset;
            while(vs != cruise->next)
                cruise = cruise->next;

            cruise->next = vs->next;
        }

        _mm_free(md->allochandle, vs);
    }
}


static void __inline recurse_set_vdesc(MD_VOICESET *vs, MD_VDESC *vdesc, uint voices)
{
    MD_VOICESET    *cruise = vs->children;

    while(cruise)
    {
        if(vs->flags & MDVS_INHERITED_VOICES)
            recurse_set_vdesc(cruise, vdesc, voices);
        cruise = cruise->next;
    }

    vs->vdesc = vdesc;
    if(vs->voices > voices) vs->voices = voices;
}


// =====================================================================================
    int Voiceset_SetNumVoices(MD_VOICESET *vs, uint voices)
// =====================================================================================
// Set the number of voices allocated to an audio type.  Provided mainly for use by the 
// player only,and is generally a good idea to avoid using this function unless the 
// voiceset in question is not in use currently.  Increasing the number of voices is 
// usually fine (tho may not always succeed).

// Note: Decreasing the number of voices simply strips out the voices from the end of the
// set's voice list.  In the future, I may make it smart:
//  a) unused voices are deallocated first
//  b) non-critical voices are deallocated next
//  c) rest are deallocated from the back
{
    if(!vs) return 0;

    if(!(vs->flags & MDVS_INHERITED_VOICES))
    {
        // Master Voiceset Voice Allocation
        // --------------------------------
        // Master voicesets get physical voices directly from the driver's voice pool.
        // Masters are not limited by their parent's voice limitations.

        uint           i, k;
        int            t, mdv;
        MD_VOICESET  **cruise, **vmc;
        MDRIVER       *md;

        md = vs->md;

        if(voices > vs->voices)
        {
            vs->vdesc = (MD_VDESC *)_mm_realloc(vs->md->allochandle, vs->vdesc, voices * sizeof(MD_VDESC));
            memset(&vs->vdesc[vs->voices], 0, (voices-vs->voices) * sizeof(MD_VDESC));
            
            // Allocate voices to this voiceset
            // --------------------------------
            // Unfortunately this is a somewhat complex procedure.  We have to determine if we
            // want to allocate from static or dynamic voicesets.  If we are allocating from
            // static, then we need to check if any are available, and if not, fall back on
            // dynamic voices.

            // Finally, if the allocation fails, we simply allocate as many as possible.  Some
            // are better than none.
   
            i = t = 0;
            if(vs->flags & MDVS_STATIC)
            {   vmc    = md->vmh;
                mdv    = md->hardvoices;
            } else
            {   vmc    = md->vms;
                mdv    = md->softvoices;
            }

            // fan out anything that is already allocated...

            while((i < voices) && (t < mdv))
            {   if(vmc[t] == NULL)
                {   vmc[t] = vs;
                    vs->vdesc[i++].voice = t;
                }
                t++;
            }
    
            if(i < voices)
            {   voices -= i;
                k = allocvoices(md,(vs->flags & MDVS_STATIC) ? MM_STATIC : MM_DYNAMIC, voices);

                if(k < voices)
                {   // Great, it couldn't fulfill our request.  Just allocate as many as possible.
                    voices = k;
                } else voices += i;
    
                // reget some important variables ----
                if(vs->flags & MDVS_STATIC)
                {   vmc    = md->vmh;
                    mdv    = md->hardvoices;
                } else
                {   vmc    = md->vms;
                    mdv    = md->softvoices;
                }
   
                while(i < voices)
                {   vmc[t] = vs;
                    md->device.VoiceStop(&md->device, t);
                    vs->vdesc[i++].voice = t;
                    t++;
                }
            }
        } else if(voices < vs->voices)
        {   // for now, just remove the last x number of allocations

            cruise = (vs->flags & MDVS_STATIC) ? vs->md->vmh : vs->md->vms;
            for(i=vs->voices-1; i>=voices; i--)
                cruise[vs->vdesc[i].voice] = NULL;
        }

        vs->voices = voices;

        // Update Dependant Children
        // -------------------------
        // Any children who have inherited our vdesc pool need to be updated to
        // ensure their vdesc pointers are correct!

        recurse_set_vdesc(vs, vs->vdesc, vs->voices);

    } else
    {
        vs->vdesc = vs->owner->vdesc;
        if(vs->voices > vs->owner->voices)
            vs->voices = vs->owner->voices;
    }

    return voices;
}

	
// =====================================================================================
    static void __inline vs_disableoutput(MD_VOICESET *vs)
// =====================================================================================
// Stops the mixer in its tracks for the voices within this set.  No mixing or updates
// of the voices will occur, leaving the voices in the exact same state until resumed 
// with vs_enableoutput.
{
    uint   i;
    for(i=0; i<vs->voices; i++)
    {   // Have the driver stop updating all voices.  Note, however, that the
        // driver still retains all voice data in that voice!

        if(!vs->md->device.VoiceStopped(&vs->md->device, vs->vdesc[i].voice))
        {   vs->vdesc[i].flags |= MDVD_PAUSED;
            vs->md->device.VoiceStop(&vs->md->device, vs->vdesc[i].voice);
        }
    }

    vs->flags |= MDVS_FROZEN;
}


// =====================================================================================
    static void __inline vs_enableoutput(MD_VOICESET *vs)
// =====================================================================================
{
    uint   i;
    for(i=0; i<vs->voices; i++)
        if(vs->vdesc[i].flags & MDVD_PAUSED)
        {   vs->md->device.VoiceResume(&vs->md->device, vs->vdesc[i].voice);
            vs->vdesc[i].flags &= ~MDVD_PAUSED;
        }

    vs->flags &= ~MDVS_FROZEN;
}


// =====================================================================================
    static void __inline vs_reset(MD_VOICESET *vs)
// =====================================================================================
// stops all voices, and reinitializes the player-update timer (countdown)
{
    uint    i;
    for(i=0; i<vs->voices; i++)
        Voice_Stop(vs,i);

    // reset the player timer.
    vs->countdown = 0;
}


// =====================================================================================
    void Voiceset_DisableOutput(MD_VOICESET *vs)
// =====================================================================================
// Stops the mixer in its tracks for the voices within this set.  No mixing or updates
// of the voices will occur, leaving the voices in the exact same state until resumed 
// with Voiceset_EnableOutput.
// Note: this is the end-user API for the vs_disableoutput function.  Processes the
// given voiceset's children in addition to itself.
{
    MD_VOICESET *cruise;

    if(!vs) return;

    cruise = vs->children;

    while(cruise)
    {   vs_disableoutput(cruise);
        cruise = cruise->next;
    }
    vs_disableoutput(vs);
}


// =====================================================================================
    void Voiceset_EnableOutput(MD_VOICESET *vs)
// =====================================================================================
{
    MD_VOICESET *cruise;

    if(!vs) return;

    cruise = vs->children;

    while(cruise)
    {   vs_enableoutput(cruise);
        cruise = cruise->next;
    }
    vs_enableoutput(vs);
}


// =====================================================================================
    void Voiceset_Reset(MD_VOICESET *vs)
// =====================================================================================
// stops all voices, and reinitializes the player-update timer (countdown).  Processes
// all chidlren of the voiceset.
{
    MD_VOICESET *cruise;

    if(!vs) return;

    cruise = vs->children;

    while(cruise)
    {   vs_reset(cruise);
        cruise = cruise->next;
    }
    vs_reset(vs);
}


// =====================================================================================
    void Voiceset_PlayStart(MD_VOICESET *vs)
// =====================================================================================
{
    if(!(vs->flags & MDVS_PLAYER))
    {
        // set the player flag, and preempt the mixer to get this thing going
        // as soon as possible!

        vs->flags |= MDVS_PLAYER;
        vs->md->device.Preempt(&vs->md->device);
    }
}

// =====================================================================================
    void Voiceset_PlayStop(MD_VOICESET *vs)
// =====================================================================================
{
    if(vs) vs->flags &= ~MDVS_PLAYER;
}


/*
// =====================================================================================
    _VS_STREAM *md_streaminstance(MD_STREAM *stream)
// =====================================================================================
{
    _VS_STREAM *vstream;

    vstream = (_VS_STREAM *)_mm_malloc(sizeof(_VS_STREAM));

    vstream->streaminfo = stream->streaminfo;
    vstream->callback   = stream->callback;
    vstream->blocksize  = stream->blocksize;
    vstream->numblocks  = stream->numblocks;
    vstream->handle     = -1;
    vstream->oldpos     = 0;

    return vstream;
}
*/

// =====================================================================================
    void Voiceset_Callback(MD_VDESC *vdesc, void *bufpos, ulong pos, uint format)
// =====================================================================================
{
/*    // call our callback function with the appropriate area to fill with data.

    if(vdesc->stream->numblocks > 1)
    {   // Block mode....
        vdesc->stream->callback(vdesc, bufpos, vdesc->stream->blocksize);
        pos += vdesc->stream->blocksize;
        if(pos > (vdesc->stream->numblocks * vdesc->stream->blocksize))
            pos = 0;

        vs->md->device.SampleSetCallback(vdesc->stream->handle, pos, Voiceset_Callback, vdesc);
    } else
    {   // hard headed streaming mode
        if(vdesc->stream->oldpos < vdesc->stream->blocksize)
        {   vdesc->stream->callback(vdesc, bufpos, (vdesc->stream->blocksize - pos));
        }
    }*/
}


// ==================================================
// ----        Mikmod Voice Control API          ----
// ==================================================

// Voice bounds checking macro.  Makes life simpler.  (Macros are good!)

#define VoiceCheck() ((vs->flags & MDVS_FROZEN) || (voice >= vs->voices))

// =====================================================================================
    static void MD_UpdateVoiceVolume(MD_VOICESET *vs, MD_VDESC *vdesc)
// =====================================================================================
{
    MMVOLUME   q;
    int        pan_lr;

    // Run this volume through all kinds of layers of modifiers, including
    // the voiceset volume, the voiceset panning, and finally the voice
    // panning.

    pan_lr = (vdesc->pan_lr * vs->md->pansep) / 128;
    pan_lr = _mm_boundscheck(pan_lr, PAN_LEFT, PAN_RIGHT);
    if(vs->md->mode & DMODE_REVERSE) pan_lr = -pan_lr;

    q.front.left  = ((vs->absvol * vdesc->volume) * (PAN_RIGHT - pan_lr) * (PAN_REAR - vdesc->pan_fb)) / (128*128);
    q.front.right = ((vs->absvol * vdesc->volume) * (PAN_RIGHT + pan_lr) * (PAN_REAR - vdesc->pan_fb)) / (128*128);
    //q.rear.left  = ((vs->absvol * vdesc->volume) * (PAN_RIGHT - vdesc->pan_lr) * (PAN_REAR + vdesc->pan_fb)) / (128*128);
    //q.rear.right  = ((vs->absvol * vdesc->volume) * (PAN_RIGHT + vdesc->pan_lr) * (PAN_REAR + vdesc->pan_fb)) / (128*128);

    q.front.left  *= ((PAN_RIGHT - vs->pan_lr) * (PAN_REAR - vs->pan_fb)) / 128;
    q.front.right *= ((PAN_RIGHT + vs->pan_lr) * (PAN_REAR - vs->pan_fb)) / 128;
    //q.rear.left  *= ((PAN_RIGHT - vs->pan_lr) * (PAN_REAR + vs->pan_fb)) / 128;
    //q.rear.right  *= ((PAN_RIGHT + vs->pan_lr) * (PAN_REAR + vs->pan_fb)) / 128;

    // remove the fixed point (uses a constant that auto-adjusts to panning range changes)

    q.front.left /= (PAN_RIGHT * PAN_REAR);
    q.front.right /= (PAN_RIGHT * PAN_REAR);
    //q.rear.left  /= (PAN_RIGHT * PAN_REAR);
    //q.rear.right  /= (PAN_RIGHT * PAN_REAR);

    vs->md->device.VoiceSetVolume(&vs->md->device, vdesc->voice, &q);
}


/*
// =====================================================================================
    void Voice_SetStream(MD_VOICESET *vs, uint voice, MD_STREAM *stream, uint format)
// =====================================================================================
// setup a voice for streaming audio based on the give streaming information and sample
// format.  A block of memory is allocated from the driver to allow the stream to function,
// and the stream data is attached to the voiceset's voice.
{
    MD_VDESC *vdesc;
    uint     length;

    vdesc = &vs->vdesc[voice];
    length = stream->blocksize * stream->numblocks;
    
    // create a sample that can be streamed to and from.
    vdesc->stream = md_streaminstance(stream);

    // set the flags - remove all advanced looping options, since they don't make for
    // good streaming.  And force standard looping because it does... :)

    format &= ~(SL_BIDI | SL_REVERSE | SL_SUSTAIN_LOOP | SL_SUSTAIN_BIDI | SL_SUSTAIN_REVERSE);
    format |= SL_LOOP;
    
    vdesc->stream->handle = vs->md->device.SampleAlloc(length, &format);
    vs->md->device.VoicePlay(voice, vdesc->stream->handle, 0, length, 0, length, 0, 0, format);

    // Call our callback once to start, so that the mixer doesn't get
    // bogged down initializing several streams at once. (bad!)
    // The callback also sets the mixer to continte to call it.

    Voiceset_Callback(vdesc, vs->md->device.SampleGetPtr(vdesc->stream->handle),0,format);
}
*/


// =====================================================================================
    void Voice_EasyStream(MD_VOICESET *vs, uint blocksize, uint numblocks, void *param,
                          void (*callback)(struct MD_VOICESET *voiceset, uint voice, void *dest, uint len), uint format)
// =====================================================================================
// bufsize is size in samples. its better that way.
// if numblocks is greater than one, then the streaming is put into block mode, where
// the callback function is called only when a complete block is ready to be
// filled (and the len will always be bufsize)
{
//    int       i;
//    MD_VDESC  *vdesc;

    
/*    vdesc = vs->vdesc[v];

    // check blockmode and stuff

    if(numblocks)
    {   samplesize = blocksize*numblocks;
    }
    else
    {   samplesize = blocksize;
    }
*/
}


// =====================================================================================
    MD_STREAM *Stream_Create(uint blocksize, uint numblocks, void *param, void (*callback)(struct MD_VOICESET *voiceset, uint voice, void *dest, uint len))
// =====================================================================================
{
/*    _VS_STREAM *stream;

    stream = (_VS_STREAM *)_mm_malloc(sizeof(_VS_STREAM);

    stream->streaminfo = param;
    stream->callback   = callback;
    stream->blocksize  = blocksize;
    stream->numblocks  = numblocks;*/

    return NULL;
}
    

// =====================================================================================
    void Voice_SetVolume(MD_VOICESET *vs, uint voice, int vol)
// =====================================================================================
{    
    // all the checks
    if(VoiceCheck()) return;
    _mm_boundscheck(vol,0,128);
    if(vs->vdesc[voice].volume == vol) return;

    vs->vdesc[voice].volume = vol;
    MD_UpdateVoiceVolume(vs, &vs->vdesc[voice]);
}


// =====================================================================================
    void Voice_SetFrequency(MD_VOICESET *vs, uint voice, long frq)
// =====================================================================================
{
    if(VoiceCheck()) return;
    //if(md_sample[voice]!=NULL && md_sample[voice]->divfactor!=0) frq/=md_sample[voice]->divfactor;
    vs->md->device.VoiceSetFrequency(&vs->md->device, vs->vdesc[voice].voice, frq);
}


// =====================================================================================
    void Voice_SetPanning(MD_VOICESET *vs, uint voice, int pan_lr, int pan_fb)
// =====================================================================================
{
    if(VoiceCheck()) return;

    // Surround Sound sloppiness
    // I basically hate the implimentation I have set up here.  Surely there
    // is a better way, but without better understanding how 4/5/6 channel stereo
    // is implemented I am pretty much powerless.
    
    if(pan_lr != PAN_SURROUND)
    {   vs->vdesc[voice].pan_lr = _mm_boundscheck(pan_lr, PAN_LEFT, PAN_RIGHT);
        vs->vdesc[voice].pan_fb = _mm_boundscheck(pan_fb, PAN_FRONT, PAN_REAR);
    } else
        vs->vdesc[voice].pan_lr = vs->vdesc[voice].pan_fb = PAN_CENTER;

    MD_UpdateVoiceVolume(vs, &vs->vdesc[voice]);
//    if(pan_lr == PAN_SURROUND) vs->md->device.VoiceSetSurround(vs->vdesc[voice].voice);
}


// =====================================================================================
    void Voice_Resume(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    vs->md->device.VoiceResume(&vs->md->device, vs->vdesc[voice].voice);
    vs->vdesc[voice].flags &= ~MDVD_PAUSED;
}


// =====================================================================================
    void Voice_Play(MD_VOICESET *vs, uint voice, int handle, long start, int length, int reppos, int repend, int suspos, int susend, uint flags)
// =====================================================================================
// Signals to the driver to start playing a voice. if the loopstart vaules are illegal 
// (<0 or >length), or handle is illegal, the loop will not be played.
{
    if(VoiceCheck() || (handle < 0) || (length <= 0) || (start > length)) return;

    if(repend > length) repend = length;    // repend can't be bigger than size
    if(susend > length) susend = length;

    if(reppos >= repend)  flags &= ~SL_LOOP;
    if(suspos >= susend)  flags &= ~SL_SUSTAIN_LOOP;

    vs->md->device.VoicePlay(&vs->md->device, vs->vdesc[voice].voice, handle, start, length, reppos, repend, suspos, susend, flags);
}


// =====================================================================================
    void Voice_SetPosition(MD_VOICESET *vs, uint voice, long start) 
// =====================================================================================
{
    if(VoiceCheck()) return;
    vs->md->device.VoiceSetPosition(&vs->md->device, vs->vdesc[voice].voice, start);
}


// =====================================================================================
    void Voice_ReleaseSustain(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return;
    vs->md->device.VoiceReleaseSustain(&vs->md->device, vs->vdesc[voice].voice);
}


// =====================================================================================
    void Voice_Pause(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return;
    vs->md->device.VoiceStop(&vs->md->device, vs->vdesc[voice].voice);
    vs->vdesc[voice].flags |= MDVD_PAUSED;
}


// =====================================================================================
    void Voice_Stop(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return;
    vs->md->device.VoiceStop(&vs->md->device, vs->vdesc[voice].voice);
    vs->vdesc[voice].flags &= ~MDVD_PAUSED;
}


// =====================================================================================
    BOOL Voice_Paused(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return 0;
    return (vs->vdesc[voice].flags & MDVD_PAUSED);
}


// =====================================================================================
    BOOL Voice_Stopped(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return 0;
    if(vs->vdesc[voice].flags & MDVD_PAUSED) return 0;

    return(vs->md->device.VoiceStopped(&vs->md->device, vs->vdesc[voice].voice));
}


// =====================================================================================
    SLONG Voice_GetPosition(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return 0;
    return(vs->md->device.VoiceGetPosition(&vs->md->device, vs->vdesc[voice].voice));
}


// =====================================================================================
    ULONG Voice_RealVolume(MD_VOICESET *vs, uint voice)
// =====================================================================================
{
    if(VoiceCheck()) return 0;
    return(vs->md->device.VoiceRealVolume(&vs->md->device, vs->vdesc[voice].voice));
}


// =====================================================================================
    void Voice_SetResonance(MD_VOICESET *vs, uint voice, int cutoff, int resonance)
// =====================================================================================
{
    if(VoiceCheck()) return;    
    vs->md->device.VoiceSetResonance(&vs->md->device, vs->vdesc[voice].voice, cutoff, resonance);
}


// =====================================================================================
    int Voice_Find(MD_VOICESET *vs, uint flags)
// =====================================================================================
{
    uint  c;

    // we start our search with sfxpool, because that is the 'oldest'
    // sample playing in our list.  It is not an accurate way to
    // determine the oldest, but it is effective enough, and above
    // all uses simple and fast logic!

    c = vs->sfxpool;
 
    do
    {
        if(Voice_Stopped(vs, c))
        {   // found a silent voice, so use it!
            vs->vdesc[c].flags = flags;
            vs->sfxpool        = c+1;
            if(vs->sfxpool >= vs->voices) vs->sfxpool = 0;
            return c;
        }

        c++;
        if(c >= vs->voices) c = 0;
    } while(c != vs->sfxpool);

    // no silent voices, so pick sfxpool (if it isn't cirtical)
    // So do a while loop to find first non-critical voice.
    
    while(vs->vdesc[c].flags & MDVD_CRITICAL)
    {   c++;
        if(c >= vs->voices) c = 0;
        if(c==vs->sfxpool) return -1;    // woops no voices!
    };

    // ** Return our selected voice **

    vs->vdesc[c].flags = flags;
    vs->sfxpool        = c+1;
    if(vs->sfxpool    >= vs->voices) vs->sfxpool = 0;

    return c;
}


// ========================================================================
// ----        Mikmod's **Voiceset** Volume Management System          ----
// ========================================================================
/*
  Notes and Terminology:
   - relative volume (vs->volume)
       the volume of a voiceset (0..128) relative to its parent voiceset.
       this value should be thought of by the user as the volume property
       of the voiceset.

   - absolute volume (vs->absvol)
       the volume of a voiceset (0..128) in absolute mikmod (driver) terms.
       This is an internal variable.  End users should not know this exists.

   - maxvolume
       the maximum absolute volume a voiceset could have due to diminished
       volumes of parent voicesets.  At a later point, we can add this to the
       MD_VOICESET struct and avoid havign to repeat part of Voiceset_SetVolume
       as often (the upwards-traversing part calling get_max_volume)


  set our new relative volume, then find out, by upwards-traversing the voiceset
free, the maximum absolute volume that this voiceset could have.  Knowing that,
and its relative volume, we can find out its absolute volume, by a simple
proportion calculation.
After that, we tell mikmod to recalculate all its voice volumes, since this
voiceset now has a new absolute volume.
Then we modify all the children voicesets, since their theoretical absolute
volumes have changed, even though their relative volumes have not.  (Relative
volume--relative to what?  When the thing that its relative to changes, it
changes.)

TODO: later on, by putting max_volume in each MD_VOICESET, we can merge these
two functions.
*/

// =====================================================================================
    static int __inline get_max_volume(MD_VOICESET *vs)
// =====================================================================================
{
	if(vs->owner)
    	return (vs->volume * get_max_volume(vs->owner)) / 128;
	else
		return vs->volume * 0x1000;
}

// =====================================================================================
    static void child_volupd(MD_VOICESET *vs, int effective_volume)
// =====================================================================================
{
	MD_VOICESET *cruise;
    uint         i;

	// If the absolute volume didn't change, let's make like snow cones
    // and melt away the cpu cycles.

    if(vs->absvol == (effective_volume / 0x1000)) return;
    
    vs->absvol = effective_volume / 0x1000;

	// set my own voices
	for(i=0; i<vs->voices; i++) MD_UpdateVoiceVolume(vs, &vs->vdesc[i]);

	// set my children voicesets. my effective volume will be their max volume
	cruise = vs->children;
	while(cruise)
	{   child_volupd(cruise,(cruise->volume*effective_volume) / 128);
        cruise = cruise->nextchild;
	}
}

// =====================================================================================
    void Voiceset_SetVolume(MD_VOICESET *vs, int volume)
// =====================================================================================
{
	int max_volume;

	if(!vs) return;
    if(volume == vs->volume) return;
        
    // Set the relative volume
    vs->volume = volume;

	// find out our maximum absolute volume
	max_volume = get_max_volume(vs);

    // if we have no parent voiceset, our volumes are on a new scale of 0..128

    child_volupd(vs, max_volume);
}


// =====================================================================================
    int Voiceset_GetVolume(MD_VOICESET *vs)
// =====================================================================================
// Returns the current volume of the given voiceset.
{
    return vs ? vs->volume : 0;
}

