
#ifndef __RANDOM_H__
#define __RANDOM_H__

#include "mmio.h"

typedef struct DE_RAND
{   
    MM_ALLOC  *allochandle;
    ulong      seed,         // the original selected seed
               iseed;         // current seed progression
} DE_RAND;

extern DE_RAND   *drand_create(MM_ALLOC *allochandle, ulong seed);
extern void       drand_setseed(DE_RAND *Sette, ulong seed);
extern long       drand_getlong(DE_RAND *hand, uint bits);
extern void       drand_free(DE_RAND *freeme);
extern void       drand_reset(DE_RAND *resetme);

// These two functions do not use a 'localized object' like the ones above.  Instead
// they work on the global random number generator, making them useful as a drop-in
// replacement for Microsoft's shitty random number generator.

extern void       rand_setseed(ulong seed);
extern BOOL       rand_getbit(void);
extern long       rand_getlong(uint bits);
extern void       rand_setseed(ulong seed);
#endif
