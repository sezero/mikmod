#ifndef _PSFORBID_H_
#define _PSFORBID_H_

#include "mplayer.h"

// =====================================================================================
    static void __inline MP_WipePosPlayed(MPLAYER *ps)
// =====================================================================================
{
    if(ps->state.pos_played)
    {   uint   i;
        for(i=0; i<ps->mf->numpos; i++)
            memset(ps->state.pos_played[i],0,sizeof(ULONG) * ((ps->mf->pattrows[ps->mf->positions[i]]/32)+1));
    }
}

static BOOL __inline MP_PosPlayed(MPLAYER *ps)
{
    if(!ps->state.pos_played) return 0;
    return (ps->state.pos_played[ps->state.sngpos][(ps->state.patpos/32)] & (1<<(ps->state.patpos&31)));
}


static void __inline MP_SetPosPlayed(MPLAYER *ps)
{
    if(ps->state.pos_played) ps->state.pos_played[ps->state.sngpos][(ps->state.patpos/32)] |= (1<<(ps->state.patpos&31));
}

static void __inline MP_UnsetPosPlayed(MPLAYER *ps)
{
    if(ps->state.pos_played) ps->state.pos_played[ps->state.sngpos][(ps->state.patpos/32)] &= ~(1<<(ps->state.patpos&31));
}

// =====================================================================================
    static void __inline MP_LoopSong(MPLAYER *ps, const UNIMOD *pf)
// =====================================================================================
{
    if((pf->reppos >= (int)pf->numpos) || (pf->reppos == 0))
    {
        ps->state.sngpos        = 0;
        ps->state.patpos        = 0;
        ps->state.volume        = pf->initvolume;
        ps->state.sngspd        = pf->initspeed;
        ps->state.bpm           = pf->inittempo;
        ps->state.strip_timeout = pf->strip_threshold;

        // reset channel volumes to module default
        memcpy(ps->state.chanvol, pf->chanvol, 64*sizeof(pf->chanvol[0]));

        //Player_Cleaner(ps);
    } else ps->state.sngpos = pf->reppos;
}


// Use the forbid API when you want to modify any of the ps->sngpos, ps->patpos etc.
// variables and clear it when you're done. This prevents getting strange results
// due to intermediate interrupts.

#define ps_forbid_init()    _mmforbid_init();
#define ps_forbid_deinit()  _mmforbid_deinit(ps->mutex);
#define ps_forbid()         _mmforbid_enter(ps->mutex);
#define ps_unforbid()       _mmforbid_exit(ps->mutex);


#endif
