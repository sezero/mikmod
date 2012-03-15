#ifndef _MMERROR_H_
#define _MMERROR_H_

// ===============================================
//  Error handling and logging stuffs - MMERROR.C
// ===============================================

// Generic Error Codes
// -------------------
// Notes:
//  - These should always be relatively generalized. (ie, you probably
//    shouldn't have to add any new errors).  More specific error infor-
//    mation is provided via logging and error strings.
//

enum
{
    MMERR_NONE = 0,
    MMERR_INVALID_PARAMS,
    MMERR_OPENING_FILE,         // read-only or protection error!
    MMERR_UNSUPPORTED_FILE,
    MMERR_OUT_OF_MEMORY,
    MMERR_END_OF_FILE,
    MMERR_DISK_FULL,
    MMERR_DETECTING_DEVICE,     // as relating to video/audio/etc drviers
    MMERR_INITIALIZING_DEVICE,  // as relating to video/audio/etc drviers
    MMERR_FATAL                 // generic fatal error
};

// -------------------------------------------------------------------------------------
    typedef struct MM_ERRINFO
// -------------------------------------------------------------------------------------
// Error info struct, for dealing with general errors of a variety of brands.  This allows
// mmio-based libs to submit errors which can subsequently be handled by the calling
// program.  See also: _mmerr_sethandler!
{
    int         num;              // error number
    CHAR       *heading;          // error heading (window heading)
    CHAR       *desc;             // description of the error

    void      (*handler)(struct MM_ERRINFO *errinfo);
    MM_ALLOC   *allochandle;

} MM_ERRINFO;


// Error / Logging prototypes
// --------------------------

MMEXPORT void    _mmlog_init(void (__cdecl *log)(const CHAR *fmt, ... ), void (__cdecl *logv)(const CHAR *fmt, ... ), void (__cdecl *logc)(const CHAR *fmt, ... ));
MMEXPORT void    _mmlog_exit(void);

MMEXPORT void    (__cdecl *_mmlog)(const CHAR *fmt, ... );          // general logging, default to log file/window
MMEXPORT void    (__cdecl *_mmlogv)(const CHAR *fmt, ... );         // for verbose messages (important msgs to main window)
MMEXPORT void    (__cdecl *_mmlogc)(const CHAR *fmt, ... );         // for critical errors!

MMEXPORT void    _mmerr_sethandler(void (*func)(struct MM_ERRINFO *errinfo));
MMEXPORT void    _mmerr_set(int errie, const CHAR *head, const CHAR *desc);
MMEXPORT void    _mmerr_setsub(int errie, const CHAR *head, const CHAR *desc);
MMEXPORT CHAR   *_mmerr_get_heading(void);
MMEXPORT CHAR   *_mmerr_get_desc(void);
MMEXPORT int     _mmerr_get_ident(void);
MMEXPORT void    _mmerr_handler(void);

#endif      // _MMERROR_H_