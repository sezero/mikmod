/*
 
 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Hour 13 Studios (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 --------------------------------------------------
 mmerror.c

 Mikmod Portable Error Handling and logging function layer.  Error handling and
 logging facilities that allow the end userto configure mikmod to use any pre-
 defined error handler and/or logging system.

 Remarks
  The Error system is divided into two branchs.  The Logging System is intended for
  developer messages and high-detail bug reporting.  The Error system is intended for
  reporting messages to the user in (duh) user-readable fashion.  So when using the
  varions _mmerr_* functions, take care to make error messages that are user-readable
  and not overly-detailed.

  Use of mmlogc should be limited to critical errors only, and should be used in such
  a way that the output string would be readable in the context of a MessageBox or
  other immediate user notification window.

 NOTES:
  - Register an error handler with _mmerr_sethandler();

  - If you want to report logging errors to the user as MessageBox windows or
    whatever, then just intercept mmlogc, which is used for any critical errors.

 Portability:
  All systems - all compilers
*/


#include "mmio.h"
#include <stdarg.h>


// ===========================
// private variables
// These have been structurized for future per-thread data storage.
// (as soon as I figure out how to do that in a portable manner :)

static MM_ERRINFO errinfo = { 0 };

static void __cdecl mmlog(const CHAR *fmt, ... )
{
}

static void __cdecl mmlogv(const CHAR *fmt, ... )
{
}

static void __cdecl mmlogc(const CHAR *fmt, ... )
{
}

// ============================
//   Public Function Pointers
// ============================

void (__cdecl *_mmlog)  (const CHAR *fmt, ... ) = mmlog;
void (__cdecl *_mmlogv) (const CHAR *fmt, ... ) = mmlogv;
void (__cdecl *_mmlogc) (const CHAR *fmt, ... ) = mmlogc;


// ===================================
//   Public error/logging interfaces
// ===================================

// ===========================================================================
    void _mmlog_init(void (__cdecl *log)(const CHAR *fmt, ... ), void (__cdecl *logv)(const CHAR *fmt, ... ), void (__cdecl *logc)(const CHAR *fmt, ... ))
// ===========================================================================
{
    _mmlog  = log  ? log  : mmlog;
    _mmlogv = logv ? logv : mmlogv;
    _mmlogc = logc ? logc : mmlogc;
}

// ===========================================================================
    void _mmlog_exit(void)
// ===========================================================================
{
    _mmlog  = mmlog;
    _mmlogv = mmlogv;
    _mmlogc = mmlogc;
}

// ===========================================================================
    void _mmerr_sethandler(void (*func)(struct MM_ERRINFO *errinfo))
// ===========================================================================
{
    errinfo.handler = func;
}

// ===========================================================================
    void _mmerr_handler(void)
// ===========================================================================
{
    if(errinfo.handler) errinfo.handler(&errinfo);
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
    void _mmerr_set(int errie, const CHAR *head, const CHAR *desc)
// ===========================================================================
// head   - heading for error (used as a messagebox heading usually)
// desc   - Qualified error message... error window body
{
    if(!errinfo.allochandle)
        errinfo.allochandle  = _mmalloc_create("MM Error Handler", NULL);

    _mm_free(errinfo.allochandle, errinfo.desc);
    _mm_free(errinfo.allochandle, errinfo.heading);

    errinfo.num     = errie;
    errinfo.heading = _mm_strdup(errinfo.allochandle, head);
    errinfo.desc    = _mm_strdup(errinfo.allochandle, desc);

    // automatically call the automated error handler.
    if((errie != MMERR_NONE) && errinfo.handler) errinfo.handler(&errinfo);
}


// ===========================================================================
    void _mmerr_setsub(int errie, const CHAR *head, const CHAR *desc)
// ===========================================================================
// Set the error and call the errorhandler if an error is not already set.
// This function must be used in conjunction with a call to _mmerr_set(MMERR_NONE, NULL);
// to ensure that the error information base is cleared.
{
    if(errinfo.num != MMERR_NONE)
    {
        if(!errinfo.allochandle)
            errinfo.allochandle  = _mmalloc_create("MM Error Handler", NULL);

        _mm_free(errinfo.allochandle, errinfo.desc);
        _mm_free(errinfo.allochandle, errinfo.heading);

        errinfo.num     = errie;
        errinfo.heading = _mm_strdup(errinfo.allochandle, head);
        errinfo.desc    = _mm_strdup(errinfo.allochandle, desc);

        // automatically call the automated error handler.
        if((errie != MMERR_NONE) && errinfo.handler) errinfo.handler(&errinfo);
    }
}


CHAR *_mmerr_get_desc(void)
{
    return errinfo.desc;
}

CHAR *_mmerr_get_heading(void)
{
    return errinfo.heading;
}

int _mmerr_get_ident(void)
{
    return errinfo.num;
}
