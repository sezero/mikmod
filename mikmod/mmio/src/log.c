/*
 
 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Divine Entertainment (1996-2000) and

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 ------------------------------------------------------
 module: log.c
 
 A generic set of logging functions included with the mikmod package for
 optional use.  These functions can either log messages to display or to a
 specified log file (or both).

 All output to display uses Stdout functions - puts.
 All output to file uses MMSTREAM functions - _mm_fputs.

 If any errors occur during initialization of or during output to the
 logfile, automatic full verbose output to stdout is enabled.

 Note:
  Other than the included example programs, mikmod is in no way dependant
  on this module.  See mmerror.c for the mikmod logging 'layer' and how to
  have mikmod use your own logging facilities.
*/

#include "mmio.h"
#include "log.h"
#include <string.h>
#include <stdarg.h>


static BOOL      verbose   = 1;
static MMSTREAM *log_file  = NULL;

static CHAR     *str;

// =====================================================================================
    int log_init(const CHAR *logfile, BOOL val)
// =====================================================================================
// Initializes the system for logging.  The log file and the option of
// verbose output are set here.
//
// logfile - If the log file specified is NULL, no log file will be used.  If a log file
// already exists, it is renamed as a .old file.  DO NOT specify an extension, a .txt
// will automatically be attached!
//
// Suggested lognames would be DIVLOG or the like.
//
// Returns 0  if successful.
//         1  if error opening the logfile (in this case, verbose logging to
//            stdout is automatically enabled)
{
    CHAR *backup;

    if((str = calloc(1024, sizeof(CHAR))) == NULL) return 1;

    _mmlog_init(log_print, log_printv); // register the logging routines with mmerror

    if(logfile && logfile[0])
    {   // - Check for the file and make a backup if it exists.
        // - Create our logfile.

        if((backup = malloc(strlen(logfile)+5)) == NULL)
        {   log_file = NULL;  return 1;  }

        strcpy(backup,logfile);  strcat(backup,".old");
        strcpy(str,logfile);     strcat(str,".txt");
        remove(backup);
        rename(str,backup);
        free(backup);

        if((log_file = _mm_fopen(str,"wb")) == NULL) return 1;
    } else log_file = NULL;

    verbose = val;
    return 0;
}


// =====================================================================================
    void log_exit(void)
// =====================================================================================
// Closes up the log file.
{
    if(log_file)
    {   _mm_fclose(log_file);
        log_file = NULL;
    }

    if(str) free(str);
    str = NULL;
}


void log_verbose(void) {  verbose = 1;  }
void log_silent(void)  {  verbose = 0;  }


// =====================================================================================
    void __cdecl log_print(const CHAR *fmt, ... )
// =====================================================================================
// Direct replacement for printf using logging features.  Anything logged using this 
// function is only displayed if the user envokes verbose initialization.
{
    if(str)
    {   static va_list argptr;
        ULONG cnt=0;

        va_start(argptr, fmt);
        cnt = vsprintf(str, fmt, argptr);
        str[cnt + 1] = 0;
        va_end(argptr);

        if(log_file)
        {   _mm_fputs(log_file,str);
            fflush(log_file->fp);
        }
        if(verbose) puts(str);
    }
}


// =====================================================================================
    void __cdecl log_printv(const CHAR *fmt, ... )
// =====================================================================================
// Direct replacement for printf using logging features.  Anything logged using this
// function will ALWAYS be displayed to the console (useful for critical errors and such).
{
    if(str)
    {   static va_list argptr;
        ULONG cnt=0;

        va_start(argptr, fmt);
        cnt = vsprintf(str, fmt, argptr);
        str[cnt + 1] = 0;
        va_end(argptr);

        if(log_file)
        {  _mm_fputs(log_file,str);
           fflush(log_file->fp);
        }
        puts(str);
    }
}

