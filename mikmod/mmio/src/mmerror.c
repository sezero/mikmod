/*
 
 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Divine Entertainment (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 --------------------------------------------------
 Module: mmerror.c

 Mikmod Portable Error Handling and logging function layer.  Error handling and
 logging facilities that allow the end userto configure mikmod to use any pre-
 defined error handler and/or logging system.

 NOTES:
  - Register an error handler with _mmerr_sethandler();

 Portability:
  All systems - all compilers
*/


#include "mmio.h"
#include <stdarg.h>


// ===========================
// private variables
// These have been structurized for future per-thread data storage.
// (as soon as I figure out how to do that in a portable manner :)

static struct
{
    int   num;              // error number
    CHAR *desc;             // description of the error
    void (*handler)(int error_this, const CHAR *my_bad);
} errinfo;

static void __cdecl mmlog(const CHAR *fmt, ... )
{
}

static void __cdecl mmlogv(const CHAR *fmt, ... )
{
}

// ============================
//   Public Function Pointers
// ============================

void (__cdecl *_mmlog)  (const CHAR *fmt, ... ) = mmlog;
void (__cdecl *_mmlogv) (const CHAR *fmt, ... ) = mmlogv;


// ===================================
//   Public error.logging interfaces
// ===================================

// ===========================================================================
    void _mmlog_init(void (__cdecl *log)(const CHAR *fmt, ... ), void (__cdecl *logv)(const CHAR *fmt, ... ))
// ===========================================================================
{
    _mmlog  = log;
    _mmlogv = logv;
}


// ===========================================================================
    void _mmlog_exit(void)
// ===========================================================================
{
    _mmlog  = mmlog;
    _mmlogv = mmlogv;
}

// ===========================================================================
    void _mmerr_sethandler(void (*func)(int, const CHAR *))
// ===========================================================================
{
    errinfo.handler = func;
}

// ===========================================================================
    void _mmerr_handler(void)
// ===========================================================================
{
    if(errinfo.handler) errinfo.handler(errinfo.num, errinfo.desc);
}

// ==========================================================================
// Error Setting / Handling
//
// A call to _mmerr_set will set the mmerror information (for later retrival
// via _mm_geterror) and calls the error handler, if there is one.  For added
// functionality, there is now _mm_errsub, which only sets the error an error
// has not already been triggered.  This is especially useful for portable
// driver systems like in Mikmod where the individual drivers usually do not
// generate custom error messages - but can if the authors want them to.

// ===========================================================================
    void _mmerr_set(int errie, const CHAR *errah)
// ===========================================================================
{
    errinfo.num  = errie;
    errinfo.desc = _mm_strdup(NULL, errah);

    // automatically call the automated error handler.
    if((errie != MMERR_NONE) && errinfo.handler) errinfo.handler(errinfo.num, errinfo.desc);
}


// ===========================================================================
    void _mmerr_setsub(int errie, const CHAR *errah)
// ===========================================================================
// Set the error and call the errorhandler if an error is not already set.
// This function must be used in conjunction with a call to _mmerr_set(MMERR_NONE,NULL);
// to ensure that the error information base is cleared.
{
    if(errinfo.num != MMERR_NONE)
    {   errinfo.num  = errie;
        errinfo.desc = _mm_strdup(NULL, errah);

        // automatically call the automated error handler.
        if((errie != MMERR_NONE) && errinfo.handler) errinfo.handler(errinfo.num, errinfo.desc);
    }
}


CHAR *_mmerr_getstring(void)
{
    return(errinfo.desc);
}


int _mmerr_getinteger(void)
{
    return(errinfo.num);
}


