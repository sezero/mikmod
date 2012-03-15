/*
  --> GetOpt : Grabs filenames and options from the command-line

 Gets options from the command-line, and I mean this is nice stuff!
 Long Filename Support: No! [but soon, I hope]

*/


#include <string.h>
#include <process.h>
#include <errno.h>
#include <dos.h>

#ifdef DJGPP
#include <dir.h>
#define MAXNAME MAXPATH
#define _MAX_FNAME MAXFILE
#define _MAX_EXT MAXEXT
#define _splitpath fnsplit
#define _makepath fnmerge
#endif
#include "mmio.h"
#include "mmgetopt.h"

#define Numeric(x)        (x>='0' && x<='9')
#if defined(__WATCOMC__) && !defined(__CHAR_SIGNED__)
  #define AlphaNumeric(x)   ((x != 32) && (x != 255) && (x != 0))
#else
#define AlphaNumeric(x)   ((x != 32) && (x != -1) && (x != 0))
#endif

FILESTACK *filestack;
unsigned int fcounter;
int sortbydir = 1;

static CHAR *ex_defdir, *ex_defmask;

BOOL strcheck(CHAR *checker, CHAR *checkie)
{
    int t;

    for(t=0; t<strlen(checker) && t<strlen(checkie); t++)
       if(checker[t] != checkie[t]) break;

    return(t==strlen(checker));
}

// max values taken from STDIO.H's __NT__ defines to support Win95 long
// filenames.

#ifdef __WATCOMC__
#define MAXPATH    260
#define MAXDRIVE     3
#define MAXDIR     256
#define MAXNAME    256
#define MAXEXT     256
#endif

static CHAR *wildname = NULL, *path = NULL, *drive = NULL,
             *dir = NULL, *fname = NULL, *ext = NULL;

static struct find_t ffblk;
static BOOL (*cmpfunc)(CHAR *path1, CHAR *path2);


// Finds the first file in a directory that corresponds to the wildcard
// name 'wildname'.
//
// returns: ptr to full pathname or NULL if file couldn't be found

static CHAR *GetFirstName(CHAR *wildname, int attrib)
{
    _splitpath(wildname,drive,dir,fname,ext);

    if(!_dos_findfirst(wildname,attrib,&ffblk))
    {  _splitpath(ffblk.name,NULL,NULL,fname,ext);
       _makepath(path,drive,dir,fname,ext);
       return path;
    }
    return NULL;
}


// Finds another file in a directory that corresponds to the wildcard
// name of the GetFirstName call.
// returns:
//   ptr to full pathname or NULL if file couldn't be found
static CHAR *GetNextName(void)
{
    if(!_dos_findnext(&ffblk))
    {  _splitpath(ffblk.name,NULL,NULL,fname,ext);
       _makepath(path,drive,dir,ffblk.name,NULL);
       return path;
    }
#ifndef DJGPP
    _dos_findclose(&ffblk);
#endif
    return NULL;
}

static BOOL sort_byfull(CHAR *path1, CHAR *path2)
// sort files using the FULL path of the file - so files will be sorted
// by their directories as well as their names!
{
    return(strcmp(path1,path2) <= 0);
}

static BOOL sort_byname(CHAR *path1, CHAR *path2)
// Sort files by their name only - directories do NOT affect final order.
{
    CHAR dname1[_MAX_FNAME],dname2[_MAX_FNAME],dext[_MAX_EXT];

    _splitpath(path1,NULL,NULL,dname1,dext);
    strcat(dname1,dext);
    _splitpath(path2,NULL,NULL,dname2,dext);
    strcat(dname2,dext);

    return(strcmp(dname1,dname2) <= 0);
}


BOOL ex_init(CHAR *defdir, CHAR *filemask, int sort)

// Initializes the wildcard expander and configures the default assumptions
// for the wildcard expander (MS-DOS systems only).
// Returns 1 on error, _mm_error set
//         0 if successful

{
    path  = (CHAR *)_mm_calloc(MAXPATH,1);
    drive = (CHAR *)_mm_calloc(MAXDRIVE,1);
    dir   = (CHAR *)_mm_calloc(MAXDIR,1);
    fname = (CHAR *)_mm_calloc(MAXNAME,1);
    ext   = (CHAR *)_mm_calloc(MAXEXT,1);

    if(ext == NULL) return 1;

    ex_defmask = filemask;
    ex_defdir  = defdir;

    switch(sort)
    {  case EX_FULLSORT: cmpfunc = sort_byfull;  break;
       case EX_FILESORT: cmpfunc = sort_byname;  break;
    }

    return 0;
}


void ex_exit(void)
{
    if(path!=NULL)  free(path);
    if(drive!=NULL) free(drive);
    if(dir!=NULL)   free(dir);
    if(fname!=NULL) free(fname);
    if(ext!=NULL)   free(ext);

    path  = NULL;
    drive = NULL;
    dir   = NULL;
    fname = NULL;
    ext   = NULL;
}

// adds an item to the FILESTACK linked list -- items are added to the
// start of the list faster that way :)
static void TackOn(CHAR *path, ULONG size)
{
    FILESTACK *work, *cruise, *ctmp;
    CHAR *doh;

    doh = strdup(path);
    work = (FILESTACK *)_mm_malloc(sizeof(FILESTACK));
    work->path = doh;  work->size = size;
    work->prev = NULL; work->next = NULL;

    if((cruise=filestack) == NULL)
    {   filestack = work; fcounter++;
        return;
    }

    while(cruise!=NULL)
    {   if(cmpfunc(path,cruise->path)) break;
        ctmp = cruise;
        cruise = cruise->next;
    }

    // now add it to the linked list between cruise->prev and cruise
    work->next = cruise;

    if(cruise != NULL)
    {   work->prev   = cruise->prev;
        cruise->prev = work;
    } else
        work->prev   = ctmp;   //->prev;

    if(work->prev != NULL)
        work->prev->next = work;

    // If it's tacked on at the top of the list, reassign 'filestack'
    if(cruise==filestack) filestack=work;

    fcounter++;
}

// returns 0 if no files were found at all
static BOOL ExpandFilename(CHAR *pattern)
{
    CHAR      *s;
    FILESTACK *temp,*cruise;

    if(sortbydir)
    {   temp = filestack;
        filestack = NULL;
    }
         
    if(strpbrk(pattern,"*?")==NULL)
    {   if((s=GetFirstName(pattern,0))==NULL)
        {   // check it for special cases (null filename, etc)
            if(ex_defmask!=NULL && ex_defmask[0]!=NULL)
            {   _makepath(wildname,NULL,pattern,ex_defmask,NULL);
                if((s=GetFirstName(wildname,0))==NULL) goto errorexit;
                if(strpbrk(ex_defmask,"*?")==NULL)
                    TackOn(s,ffblk.size);
                else
                {   do 
                    {   TackOn(s,ffblk.size);
                    } while((s=GetNextName()) != NULL);
                }
            }
        } else
            TackOn(s,ffblk.size);
    } else
    {   if((s=GetFirstName(pattern,0)) == NULL) goto errorexit;
        do
        {   TackOn(s,ffblk.size);
        } while((s=GetNextName()) != NULL);
    }

    if(sortbydir && ((cruise=temp)!=NULL))
    {  while(cruise->next!=NULL)
          cruise = cruise->next;
       filestack->prev = cruise;
       cruise->next = filestack;
       filestack = temp;
    }
    return 1;

errorexit:
    if(sortbydir)
       filestack = temp;
    return 0;
}


int nexpand(int argc, CHAR *argv[])
{
    int i = 1, pos = 0, ln;
    CHAR *tmp;

    tmp = (CHAR *)_mm_malloc(256*sizeof(CHAR));

    while(i < argc)
    {   if(argv[i][0] == 34)      // 34 ASCII == Double Quote (")
        {   ln = 0;
            while((argv[i][pos] != 34) && (ln < 255))
            {   tmp[ln++] = argv[i][pos++];
                if(pos >= strlen(argv[i]))  {  pos = 0;  i++;  }
            }
            tmp[ln] = NULL;
            ExpandFilename(tmp);
        } else
        {   ExpandFilename(argv[i]);
            i++;
            continue;
        }
        pos = 0;
    }

    free(tmp);
    return fcounter;
}


// returns the number of options grabbed from the list
int ngetopt(CHAR *token, P_PARSE *parse, int argc, CHAR *argv[], void (*post)(int, P_VALUE *))
{
    int     i,s,tokenlen,pos,ln;
    CHAR   *tmp;
    BOOL    def = 1;         // default search flag
    P_VALUE retval;

    if((token==NULL) || (parse==NULL) || (post==NULL))
        return nexpand(argc, argv);
    
    tmp = (CHAR *)_mm_malloc(256*sizeof(CHAR));

    tokenlen = strlen(token);
    pos = 0; i = 1;

    while(i < argc)
    {   if(pos==0)
        {   for(s=0; s<tokenlen; s++)
                if(argv[i][0] == token[s]) break;
            if(s == tokenlen)   // not an option.. maybe a file, put it on the file stack
            {   if(argv[i][0] == 34)      // 34 ASCII == Double Quote (")
                {   ln = 0;
                    while((argv[i][pos] != 34) && (ln < 255))
                    {   tmp[ln++] = argv[i][pos++];
                        if(pos >= strlen(argv[i]))  {  pos=0;  i++;  }
                    }
                    tmp[ln] = NULL;
                    ExpandFilename(tmp);
                } else
                {   ExpandFilename(argv[i]);
                }
                i++;  def = 0;  pos = 0;
                continue;
            }
            pos++;
        }

        if(parse==NULL)
        {   pos = 0;  i++;
            continue;
        }

        for(s=0; s<parse->num; s++)
            if(strcheck(parse->option[s].token, &argv[i][pos])) break;

        if(s == parse->num) {  pos = 0; i++; continue;  }
        pos += strlen(parse->option[s].token);

        switch(parse->option[s].type)
        {   case P_BOOLEAN:    // check next char for + / -
               if(strlen(argv[i]) > pos)
                   switch(argv[i][pos])
                   {   case '+': retval.number = 1;  post(s,&retval); pos++; break;
                       case '-': retval.number = 0;  post(s,&retval); pos++; break;
                       default : retval.number = -1; post(s,&retval);
                   }
               else retval.number = -1; post(s,&retval);
            break;

            case P_NUMVALUE:      // check next char / next arg for a value
               if(strlen(argv[i]) > pos)
               {   if(Numeric(argv[i][pos]))
                   {   ln = 0;
                       while((strlen(argv[i]) > pos) && Numeric(argv[i][pos]))
                       {   tmp[ln] = argv[i][pos];
                           ln++; pos++;
                       }
                       tmp[ln] = NULL;
                       retval.number = atoi(tmp); post(s,&retval);
                   } else
                   {   retval.number = 0;
                       post(s,&retval);
                   }
               } else
               {   i++; pos = 0;
                   if(Numeric(argv[i][pos]))
                   {   ln = 0;
                       while((strlen(argv[i]) > pos) && Numeric(argv[i][pos]))
                       {   tmp[ln] = argv[i][pos];
                           ln++; pos++;
                       } 
                       tmp[ln] = NULL;
                       retval.number = atoi(tmp); post(s,&retval);
                   } else
                   {   retval.number = 0;
                       post(s,&retval);
                   }
               }
            break;

            case P_STRING:               // Check for a string value
               if(strlen(argv[i]) > pos)
               {   ln = 0;
                   if(argv[i][pos] == 34)  // check for a double quote(")
                   {   while(argv[i][pos] != 34 && ln < 255)
                       {   tmp[ln++] = argv[i][pos++];
                           if(pos >= strlen(argv[i]))  {  pos=0;  i++;  }
                       }
                       tmp[ln] = NULL;
                   } else if(AlphaNumeric(argv[i][pos]))
                   {   while((strlen(argv[i]) > pos) && AlphaNumeric(argv[i][pos]))
                           tmp[ln++] = argv[i][pos++];
                       tmp[ln] = NULL;
                       retval.text = tmp;  post(s,&retval);
                   } else
                   {   retval.text = NULL;
                       post(s,&retval);
                   }
               } else
               {   i++;  pos=0;  ln=0;
                   if(argv[i][pos] == 34)  // check for a double quote(")
                   {   while(argv[i][pos] != 34 && ln < 255)
                       {   tmp[ln++] = argv[i][pos++];
                           if(pos >= strlen(argv[i]))  {  pos=0;  i++;  }
                       }
                       tmp[ln] = NULL;
                   } else if(AlphaNumeric(argv[i][pos]))
                   {   while((strlen(argv[i]) > pos) && AlphaNumeric(argv[i][pos]))
                       {   tmp[ln] = argv[i][pos];
                           ln++; pos++;
                       } 
                       tmp[ln] = NULL;
                       retval.text = tmp;  post(s,&retval);
                   } else
                   {   retval.text = NULL;
                       post(s,&retval);
                   }
               }
            break;
        }

        if(pos >= strlen(argv[i])) {  pos=0;  i++;  }
    }

    // check if we perform a default directory search?
    if(def && ex_defdir!=NULL && ex_defdir[0]!=NULL)
       ExpandFilename(ex_defdir);

    free(tmp);
    return fcounter;
}

