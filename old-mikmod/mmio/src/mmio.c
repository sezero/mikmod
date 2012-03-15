/*
 
 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Hour 13 Studios (1996-2002) and
     Jean-Paul Mikkers (1993-1996)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 --------------------------------------------------
 mmio.c

 Miscellaneous portable I/O routines.. used to solve some portability
 issues (like big/little endian machines and word alignment in structures).

 Notes:
  Expanded to allow for streaming from a memory block as well as from
  a file.  Also, set up in a manner which allows easy use of packfiles
  without having to overload any functions (faster and easier!).

 Portability:
  All systems - all compilers

 -----------------------------------
 The way this module works - By Jake Stine [Air Richter]

 - _mm_read_I_UWORD and _mm_read_M_UWORD have distinct differences:
   the first is for reading data written by a little endian (intel) machine,
   and the second is for reading big endian (Mac, RISC, Alpha) machine data.

 - _mm_write functions work the same as the _mm_read functions.

 - _mm_read_string is for reading binary strings.  It is basically the same
   as an fread of bytes.
*/                                                                                     

#include "mmio.h"
#include "mminline.h"
#include <string.h>

#ifndef COPY_BUFSIZE
#define COPY_BUFSIZE  1024
#endif

static UBYTE  _mm_cpybuf[COPY_BUFSIZE];


// =====================================================================================
    static void __inline mfp_init(MMSTREAM *mfp)
// =====================================================================================
{
#ifndef MM_FAST_FILEIO
    mfp->fread    = fread;
    mfp->fwrite   = fwrite;
    mfp->fgetc    = fgetc;
    mfp->fputc    = fputc;
    mfp->fseek    = fseek;
    mfp->ftell    = ftell;
    mfp->grrr     = feof;
#endif
}


// =====================================================================================
    MMSTREAM *_mmstream_createfp(FILE *fp, int iobase)
// =====================================================================================
// Creates an MMSTREAM structure from the given file pointer and seekposition
{
    MMSTREAM     *mmf;

    mmf = (MMSTREAM *)_mm_malloc(NULL, sizeof(MMSTREAM));

    mmf->fp      = fp;
    mmf->iobase  = iobase;
    mmf->dp      = NULL;
    mmf->seekpos = 0;

    mfp_init(mmf);

    return mmf;
}


// =====================================================================================
    MMSTREAM *_mmstream_createmem(void *data, int iobase)
// =====================================================================================
// Creates an MMSTREAM structure from the given memory pointer and seekposition
{
    MMSTREAM *mmf;

    mmf = (MMSTREAM *)_mm_malloc(NULL, sizeof(MMSTREAM));

    mmf->dp        = (UBYTE *)data;
    mmf->iobase    = iobase;
    mmf->fp        = NULL;
    mmf->seekpos   = 0;

    mfp_init(mmf);

    return mmf;
}


#ifndef MM_FAST_FILEIO
// =====================================================================================
    void _mmstream_setapi(MMSTREAM *mmf, 
           int (__cdecl *fread)(void *buffer, size_t size, size_t count, FILE *stream),
           int (__cdecl *fwrite)(const void *buffer, size_t size, size_t count, FILE *stream),
           int (__cdecl *fgetc)(FILE *stream),
           int (__cdecl *fputc)(int c, FILE *stream),
           int (__cdecl *fseek)(FILE *stream, long offset, int origin),
           int (__cdecl *ftell)(FILE *stream),
           int (__cdecl *feof)(FILE *stream))
// =====================================================================================
{
    if(!mmf) return;

    mmf->fread    = fread;
    mmf->fwrite   = fwrite;
    mmf->fgetc    = fgetc;
    mmf->fputc    = fputc;
    mmf->fseek    = fseek;
    mmf->ftell    = ftell;
    mmf->grrr     = feof;
}
#endif


// =====================================================================================
    void _mmstream_delete(MMSTREAM *mmf)
// =====================================================================================
{
    _mm_free(NULL, mmf);
}


// =====================================================================================
    void StringWrite(const CHAR *s, MMSTREAM *fp)
// =====================================================================================
// Specialized file output procedure.  Writes a UWORD length and then a
// string of the specified length (no NULL terminator) afterward.
{
    int slen;

    if(s==NULL)
    {   _mm_write_I_UWORD(0,fp);
    } else
    {   _mm_write_I_UWORD(slen = strlen(s),fp);
        _mm_write_UBYTES((UBYTE *)s,slen,fp);
    }
}


// =====================================================================================
    CHAR *StringRead(MMSTREAM *fp)
// =====================================================================================
// Reads strings written out by StringWrite above:  a UWORD length followed by length 
// characters.  A NULL is added to the string after loading.
{
    CHAR  *s;
    UWORD len;

    len = _mm_read_I_UWORD(fp);
    if(len==0)
    {   s = _mm_calloc(NULL, 16, sizeof(CHAR));
    } else
    {   if((s = (CHAR *)_mm_malloc(NULL, len+1)) == NULL) return NULL;
        _mm_read_UBYTES((UBYTE *)s,len,fp);
        s[len] = 0;
    }

    return s;
}


// =====================================================================================
    MMSTREAM *_mm_tmpfile(void)
// =====================================================================================
{
    MMSTREAM *mfp;

    mfp           = _mmobj_allocblock(NULL, MMSTREAM);
    mfp->fp       = tmpfile();
    mfp->iobase   = 0;
    mfp->dp       = NULL;

    mfp_init(mfp);

    return mfp;
}


// =====================================================================================
    MMSTREAM *_mm_fopen(const CHAR *fname, const CHAR *attrib)
// =====================================================================================
{
    MMSTREAM *mfp = NULL;

    if(fname && attrib)
    {
        FILE     *fp;
        if((fp=fopen(fname, attrib)) == NULL)
        {
            CHAR   sbuf[_MAX_PATH*2];
            sprintf(sbuf, "Failed opening file: %s\nSystem message: %s\n"
                          "(Please ensure the file exists and that you have read/write permissions and access to the file, and then try again)",
                          fname, _sys_errlist[errno]);
        
            _mmerr_set(MMERR_OPENING_FILE, "Error opening file!", sbuf);
            _mmlogd2("Error opening file: %s > %s",fname, _sys_errlist[errno]);
            return NULL;
        }

        mfp           = _mmobj_allocblock(NULL, MMSTREAM);
        mfp->fp       = fp;
        mfp->iobase   = 0;
        mfp->dp       = NULL;

        mfp_init(mfp);
    }

    return mfp;
}


// =====================================================================================
    void _mm_fclose(MMSTREAM *mmfile)
// =====================================================================================
{
    if(mmfile)
    {   if(mmfile->fp) fclose(mmfile->fp);
        _mm_free(NULL, mmfile);
    }
}

#ifndef MM_FAST_FILEIO
#define _my_fseek  stream->fseek
#define _my_ftell  stream->ftell
#define _my_feof   stream->grrr
#define _my_fread  fp->fseek
#else
#define _my_fseek  fseek
#define _my_ftell  ftell
#define _my_feof   feof
#define _my_fread  fread
#endif

// =====================================================================================
    int _mm_fseek(MMSTREAM *stream, long offset, int whence)
// =====================================================================================
{
    if(!stream) return 0;

    if(stream->fp)
    {   // file mode...
        
        return _my_fseek(stream->fp,(whence==SEEK_SET) ? offset+stream->iobase : offset, whence);
    } else
    {   long   tpos = -1;
        switch(whence)
        {   case SEEK_SET: tpos = offset;                   break;
            case SEEK_CUR: tpos = stream->seekpos + offset; break;
            case SEEK_END: /*tpos = stream->length + offset;*/  break; // not supported!
        }
        if((tpos < 0) /*|| (stream->length && (tpos > stream->length))*/) return 1; // seek failed
        stream->seekpos = tpos;
    }

    return 0;
}


// =====================================================================================
    long _mm_ftell(MMSTREAM *stream)
// =====================================================================================
{
   if(!stream) return 0;
   return (stream->fp) ? (_my_ftell(stream->fp) - stream->iobase) : stream->seekpos;
}


// =====================================================================================
    BOOL __inline _mm_feof(MMSTREAM *stream)
// =====================================================================================
{
    if(!stream) return 1;
    if(stream->fp) return _my_feof(stream->fp);

    return 0;
}


// =====================================================================================
    BOOL _mm_fexist(CHAR *fname)
// =====================================================================================
{
   FILE *fp;
   
   if((fp=fopen(fname,"r")) == NULL) return 0;
   fclose(fp);

   return 1;
}


// =====================================================================================
    long _mm_flength(MMSTREAM *stream)
// =====================================================================================
{
   long   tmp, tmp2;

   tmp = _mm_ftell(stream);

   _mm_fseek(stream,0,SEEK_END);
   tmp2 = _mm_ftell(stream);

   _mm_fseek(stream,tmp,SEEK_SET);

   return tmp2;
}


// =====================================================================================
    long _flength(FILE *stream)
// =====================================================================================
{
   long tmp,tmp2;

   tmp = ftell(stream);
   fseek(stream,0,SEEK_END);
   tmp2 = ftell(stream);
   fseek(stream,tmp,SEEK_SET);
   return tmp2-tmp;
}


// =====================================================================================
    BOOL _copyfile(FILE *fpi, FILE *fpo, uint len)
// =====================================================================================
// Copies a given number of bytes from the source file to the destination file.  Copy 
// begins whereever the current read pointers are for the given files.  Returns 0 on error.
{
    ULONG todo;

    while(len)
    {   todo = (len > COPY_BUFSIZE) ? COPY_BUFSIZE : len;
        if(!fread(_mm_cpybuf, todo, 1, fpi))
        {   _mmerr_set(MMERR_END_OF_FILE, "Unexpected End of File", "An unexpected end of file error occured while copying a file.");
            return 0;
        }
        if(!fwrite(_mm_cpybuf, todo, 1, fpo))
        {   _mmerr_set(MMERR_DISK_FULL, "Disk Full Error. Ouch.", "Your disk got full while making a copy of a file.");
            return 0;
        }
        len -= todo;
    }

    return -1;
}


// =====================================================================================
    void _mm_write_stringz(const CHAR *data, MMSTREAM *fp)
// =====================================================================================
// Write an ASCIIZ-terminated string.  Only the number of bytes needed to represent the
// string are written to disk.
{
    if(data) _mm_write_UBYTES((UBYTE *)data, strlen(data), fp);
}


// =====================================================================================
    void _mm_write_string(const CHAR *data, const uint count, MMSTREAM *fp)
// =====================================================================================
// Writes a fixed number of bytes out of an array.  This function is essentially like
// _mm_write_I_UBYTES except behaves properly for UNICODE strings and the like if the
// compiled code happens to be in unicode mode.
// count   - size of the string buffer.  String buffers must be at least this long,
//           although the string itself can be shorter.
{
    if(data) _mm_write_UBYTES((UBYTE *)data, count, fp);
}


// =====================================================================================
    void _mm_fgets(MMSTREAM *fp, CHAR *buffer, uint buflen)
// =====================================================================================
// A non-sucky verison of fgets!  Keeps reading until the end-of-file or newline is
// encountered, regardless of if the buffer is filled or not before hand.  Also, any
// actual newline characters are ignored!
{
    if(buffer && (buflen > 1))
    {
        CHAR c    = _mm_read_UBYTE(fp);
        uint clen = 0;

        while(!_mm_feof(fp) && (c != 13) && (c != 10))
        {
            if(clen < buflen-1)
            {   buffer[clen] = c;
                clen++;
            }

            c  = _mm_read_UBYTE(fp);
        }

        buffer[clen] = 0;
    }
}


// =====================================================================================
    void _mm_fputs(MMSTREAM *fp, const CHAR *data)
// =====================================================================================
{
   if(data) _mm_write_UBYTES((UBYTE *)data, strlen(data), fp);

#ifndef __UNIX__
   _mm_write_UBYTE(13,fp);
#endif
   _mm_write_UBYTE(10,fp);
}


// =====================================================================================
//                            THE OUTPUT (WRITE) SECTION
// =====================================================================================

void _mm_write_SBYTE(SBYTE data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_SBYTE(data,fp); } else { datawrite_SBYTE(data,fp); }
}

void _mm_write_UBYTE(UBYTE data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_UBYTE(data,fp); } else { datawrite_UBYTE(data,fp); }
}

#ifdef _MSC_VER
#pragma optimize( "g", off )
#endif

void _mm_write_M_UWORD(UWORD data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_M_UWORD(data,fp); } else { datawrite_M_UWORD(data,fp); }
}

void _mm_write_I_UWORD(UWORD data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_I_UWORD(data,fp); } else { datawrite_I_UWORD(data,fp); }
}

void _mm_write_M_ULONG(ULONG data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_M_ULONG(data,fp); } else { datawrite_M_ULONG(data,fp); }
}

void _mm_write_I_ULONG(ULONG data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_I_ULONG(data,fp); } else { datawrite_I_ULONG(data,fp); }
}

void _mm_write_M_SWORD(SWORD data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_M_SWORD(data,fp); } else { datawrite_M_SWORD(data,fp); }
}

void _mm_write_I_SWORD(SWORD data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_I_SWORD(data,fp); } else { datawrite_I_SWORD(data,fp); }
}

void _mm_write_M_SLONG(SLONG data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_M_SLONG(data,fp); } else { datawrite_M_SLONG(data,fp); }
}

void _mm_write_I_SLONG(SLONG data, MMSTREAM *fp)
{
    if(fp->fp) { filewrite_I_SLONG(data,fp); } else { datawrite_I_SLONG(data,fp); }
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif


#define DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN(type_name, type)   \
void                                                             \
_mm_write_##type_name##S (const type *data, int number, MMSTREAM *fp)  \
{                                                                \
    if(fp->fp)                                                   \
    {   while(number>0)                                          \
        {   filewrite_##type_name(*data, fp);                    \
            number--;  data++;                                   \
        }                                                        \
    } else                                                       \
    {   while(number>0)                                          \
        {   datawrite_##type_name(*data, fp);                    \
            number--;  data++;                                   \
        }                                                        \
    }                                                            \
}


#define DEFINE_MULTIPLE_WRITE_FUNCTION_NORM(type_name, type)     \
void                                                             \
_mm_write_##type_name##S (const type *data, int number, MMSTREAM *fp)  \
{                                                                \
    if(fp->fp)                                                   \
    {   filewrite_##type_name##S(data,number,fp);                \
    } else                                                       \
    {   datawrite_##type_name##S(data,number,fp);                \
    }                                                            \
}

DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (SBYTE, SBYTE)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (UBYTE, UBYTE)


#ifdef _MSC_VER
#pragma optimize( "g", off )
#endif

#ifdef MM_BIG_ENDIAN
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_SWORD, SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_UWORD, UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_SLONG, SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_ULONG, ULONG)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_SWORD, SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_UWORD, UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_SLONG, SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_ULONG, ULONG)
#else
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_SWORD, SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_UWORD, UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_SLONG, SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_ULONG, ULONG)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_SWORD, SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_UWORD, UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_SLONG, SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_ULONG, ULONG)
#endif

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif


// =====================================================================================
//                              THE INPUT (READ) SECTION
// =====================================================================================


// ============
//   UNSIGNED
// ============

UBYTE _mm_read_UBYTE(MMSTREAM *fp)
{
    return((fp->fp) ? fileread_UBYTE(fp) : dataread_UBYTE(fp));
}

SBYTE _mm_read_SBYTE(MMSTREAM *fp)
{
    return((fp->fp) ? fileread_SBYTE(fp) : dataread_SBYTE(fp));
}


#ifdef _MSC_VER
#pragma optimize( "g", off )
#endif

UWORD _mm_read_I_UWORD(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_I_UWORD(fp) : dataread_I_UWORD(fp);
}

UWORD _mm_read_M_UWORD(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_M_UWORD(fp) : dataread_M_UWORD(fp);
}

ULONG _mm_read_I_ULONG(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_I_ULONG(fp) : dataread_I_ULONG(fp);
}

ULONG _mm_read_M_ULONG(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_M_ULONG(fp) : dataread_M_ULONG(fp);
}

SWORD _mm_read_M_SWORD(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_M_SWORD(fp) : dataread_M_SWORD(fp);
}

SWORD _mm_read_I_SWORD(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_I_SWORD(fp) : dataread_I_SWORD(fp);
}

SLONG _mm_read_M_SLONG(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_M_SLONG(fp) : dataread_M_SLONG(fp);
}

SLONG _mm_read_I_SLONG(MMSTREAM *fp)
{
    return (fp->fp) ? fileread_I_SLONG(fp) : dataread_I_SLONG(fp);
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif


int _mm_read_string(CHAR *buffer, const uint number, MMSTREAM *fp)
{
    if(fp->fp)
    {   _my_fread(buffer, sizeof(CHAR), number, fp->fp);
        return !_mm_feof(fp);
    } else
    {   memcpy(buffer,&fp->dp[fp->seekpos], sizeof(CHAR) * number);
        fp->seekpos += number;
        return 0;
    }
}


#define DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN(type_name, type)    \
int                                                              \
_mm_read_##type_name##S (type *buffer, uint number, MMSTREAM *fp) \
{                                                                \
    if(fp->fp)                                                   \
    {   while(number>0)                                          \
        {   *buffer = fileread_##type_name(fp);                  \
            buffer++;  number--;                                 \
        }                                                        \
    } else                                                       \
    {   while(number>0)                                          \
        {   *buffer = dataread_##type_name(fp);                  \
            buffer++;  number--;                                 \
        }                                                        \
    }                                                            \
    return !_mm_feof(fp);                                        \
}


#define DEFINE_MULTIPLE_READ_FUNCTION_NORM(type_name, type)      \
int                                                              \
_mm_read_##type_name##S (type *buffer, uint number, MMSTREAM *fp) \
{                                                                \
    if(fp->fp)                                                   \
    {   fileread_##type_name##S(buffer,number,fp);               \
    } else                                                       \
    {   dataread_##type_name##S(buffer,number,fp);               \
        fp->seekpos += number;                                   \
    }                                                            \
    return !_mm_feof(fp);                                        \
}

DEFINE_MULTIPLE_READ_FUNCTION_NORM   (SBYTE, SBYTE)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (UBYTE, UBYTE)

#ifdef _MSC_VER
#pragma optimize( "g", off )
#endif

#ifdef MM_BIG_ENDIAN
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_SWORD, SWORD)
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_UWORD, UWORD)
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_SLONG, SLONG)
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_ULONG, ULONG)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_SWORD, SWORD)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_UWORD, UWORD)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_SLONG, SLONG)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_ULONG, ULONG)
#else
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_SWORD, SWORD)
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_UWORD, UWORD)
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_SLONG, SLONG)
DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_ULONG, ULONG)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_SWORD, SWORD)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_UWORD, UWORD)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_SLONG, SLONG)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_ULONG, ULONG)
#endif

