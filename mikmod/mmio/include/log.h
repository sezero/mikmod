#ifndef _LOG_H_
#define _LOG_H_

#include "mmio.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================
//  LOG.C Prototypes
// ==================

#define LOG_SILENT   0
#define LOG_VERBOSE  1

extern int  log_init    (const CHAR *logfile, BOOL val);
extern void log_exit    (void);
extern void log_verbose (void);
extern void log_silent  (void);
extern void __cdecl log_print   (const CHAR *fmt, ... );
extern void __cdecl log_printv  (const CHAR *fmt, ... );


#ifdef __WATCOMC__
#pragma aux log_init    parm nomemory modify nomemory;
#pragma aux log_exit    parm nomemory modify nomemory;
#pragma aux log_verbose parm nomemory modify nomemory;
#pragma aux log_silent  parm nomemory modify nomemory;
#pragma aux printlog    parm nomemory modify nomemory;
#pragma aux printlogv   parm nomemory modify nomemory;
#endif


#ifdef __cplusplus
};
#endif

#endif
