/* Divine Entertainment Presents -->

  == The Random Bit
  == A comp-sci standard in decent random number generation

  This is a replacement random number generator for the woefully not-so-random
  Visual C random number generator.  I have tested this bitch and it *is*
  quite effectively random.  And fast.  Good stuff!

  For added coolness:
   I have encapsulated the generation code into a mini-object which makes
   this thing nice and easy to use for generating full integers.

  Notes:
   - It seems the random number sequence in the beginning (after a new seed has
     been assigned) isn't very random.  As a fix, I now automatically call the
     getbit function 30 times.  That seems to get us past that area of crappy
     generations.


Alternate Generation Schemes
-----------

#define tnrandseed  50

#define random(num) (nrand() % num)

long blnrandnext = tnrandseed;

tu16 nrand() {
  blnrandnext = (((blnrandnext * 1103515249) + 12347) % 0xFFFF);
  return blnrandnext;
}

-----------

tu16 nrand() {
   blnrandnext = ((blnrandnext * 1103515245) + 12345) );
// lower bit are no too random and only 31 bits are valid
   return (blnrandnext>>15) & 0xFFFF;
}

uint mrand( uint max ) {
   return (((((blnrandnext = ((blnrandnext * 1103515245) +
12345))>>15)&0xFFFF)*max)>>16) ;

}

-----------
*/

#include "random.h"
#include <stdlib.h>

#define INIT_BITS        29    // See notes above.

#define IB1               1
#define IB2               2
#define IB5              16
#define IB18         131072
#define MASK   (IB1+IB2+IB5)

static ulong    globseed;

// =====================================================================================
    BOOL __inline rand_getbit(void)
// =====================================================================================
// generates a random bit!
{
    if(globseed & IB18)
    {   globseed = ((globseed - MASK) << 1) | IB1;
        return 1;
    } else
    {   globseed <<= 1;
        return 0;
    }
}


// =====================================================================================
    void rand_setseed(ulong seed)
// =====================================================================================
{
    int  i;
    
    if(!(seed & (IB18-1))) seed += 8815;
    globseed = seed;
    for(i=0; i<INIT_BITS; i++) rand_getbit();
}


// =====================================================================================
    long rand_getlong(uint bits)
// =====================================================================================
// Generates an integer with the specified number of bits.
// Bits are shifted on in reverse since if we do them 'normally' the numbers generated
// tend to be in 'groups' - ie, it will pick several numbers in the 10-50 range, then
// the 80-170 range, and so on.
{
    ulong retval = 0;
    uint  i;

    for(i=bits; i; i--)
        retval |= rand_getbit() << (i-1);

    return (long)retval;
}


// ===========================================================================
    BOOL __inline getbit(ulong *iseed)
// ===========================================================================
// generates a random bit!  Used to generate the noise sample.
{
    if(*iseed & IB18)
    {   *iseed = ((*iseed - MASK) << 1) | IB1;
        return 1;
    } else
    {   *iseed <<= 1;
        return 0;
    }
}


// =====================================================================================
    DE_RAND *drand_create(MM_ALLOC *allochandle, ulong seed)
// =====================================================================================
{
    DE_RAND  *newer_than_you;

    newer_than_you = (DE_RAND *)_mm_malloc(allochandle, sizeof(DE_RAND));
    newer_than_you->allochandle = allochandle;
    drand_setseed(newer_than_you, seed);
    return newer_than_you;
}


// =====================================================================================
    void drand_setseed(DE_RAND *Sette, ulong seed)
// =====================================================================================
// Note that the seed is not garunteed to be the same as what you pass, so please read
// drand->seed after calling this procedure!
{
    if(Sette)
    {   int       i;

        if(!(seed & (IB18-1))) seed += 8815;

        Sette->seed  = seed;
        Sette->iseed = seed;

        for(i=0; i<INIT_BITS; i++) getbit(&Sette->iseed);
    }
}


// =====================================================================================
    long drand_getlong(DE_RAND *hand, uint bits)
// =====================================================================================
// Generates an integer with the specified number of bits.
{
    ulong retval = 0;
    uint  i;
    
    for(i=bits; i; i--)
        retval |= getbit(&hand->iseed) << (i-1);

    return retval;
}


// =====================================================================================
    void drand_free(DE_RAND *freeme)
// =====================================================================================
{
    if(freeme) _mm_free(freeme->allochandle, freeme);
}


// =====================================================================================
    void drand_reset(DE_RAND *resetme)
// =====================================================================================
{
    resetme->iseed = resetme->seed;
}
