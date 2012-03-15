#ifndef _MM_INLINE_H_
#define _MM_INLINE_H_

#include "mmio.h"
#include <string.h>

static long __inline _mm_timeoverflow(ulong newtime, ulong oldtime)
{
    if(newtime < oldtime) return (0xfffffffful - oldtime) + newtime;
    return newtime - oldtime;       // return timediff, with overflow checks!
}

// =====================================================================================
//                               Inline Memcpy Functions
// =====================================================================================
// These are mostly obsolete since the discovery that Visual Studio's memcpy is one heck
// of a fast mother.  I leave them here, however, and still continue to use them, in
// the event that I ever happen to implement some sort of MMX or FPU-accelerated memcpy.

static void __inline _mminline_memcpy_long(void *dst, const void *src, int count)
{
    // ** note to self : compiler memcpy is faster than mine!
    //register ULONG *dest = (ULONG *)dst, *srce = (ULONG *)src, i;
    //for(i=count; i; i--, dest++, srce++) *dest = *srce;
    memcpy(dst, src, count*4);
}


static void __inline _mminline_memcpy_word(void *dst, const void *src, int count)
{
    memcpy(dst, src, count*2);
}


static void __inline _mminline_memcpy_quad(void *dst, const void *src, int count)
{
    register ULONG *dest = (ULONG *)dst, *srce = (ULONG *)src, i;
    for(i=count*2; i; i--, dest++, srce++) *dest = *srce;
}


// =====================================================================================
//                            THE OUTPUT (WRITE) SECTION
// =====================================================================================


#ifndef MM_FAST_FILEIO
#define filewrite_SBYTE(x,y)      (y)->fputc((int)x,(y)->fp)
#define filewrite_UBYTE(x,y)      (y)->fputc((int)x,(y)->fp)
#else
#define filewrite_SBYTE(x,y)      fputc((int)x,(y)->fp)
#define filewrite_UBYTE(x,y)      fputc((int)x,(y)->fp)
#endif
#define datawrite_SBYTE(x,y)      ((y)->dp[(y)->seekpos++] = (UBYTE)x)
#define datawrite_UBYTE(x,y)      ((y)->dp[(y)->seekpos++] = x)

#define filewrite_I_SWORD(x,fp) filewrite_SBYTE(x,fp), filewrite_UBYTE(x>>8,fp)
#define filewrite_I_UWORD(x,fp) filewrite_SBYTE(x,fp), filewrite_UBYTE(x>>8,fp)
#define datawrite_I_SWORD(x,fp) datawrite_SBYTE(x,fp), datawrite_UBYTE(x>>8,fp)
#define datawrite_I_UWORD(x,fp) datawrite_SBYTE(x,fp), datawrite_UBYTE(x>>8,fp)
#define filewrite_I_SLONG(x,fp) filewrite_I_SWORD(x,fp), filewrite_I_UWORD(x>>16,fp)
#define filewrite_I_ULONG(x,fp) filewrite_I_UWORD(x,fp), filewrite_I_UWORD(x>>16,fp)
#define datawrite_I_SLONG(x,fp) datawrite_I_SWORD(x,fp), datawrite_I_UWORD(x>>16,fp)
#define datawrite_I_ULONG(x,fp) datawrite_I_UWORD(x,fp), datawrite_I_UWORD(x>>16,fp)
#define filewrite_M_SWORD(x,fp) filewrite_SBYTE(x>>8,fp), filewrite_UBYTE(x,fp)
#define filewrite_M_UWORD(x,fp) filewrite_UBYTE(x>>8,fp), filewrite_UBYTE(x,fp)
#define datawrite_M_SWORD(x,fp) datawrite_SBYTE(x>>8,fp), datawrite_UBYTE(x,fp)
#define datawrite_M_UWORD(x,fp) datawrite_UBYTE(x>>8,fp), datawrite_UBYTE(x,fp)
#define filewrite_M_SLONG(x,fp) filewrite_M_SWORD(x>>16,fp), filewrite_M_UWORD(x,fp)
#define filewrite_M_ULONG(x,fp) filewrite_M_UWORD(x>>16,fp), filewrite_M_UWORD(x,fp)
#define datawrite_M_SLONG(x,fp) datawrite_M_SWORD(x>>16,fp), datawrite_M_UWORD(x,fp)
#define datawrite_M_ULONG(x,fp) datawrite_M_UWORD(x>>16,fp), datawrite_M_UWORD(x,fp)

#ifndef MM_FAST_FILEIO
#define filewrite_SBYTES(x,y,z)  (z)->fwrite((void *)x,1,y,(z)->fp)
#define filewrite_UBYTES(x,y,z)  (z)->fwrite((void *)x,1,y,(z)->fp)
#else
#define filewrite_SBYTES(x,y,z)  fwrite((void *)x,1,y,(z)->fp)
#define filewrite_UBYTES(x,y,z)  fwrite((void *)x,1,y,(z)->fp)
#endif
#define datawrite_SBYTES(x,y,z)  (memcpy(&(z)->dp[(z)->seekpos],(void *)x,y), (z)->seekpos += y)
#define datawrite_UBYTES(x,y,z)  (memcpy(&(z)->dp[(z)->seekpos],(void *)x,y), (z)->seekpos += y)

#ifdef MM_BIG_ENDIAN
#ifndef MM_FAST_FILEIO
#define filewrite_M_SWORDS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define filewrite_M_UWORDS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define filewrite_M_SLONGS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define filewrite_M_ULONGS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#else
#define filewrite_M_SWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define filewrite_M_UWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define filewrite_M_SLONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define filewrite_M_ULONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#endif
#define datawrite_M_SWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#define datawrite_M_UWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#define datawrite_M_SLONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#define datawrite_M_ULONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#else
#ifndef MM_FAST_FILEIO
#define filewrite_I_SWORDS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define filewrite_I_UWORDS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define filewrite_I_SLONGS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define filewrite_I_ULONGS(x,y,z) (z)->fwrite((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#else
#define filewrite_I_SWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define filewrite_I_UWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define filewrite_I_SLONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define filewrite_I_ULONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#endif
#define datawrite_I_SWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#define datawrite_I_UWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(UWORD))
#define datawrite_I_SLONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#define datawrite_I_ULONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#endif


// =====================================================================================
//                              THE INPUT (READ) SECTION
// =====================================================================================

#ifndef MM_FAST_FILEIO
#define fileread_SBYTE(x)         (SBYTE)(x)->fgetc((x)->fp)
#define fileread_UBYTE(x)         (UBYTE)(x)->fgetc((x)->fp)
#else
#define fileread_SBYTE(x)         (SBYTE)fgetc((x)->fp)
#define fileread_UBYTE(x)         (UBYTE)fgetc((x)->fp)
#endif

#define dataread_SBYTE(y)         (y)->dp[(y)->seekpos++]
#define dataread_UBYTE(y)         (y)->dp[(y)->seekpos++]

#define fileread_I_SWORD(fp) ((SWORD)(fileread_UBYTE(fp) | (fileread_UBYTE(fp)<<8)))
#define fileread_I_UWORD(fp) ((UWORD)(fileread_UBYTE(fp) | (fileread_UBYTE(fp)<<8)))
#define dataread_I_SWORD(fp) ((SWORD)(dataread_UBYTE(fp) | (dataread_UBYTE(fp)<<8)))
#define dataread_I_UWORD(fp) ((UWORD)(dataread_UBYTE(fp) | (dataread_UBYTE(fp)<<8)))
#define fileread_I_SLONG(fp) ((SLONG)(fileread_I_UWORD(fp) | (fileread_I_UWORD(fp)<<16)))
#define fileread_I_ULONG(fp) ((ULONG)(fileread_I_UWORD(fp) | (fileread_I_UWORD(fp)<<16)))
#define dataread_I_SLONG(fp) ((SLONG)(dataread_I_UWORD(fp) | (dataread_I_UWORD(fp)<<16)))
#define dataread_I_ULONG(fp) ((ULONG)(dataread_I_UWORD(fp) | (dataread_I_UWORD(fp)<<16)))
#define fileread_M_SWORD(fp) ((SWORD)((fileread_UBYTE(fp)<<8) | fileread_UBYTE(fp)))
#define fileread_M_UWORD(fp) ((UWORD)((fileread_UBYTE(fp)<<8) | fileread_UBYTE(fp)))
#define dataread_M_SWORD(fp) ((SWORD)((dataread_UBYTE(fp)<<8) | dataread_UBYTE(fp)))
#define dataread_M_UWORD(fp) ((UWORD)((dataread_UBYTE(fp)<<8) | dataread_UBYTE(fp)))
#define fileread_M_SLONG(fp) ((SLONG)((fileread_M_UWORD(fp)<<16) | fileread_M_UWORD(fp)))
#define fileread_M_ULONG(fp) ((ULONG)((fileread_M_UWORD(fp)<<16) | fileread_M_UWORD(fp)))
#define dataread_M_SLONG(fp) ((SLONG)((dataread_M_UWORD(fp)<<16) | dataread_M_UWORD(fp)))
#define dataread_M_ULONG(fp) ((ULONG)((dataread_M_UWORD(fp)<<16) | dataread_M_UWORD(fp)))


#ifndef MM_FAST_FILEIO
#define fileread_SBYTES(x,y,z)  (z)->fread((void *)x,1,y,(z)->fp)
#define fileread_UBYTES(x,y,z)  (z)->fread((void *)x,1,y,(z)->fp)
#else
#define fileread_SBYTES(x,y,z)  fread((void *)x,1,y,(z)->fp)
#define fileread_UBYTES(x,y,z)  fread((void *)x,1,y,(z)->fp)
#endif
#define dataread_SBYTES(x,y,z)  memcpy((void *)x,&(z)->dp[(z)->seekpos],y)
#define dataread_UBYTES(x,y,z)  memcpy((void *)x,&(z)->dp[(z)->seekpos],y)

#ifdef MM_BIG_ENDIAN
#ifndef MM_FAST_FILEIO
#define fileread_M_SWORDS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define fileread_M_UWORDS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define fileread_M_SLONGS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define fileread_M_ULONGS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#else
#define fileread_M_SWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define fileread_M_UWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define fileread_M_SLONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define fileread_M_ULONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#endif
#define dataread_M_SWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#define dataread_M_UWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(UWORD))
#define dataread_M_SLONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#define dataread_M_ULONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#else
#ifndef MM_FAST_FILEIO
#define fileread_I_SWORDS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define fileread_I_UWORDS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define fileread_I_SLONGS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define fileread_I_ULONGS(x,y,z)  (z)->fread((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#else
#define fileread_I_SWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#define fileread_I_UWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#define fileread_I_SLONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#define fileread_I_ULONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#endif
#define dataread_I_SWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#define dataread_I_UWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(UWORD))
#define dataread_I_SLONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#define dataread_I_ULONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#endif

#endif
