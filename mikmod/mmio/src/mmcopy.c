/*
 Mikmod Portable Memory Allocation

 By Jake Stine of Divine Entertainment (1996-2000) and
    Jean-Paul Mikkers (1993-1996).

 File: mmcopy.c

 Description:

*/

#include "mmio.h"
#include <string.h>

void _mm_memcpy_long(void *dst, const void *src, int count)
{
    register ULONG *dest = (ULONG *)dst, *srce = (ULONG *)src, i;
    for(i=count; i; i--, dest++, srce++) *dest = *srce;
}

void _mm_memcpy_word(void *dst, const void *src, int count)
{
    memcpy(dst, src, count*2);
}

void _mm_memcpy_quad(void *dst, const void *src, int count)
{
    register ULONG *dest = (ULONG *)dst, *srce = (ULONG *)src, i;
    for(i=count*2; i; i--, dest++, srce++) *dest = *srce;
}


// ======================================================================================
    CHAR *_mm_strdup(MM_ALLOC *allochandle, const CHAR *src)
// ======================================================================================
// Same as Watcom's strdup function - allocates memory for a string and makes a copy of
// it.  Returns NULL if failed.
{
    CHAR *buf;

    if(!src) return NULL;
    if((buf = (CHAR *)_mm_malloc(allochandle, strlen(src)+1)) == NULL) return NULL;

    strcpy(buf,src);
    return buf;
}


// =====================================================================================
    CHAR *_mm_strcat(CHAR *dest, const CHAR *src, uint maxlen)
// =====================================================================================
// concatenates the source string onto the dest, ensuring the dest never goes above
// maxlen characters!  This is effectively a more useful version of strncat.  USe it to
// prevent buffer overflows and such.
{
    uint   safelen, slen;
    uint   riplen;

    riplen  = strlen(src);
    slen    = strlen(dest);
    safelen = (maxlen < (slen + riplen)) ? (maxlen-slen) : (riplen+1);
    strncpy(&dest[slen], src, safelen);
    dest[maxlen-1] = 0;                   // strncpy not garunteed to do this

    return dest;
}


// ======================================================================================
    CHAR *_mm_strndup(MM_ALLOC *allochandle, const CHAR *src, uint length)
// ======================================================================================
// Same as Watcom's strdup function - allocates memory for a string and makes a copy of
// it-- limiting the destinaion string length to 'length'.  Returns NULL if failed.
{
    CHAR   *buf;
    uint    stl;

    if(!src || !length) return NULL;

    stl = strlen(src);
    if(stl < length) length = stl;
    if((buf = (CHAR *)_mm_malloc(allochandle, length+1)) == NULL) return NULL;

    memcpy(buf, src, sizeof(CHAR) * length);
    buf[length] = 0;

    return buf;
}


// ======================================================================================
    void *_mm_structdup(MM_ALLOC *allochandle, const void *src, size_t size)
// ======================================================================================
{
    void *tribaldance;
    
    if(!src) return NULL;
    
    tribaldance = _mm_malloc(allochandle, size);
    if(tribaldance) memcpy(tribaldance, src, size);

    return tribaldance;
}


// =====================================================================================
    void _mm_insertchr(CHAR *dest, CHAR src, uint pos)
// =====================================================================================
// inserts the specified source character into the destination string at position.  It
// is the responsibility of the caller to ensure the string has enough room to contain
// the resultant string!
{
    if(dest)
    {   const uint dlen = strlen(dest);
        if(pos >= dlen)
        {
            // Append Mode
            // -----------
            // Just attach the character to the end of the string.

            dest[dlen]   = src;
            dest[dlen+1] = 0;
        } else
        {
            // Insert Mode
            // -----------
            // copy contents of string, in reverse, so we don't overwrite our
            // source data!

            uint  i;
            for(i=dlen+1; i>pos; i--)
                dest[i] = dest[i-1];

            dest[pos] = src;
        }
    }
}


// =====================================================================================
    void _mm_deletechr(CHAR *dest, uint pos)
// =====================================================================================
{
    if(dest)
    {   const uint dlen = strlen(dest);
        if(pos < dlen)
            memcpy(&dest[pos], &dest[pos+1], strlen(&dest[pos+1])+1);
    }
}


// =====================================================================================
    void _mm_insertstr(CHAR *dest, const CHAR *src, uint pos)
// =====================================================================================
// inserts the specified source string into the destination string at position.  It
// is the responsibility of the caller to ensure the string has enough room to contain
// the resultant string!
{
    if(dest)
    {   const uint dlen = strlen(dest);
        if(pos >= dlen)
        {
            // Append Mode
            // -----------
            // Just attach the source to the end of the string.

            strcat(dest, src);
        } else
        {
            // Insert Mode
            // -----------
            // copy contents of string, in reverse, so we don't overwrite our
            // source data!

            uint  i, srclen = strlen(src);
            for(i=dlen; i>pos; i--)
                dest[i] = dest[i-srclen];

            memcpy(&dest[pos], src, sizeof(CHAR) * srclen);
        }
    }
}
