/*

 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Divine Entertainment (1996-2000) and
  Jan Lönnberg (2001)

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 ------------------------------------------------------
 module: mmalloc.c  (that's mm - alloc!)
 
 The Mikmod Portable Memory Allocation Layer

*/

#include "mmforbid.h"

#include "mmio.h"

// =====================================================================================
    MM_FORBID *_mmforbid_init(void)
// =====================================================================================
{
    MM_FORBID  *forbid;

    forbid = (MM_FORBID *)_mm_calloc(NULL, 1, sizeof(MM_FORBID));
    
    pthread_mutex_init(forbid, NULL); /* Default attributes assumed. */

    return forbid;
}


// =====================================================================================
    void _mmforbid_deinit(MM_FORBID *forbid)
// =====================================================================================
{
    if(forbid)
    {
      pthread_mutex_destroy(forbid); 
      _mm_free(NULL, forbid);
    }
}


// =====================================================================================
    void _mmforbid_enter(MM_FORBID *forbid)
// =====================================================================================
{
  if(forbid) pthread_mutex_lock(forbid);  
}


// =====================================================================================
    void _mmforbid_exit(MM_FORBID *forbid)
// =====================================================================================
{
  if(forbid) pthread_mutex_unlock(forbid);
}

