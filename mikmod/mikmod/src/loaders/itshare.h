
#ifndef _ITSHARE_H_
#define _ITSHARE_H_

#include "uniform.h"

enum
{   STMEM_CHANVOLSLIDE = PTMEM_LAST,
    STMEM_GLOBVOLSLIDE,
    STMEM_VOLSLIDE,
    STMEM_PANSLIDE,
    STMEM_PITCHSLIDE,
    STMEM_TREMOR,
    STMEM_RETRIG,
    STMEM_TEMPO,
    STMEM_SPEED,
    STMEM_PANBRELLO_SPEED,
    STMEM_PANBRELLO_DEPTH,
    STMEM_SEFFECT,
    STMEM_ARPEGGIO,
    STMEM_LAST
};

extern uint   S3MIT_ProcessCmd(UTRK_WRITER *ut, UBYTE *poslookup, unsigned int cmd, unsigned int inf, BOOL oldeffect, int gxxmem);
#endif
