#ifndef _MMIO_H_
#define _MMIO_H_

// ---------------------------
// MM_FAST_FILEIO
//  Define this compiler macro to remove the redirectional layer of mmio's file io
//  system.  Generally speaking, most users would never need to use the redirectional
//  features-- unless you need to map mmio through a wadfile api or something.

#define MM_FAST_FILEIO
// ---------------------------
// MMALLOC_NO_FUNCPTR
//  Removes function pointers from the mmalloc block-- disabling user-remappable memory
//  allocation behavior,but resulting in reduced memory footprint and reduced execution
//  times.

#define MMALLOC_NO_FUNCPTR
// ---------------------------

#ifdef WIN32
#ifndef MMEXPORT
#ifdef MM_DLL_EXPORT
#define MMEXPORT  extern __declspec( dllexport )
#elif MIKMOD_STATIC
#define MMEXPORT  extern
#else
#define MMEXPORT  extern __declspec( dllimport )
#endif
#endif
#else
#define MMEXPORT extern
#endif

// #ifndef __VECTORC
#define restrict
#define alignvalue(a)
#define __hint__(a)
#define div12 /
#define sqrt12 sqrt

#define align(a) __declspec (align (a))

#define _mm_exchange(x, y, type)  \
{ \
    type  __tmp = x; \
    x = y; \
    y = __tmp; \
} \


#include <stdio.h>
#include <stdlib.h>
#include "mmtypes.h"

// CPU mode options.  These will force the use of the selected mode, whether that
// cpu is available or not, so make sure you check first!  Speed enhancments are
// present in sample loading and software mixing.

#define CPU_NONE              0
#define CPU_MMX         (1ul<<0)
#define CPU_3DNOW       (1ul<<1)
#define CPU_SIMD        (1ul<<2)
#define CPU_AUTODETECT    (0xff)

MMEXPORT uint _mm_cpudetect(void);

MMEXPORT void _mm_memcpy_quad(void *dst, const void *src, int count);
MMEXPORT void _mm_memcpy_long(void *dst, const void *src, int count);
MMEXPORT void _mm_memcpy_word(void *dst, const void *src, int count);


// Miscellaneous Macros 
// --------------------
// I should probably put these somewhere else.

// boundschecker macro.  I do a lot of bounds checking ;)

#define _mm_boundscheck(v,a,b)  ((v > b) ? b : ((v < a) ? a : v))

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif


// 64 bit integer macros.  These allow me to get pointers to the high
// and low portions of a 64 bit integer.  Used mostly by the Mikmod
// software mixer - but could be useful for other things.

#ifdef MM_BIG_ENDIAN

#define _mm_HI_SLONG(x) ((SLONG *)&x)
#define _mm_LO_SLONG(x) ((SLONG *)&x + 1)
#define _mm_HI_ULONG(x) ((ULONG *)&x)
#define _mm_LO_ULONG(x) ((ULONG *)&x + 1)

#else

#define _mm_HI_SLONG(x) ((SLONG *)&x + 1)
#define _mm_LO_SLONG(x) ((SLONG *)&x)
#define _mm_HI_ULONG(x) ((ULONG *)&x + 1)
#define _mm_LO_ULONG(x) ((ULONG *)&x)

#endif


#ifdef __cplusplus
extern "C" {
#endif


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
{   MMERR_NONE = 0,
    MMERR_INVALID_PARAMS,
    MMERR_OPENING_FILE,
    MMERR_UNSUPPORTED_FILE,
    MMERR_OUT_OF_MEMORY,
    MMERR_END_OF_FILE,
    MMERR_DISK_FULL,
    MMERR_DETECTING_DEVICE,
    MMERR_INITIALIZING_DEVICE,
    MMERR_FATAL
};

// Error / Logging prototypes
// --------------------------

MMEXPORT void   _mmlog_init(void (__cdecl *log)(const CHAR *fmt, ... ), void (__cdecl *logv)(const CHAR *fmt, ... ));
MMEXPORT void   _mmlog_exit(void);

MMEXPORT void   (__cdecl *_mmlog)(const CHAR *fmt, ... );
MMEXPORT void   (__cdecl *_mmlogv)(const CHAR *fmt, ... );

MMEXPORT void   _mmerr_sethandler(void (*func)(int, const CHAR *));
MMEXPORT void   _mmerr_set(int err_is_human, const CHAR *human_is_err);
MMEXPORT void   _mmerr_setsub(int err_is_human, const CHAR *human_is_err);
MMEXPORT int    _mmerr_getinteger(void);
MMEXPORT CHAR  *_mmerr_getstring(void);
MMEXPORT void   _mmerr_handler(void);


// =======================================
//  Input / Output and Streaming - MMIO.C
// =======================================

// =====================================================================================
    typedef struct MMSTREAM
// =====================================================================================
// Mikmod customized module file pointer structure.
// Contains various data that may be required for a module.
{   
    FILE   *fp;      // File pointer
    UBYTE  *dp;

    long   iobase;       // base seek position within the file
    long   seekpos;      // used in data(mem) streaming mode only.

#ifndef MM_FAST_FILEIO
    int (__cdecl *fread)(void *buffer, size_t size, size_t count, FILE *stream);
    int (__cdecl *fwrite)(const void *buffer, size_t size, size_t count, FILE *stream);

    int (__cdecl *fgetc)(FILE *stream);
    int (__cdecl *fputc)(int c, FILE *stream);

    int (__cdecl *fseek)(FILE *stream, long offset, int origin);
    int (__cdecl *ftell)(FILE *stream);

    int (__cdecl *grrr)(FILE *stream);
#endif
} MMSTREAM;

MMEXPORT MMSTREAM *_mmstream_createfp(FILE *fp, int iobase);
MMEXPORT MMSTREAM *_mmstream_createmem(void *data, int iobase);
MMEXPORT void      _mmstream_delete(MMSTREAM *mmf);

#ifndef MM_FAST_FILEIO
MMEXPORT void      _mmstream_setapi(MMSTREAM *mmfp,
           int (__cdecl *fread)(void *buffer, size_t size, size_t count, FILE *stream),
           int (__cdecl *fwrite)(const void *buffer, size_t size, size_t count, FILE *stream),
           int (__cdecl *fgetc)(FILE *stream),
           int (__cdecl *fputc)(int c, FILE *stream),
           int (__cdecl *fseek)(FILE *stream, long offset, int origin),
           int (__cdecl *ftell)(FILE *stream),
           int (__cdecl *feof)(FILE *stream));
#endif

// ===================================================
//  Memory allocation with error handling - MMALLOC.C
// ===================================================

// Block Types In-Depth
// --------------------
// By default, a global blocktype is created which is the default block used when the
// user called _mm_malloc
//

// =====================================================================================
    typedef struct MM_BLOCKINFO
// =====================================================================================
{
    uint            size;                  // size of the memory allocation, in dwords
    ULONG          *block;
    void          (*deallocate)(void *block);

    int            next, prev;             // you know what a linked list is
                                           // (this one is based on indexes, not pointers)
} MM_BLOCKINFO;


// =====================================================================================
    typedef struct MM_ALLOC
// =====================================================================================
// An allocation handle for use with the mmio allocation functions.
{
#ifdef _DEBUG
    BOOL               checkplease;
#endif
    int                blocklist;
    uint               ident;            // blocktype identifier!

    struct MM_ALLOC   *next, *prev;      // you know what a linked list is.
    struct MM_ALLOC   *children, *parent;

    BOOL               killswitch;
    BOOL               processing;       // Set TRUE if we're in list-processing callback state

    CHAR               name[64];         // every allocation type has a name.  wowie zowie!
    uint               size;             // size of allochandle in dwords
    void             (*shutdown)(void *blockdata);
    void              *shutdata;

#ifndef MMALLOC_NO_FUNCPTR
    void *(*fptr_malloc)    (size_t size);
    void *(*fptr_calloc)    (size_t nitems, size_t size);
    void *(*fptr_realloc)   (void *old_blk, size_t size);
    void *(*fptr_recalloc)  (void *old_blk, size_t nitems, size_t size);
    void *(*fptr_free)      (void *d);
#endif
} MM_ALLOC;

#define _mmalloc_callback_enter(t) \
    BOOL  _mma_processing = (t)->processing; \
    (t)->processing = TRUE;

#define _mmalloc_callback_exit(t)  { (t)->processing = _mma_processing; if((t)->killswitch) _mmalloc_cleanup(t); }
#define _mmalloc_callback_next(t) \
{   MM_ALLOC *_ttmp321 = (t)->next; \
    _mmalloc_callback_exit(t); \
    (t) = _ttmp321;  } \


#ifdef _DEBUG
#define _mmalloc_check_set(a)    (a)->checkplease = TRUE
#define _mmalloc_check_clear(a)  (a)->checkplease = FALSE
#else
#define _mmalloc_check_set(a)
#define _mmalloc_check_clear(a)
#endif

#define MMBLK_IDENT_ALLOCHANDLE      0
#define MMBLK_IDENT_USER         0x100

MMEXPORT MM_ALLOC *_mmalloc_create(const CHAR *name, MM_ALLOC *parent);
MMEXPORT MM_ALLOC *_mmalloc_create_ex(const CHAR *name, MM_ALLOC *parent, uint size);
MMEXPORT void      _mmalloc_freeall(MM_ALLOC *type);
MMEXPORT void      _mmalloc_close(MM_ALLOC *type);
MMEXPORT void      _mmalloc_closeall(void);
MMEXPORT void      _mmalloc_passive_close(MM_ALLOC *type);
MMEXPORT BOOL      _mmalloc_cleanup(MM_ALLOC *type);

MMEXPORT void      _mmalloc_setshutdown(MM_ALLOC *type, void (*shutdown)(void *block), void *block);
MMEXPORT void      _mmalloc_report(MM_ALLOC *type);

#ifdef _DEBUG
MMEXPORT void      _mm_checkallblocks(MM_ALLOC *type);
MMEXPORT void      _mm_allocreportall(void);
#else
#define _mm_checkallblocks(x)
#endif

MMEXPORT void     *_mm_mallocx(MM_ALLOC *type, size_t size, void (*deinitialize)(void *block));
MMEXPORT void     *_mm_callocx(MM_ALLOC *type, size_t nitems, size_t size, void (*deinitialize)(void *block));
MMEXPORT void     *_mm_reallocx(MM_ALLOC *type, void *old_blk, size_t size, void (*deinitialize)(void *block));
MMEXPORT void     *_mm_recallocx(MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deinitialize)(void *block));
MMEXPORT void      _mm_a_free(MM_ALLOC *type, void **d);

#define _mm_malloc(t,s)         _mm_mallocx(t,s, NULL)
#define _mm_calloc(t,n,s)       _mm_callocx(t,n,s, NULL)
#define _mm_realloc(t,o,s)      _mm_reallocx(t,o,s, NULL)
#define _mm_recalloc(t,o,n,s)   _mm_recallocx(t,o,n,s, NULL)

// Adding a cast to (void **) gets rid of a few dozen warnings.
#define _mm_free(b,a)   _mm_a_free(b,(void **)&(a))

#ifdef MM_LOG_VERBOSE

//#define _mm_free(b,a,t)   _mm_b_free(b,&a,t)
#define _mmlogd         _mmlog
#define _mmlogd1        _mmlog
#define _mmlogd2        _mmlog
#define _mmlogd3        _mmlog
#define _mmlogd4        _mmlog
#else

//#define _mm_free(b,a,t)   _mm_a_free(b,&a)

#define _mmlogd(a)           __mmlogd()
#define _mmlogd1(a,b)        __mmlogd()
#define _mmlogd2(a,b,c)      __mmlogd()
#define _mmlogd3(a,b,c,d)    __mmlogd()
#define _mmlogd4(a,b,c,d,e)  __mmlogd()

static void __inline __mmlogd(void)
{
}

#endif

MMEXPORT void _mm_RegisterErrorHandler(void (*proc)(int, CHAR *));
MMEXPORT BOOL _mm_fexist(CHAR *fname);

MMEXPORT void StringWrite(const CHAR *s, MMSTREAM *fp);
MMEXPORT CHAR *StringRead(MMSTREAM *fp);


//  MikMod/DivEnt style file input / output -
//    Solves several portability issues.
//    Notably little vs. big endian machine complications.

#define _mm_rewind(x) _mm_fseek(x,0,SEEK_SET)

MMEXPORT int      _mm_fseek(MMSTREAM *stream, long offset, int whence);
MMEXPORT long     _mm_ftell(MMSTREAM *stream);
MMEXPORT BOOL     _mm_feof(MMSTREAM *stream);
MMEXPORT MMSTREAM *_mm_fopen(const CHAR *fname, const CHAR *attrib);
MMEXPORT void     _mm_fclose(MMSTREAM *mmfile);
MMEXPORT MMSTREAM *_mm_tmpfile(void);

MMEXPORT void     _mm_fputs(MMSTREAM *fp, const CHAR *data);
MMEXPORT void     _mm_write_string(const CHAR *data, MMSTREAM *fp);
MMEXPORT int      _mm_read_string (CHAR *buffer, int number, MMSTREAM *fp);

MMEXPORT long     _mm_flength(FILE *stream);
MMEXPORT BOOL     _mm_copyfile(FILE *fpi, FILE *fpo, uint len);


MMEXPORT SBYTE _mm_read_SBYTE (MMSTREAM *fp);
MMEXPORT UBYTE _mm_read_UBYTE (MMSTREAM *fp);

MMEXPORT SWORD _mm_read_M_SWORD (MMSTREAM *fp);
MMEXPORT SWORD _mm_read_I_SWORD (MMSTREAM *fp);

MMEXPORT UWORD _mm_read_M_UWORD (MMSTREAM *fp);
MMEXPORT UWORD _mm_read_I_UWORD (MMSTREAM *fp);

MMEXPORT SLONG _mm_read_M_SLONG (MMSTREAM *fp);
MMEXPORT SLONG _mm_read_I_SLONG (MMSTREAM *fp);

MMEXPORT ULONG _mm_read_M_ULONG (MMSTREAM *fp);
MMEXPORT ULONG _mm_read_I_ULONG (MMSTREAM *fp);


MMEXPORT int _mm_read_SBYTES    (SBYTE *buffer, int number, MMSTREAM *fp);
MMEXPORT int _mm_read_UBYTES    (UBYTE *buffer, int number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_SWORDS  (SWORD *buffer, int number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_SWORDS  (SWORD *buffer, int number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_UWORDS  (UWORD *buffer, int number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_UWORDS  (UWORD *buffer, int number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_SLONGS  (SLONG *buffer, int number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_SLONGS  (SLONG *buffer, int number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_ULONGS  (ULONG *buffer, int number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_ULONGS  (ULONG *buffer, int number, MMSTREAM *fp);


MMEXPORT void _mm_write_SBYTE     (SBYTE data, MMSTREAM *fp);
MMEXPORT void _mm_write_UBYTE     (UBYTE data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SWORD   (SWORD data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SWORD   (SWORD data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_UWORD   (UWORD data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_UWORD   (UWORD data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SLONG   (SLONG data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SLONG   (SLONG data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_ULONG   (ULONG data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_ULONG   (ULONG data, MMSTREAM *fp);

MMEXPORT void _mm_write_SBYTES    (const SBYTE *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_UBYTES    (const UBYTE *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SWORDS  (const SWORD *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SWORDS  (const SWORD *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_UWORDS  (const UWORD *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_UWORDS  (const UWORD *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SLONGS  (const SLONG *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SLONGS  (const SLONG *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_ULONGS  (const ULONG *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_ULONGS  (const ULONG *data, int number, MMSTREAM *fp);

#ifdef __WATCOMC__
#pragma aux _mm_fseek      parm nomemory modify nomemory
#pragma aux _mm_ftell      parm nomemory modify nomemory
#pragma aux _mm_flength    parm nomemory modify nomemory
#pragma aux _mm_fopen      parm nomemory modify nomemory
#pragma aux _mm_fputs      parm nomemory modify nomemory
#pragma aux _mm_copyfile   parm nomemory modify nomemory
#pragma aux _mm_iobase_get parm nomemory modify nomemory
#pragma aux _mm_iobase_set parm nomemory modify nomemory
#pragma aux _mm_iobase_setcur parm nomemory modify nomemory
#pragma aux _mm_iobase_revert parm nomemory modify nomemory
#pragma aux _mm_write_string  parm nomemory modify nomemory
#pragma aux _mm_read_string   parm nomemory modify nomemory

#pragma aux _mm_read_M_SWORD parm nomemory modify nomemory; 
#pragma aux _mm_read_I_SWORD parm nomemory modify nomemory; 
#pragma aux _mm_read_M_UWORD parm nomemory modify nomemory; 
#pragma aux _mm_read_I_UWORD parm nomemory modify nomemory; 
#pragma aux _mm_read_M_SLONG parm nomemory modify nomemory; 
#pragma aux _mm_read_I_SLONG parm nomemory modify nomemory; 
#pragma aux _mm_read_M_ULONG parm nomemory modify nomemory; 
#pragma aux _mm_read_I_ULONG parm nomemory modify nomemory; 

#pragma aux _mm_read_M_SWORDS parm nomemory modify nomemory; 
#pragma aux _mm_read_I_SWORDS parm nomemory modify nomemory; 
#pragma aux _mm_read_M_UWORDS parm nomemory modify nomemory; 
#pragma aux _mm_read_I_UWORDS parm nomemory modify nomemory; 
#pragma aux _mm_read_M_SLONGS parm nomemory modify nomemory; 
#pragma aux _mm_read_I_SLONGS parm nomemory modify nomemory; 
#pragma aux _mm_read_M_ULONGS parm nomemory modify nomemory; 
#pragma aux _mm_read_I_ULONGS parm nomemory modify nomemory; 

#pragma aux _mm_write_M_SWORD parm nomemory modify nomemory; 
#pragma aux _mm_write_I_SWORD parm nomemory modify nomemory; 
#pragma aux _mm_write_M_UWORD parm nomemory modify nomemory; 
#pragma aux _mm_write_I_UWORD parm nomemory modify nomemory; 
#pragma aux _mm_write_M_SLONG parm nomemory modify nomemory; 
#pragma aux _mm_write_I_SLONG parm nomemory modify nomemory; 
#pragma aux _mm_write_M_ULONG parm nomemory modify nomemory; 
#pragma aux _mm_write_I_ULONG parm nomemory modify nomemory; 

#pragma aux _mm_write_M_SWORDS parm nomemory modify nomemory; 
#pragma aux _mm_write_I_SWORDS parm nomemory modify nomemory; 
#pragma aux _mm_write_M_UWORDS parm nomemory modify nomemory; 
#pragma aux _mm_write_I_UWORDS parm nomemory modify nomemory; 
#pragma aux _mm_write_M_SLONGS parm nomemory modify nomemory; 
#pragma aux _mm_write_I_SLONGS parm nomemory modify nomemory; 
#pragma aux _mm_write_M_ULONGS parm nomemory modify nomemory; 
#pragma aux _mm_write_I_ULONGS parm nomemory modify nomemory; 
#endif

// Defined in mmcopy.c
// -------------------

MMEXPORT CHAR *_mm_strcat(CHAR *dest, const CHAR *src, uint maxlen);
MMEXPORT CHAR *_mm_strdup(MM_ALLOC *allochandle, const CHAR *src);
MMEXPORT CHAR *_mm_strndup(MM_ALLOC *allochandle, const CHAR *src, uint length);
MMEXPORT void *_mm_structdup(MM_ALLOC *allochandle, const void *src, size_t size);
MMEXPORT void _mm_insertchr(CHAR *dest, CHAR src, uint pos);
MMEXPORT void _mm_deletechr(CHAR *dest, uint pos);
MMEXPORT void _mm_insertstr(CHAR *dest, const CHAR *src, uint pos);


#ifdef __cplusplus
};
#endif

#endif



