
#ifndef _GETOPT_H_
#define _GETOPT_H_

#ifdef __cplusplus
extern "C" {
#endif

// ================
// GETOPT.C Defines
// ================

/*
 Types:

  P_BOOLEAN  : Looks for a + or - immidiately after the option.
               If none found (space or other option), -1 is passed.

  P_NUMVALUE : Grabs the value after the option (whitespace ignored).
               If no value is given, -1 is passed.

  P_STRING   : Grabs the string after the option (leading whitespace
               is ignored).  If no string was present, NULL is returned.

 Notes:

   A filename or string is normally terminated by a space (always a single
   word long).  If a filename or string is enclosed in quotations ("blah
   blah"), then the string is not terminated until the closing quote is
   encountered.
               
*/

typedef struct FILESTACK
{   struct  FILESTACK *prev,*next;
    CHAR    *path;            // full path, including filename
    ULONG   size;             // Size of the file
} FILESTACK;


typedef struct P_OPTION
{   CHAR  *token;             // option token (string)
    UBYTE type;               // type of option
} P_OPTION;


typedef struct P_PARSE
{   int   num;                    // number of options
    struct P_OPTION *option;      // array of options
} P_PARSE;


typedef union P_VALUE
{   SLONG number;            // numeric return value
    CHAR  *text;             // string return value
} P_VALUE;

#define P_STRING        32
#define P_BOOLEAN       64
#define P_NUMVALUE     128

#define EX_FULLSORT  0
#define EX_FILESORT  1          

int  ngetopt(CHAR *token, P_PARSE *parse, int argc, CHAR *argv[], void (*post)(int, P_VALUE *));
BOOL ex_init(CHAR *dir, CHAR *filemask, int sort);
void ex_exit(void);

extern FILESTACK *filestack;
extern BOOL sortbydir;       // set this to have getopt to catagorize filenames
                             // by the way they are given on the command line.

#ifdef __cplusplus
}
#endif

#endif

