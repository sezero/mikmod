/*
 Mikmod Portable Memory Allocation

 By Jake Stine of Hour 13 Studios (1996-2002) and
    Jean-Paul Mikkers (1993-1996).

 File: mmcopy.c

 Description:

*/

#include "mmio.h"
#include <string.h>

void _mm_memcpy_long(void *dst, const void *src, const int count)
{
    register ULONG *dest = (ULONG *)dst, *srce = (ULONG *)src, i;
    for(i=count; i; i--, dest++, srce++) *dest = *srce;
}

void _mm_memcpy_word(void *dst, const void *src, const int count)
{
    memcpy(dst, src, count*2);
}

void _mm_memcpy_quad(void *dst, const void *src, const int count)
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
    if((buf = (CHAR *)_mm_malloc(allochandle, (strlen(src)+1) * sizeof(CHAR))) == NULL) return NULL;

    strcpy(buf,src);
    return buf;
}


// =====================================================================================
    CHAR *_mm_strcpy(CHAR *dest, const CHAR *src, const uint maxlen)
// =====================================================================================
// copies the source string onto the dest, ensuring the dest never goes above maxlen
// characters!  This is effectively a more useful version of strncpy.  Use it to
// prevent buffer overflows and such.
{
    if(dest && src)
    {
        strncpy(dest, src, maxlen-1);
        dest[maxlen-1] = 0;                   // strncpy not garunteed to do this
    }

    return dest;
}


// =====================================================================================
    CHAR *_mm_strcat(CHAR *dest, const CHAR *src, const uint maxlen)
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
    void *_mm_structdup(MM_ALLOC *allochandle, const void *src, const size_t size)
// ======================================================================================
{
    void *tribaldance;
    
    if(!src) return NULL;

    tribaldance = _mm_malloc(allochandle, size);
    if(tribaldance) memcpy(tribaldance, src, size);

    return tribaldance;
}


// =====================================================================================
    void *_mm_objdup(MM_ALLOC *parent, const void *src, const uint blksize)
// =====================================================================================
{
    void  *dest = _mmalloc_create_ex("Sumkinda Duplicate", parent, blksize);
   
    {
        UBYTE        *stupid = (UBYTE *)dest;
        const UBYTE  *does   = (UBYTE *)src;

        memcpy(&stupid[sizeof(MM_ALLOC)], &does[sizeof(MM_ALLOC)], blksize - sizeof(MM_ALLOC));
    }
    return dest;
}


// =====================================================================================
    void _mm_insertchr(CHAR *dest, const CHAR src, const uint pos)
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
    void _mm_insertstr(CHAR *dest, const CHAR *src, const uint pos)
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


// =====================================================================================
    void _mm_splitpath(const CHAR *fullpath, CHAR *path, CHAR *fname)
// =====================================================================================
{
    if(fullpath)
    {
        int    i;
        const   flen = strlen(fullpath);

        if(path) strcpy(path, fullpath);

        for(i=flen-1; i>=0; i--)
            if(fullpath[i] == '\\') break;

        if(fname) strcpy(fname, &fullpath[i+1]);
        if(path) path[i+1] = 0;
    }
}

const static CHAR  invalidchars[] = "'|()*%?/\\\"";

// =====================================================================================
    CHAR *_mmstr_filename(CHAR *src)
// =====================================================================================
// Takes the given string and converts it into a valid filename by stripping out any and
// all invalid characters and replacing them with underscores.
{
    if(src)
    {
        uint    idx = 0;
        while(src[idx])
        {
            const CHAR  *ic  = invalidchars;
            while(*ic)
            {   if(src[idx] == *ic)
                {   src[idx] = '_';
                    break;
                }
            }
            idx++;
        }
    }
    return src;
}


// =====================================================================================
    CHAR *_mmstr_pathcat(CHAR *dest, const CHAR *src)
// =====================================================================================
// Concatenates the source path onto the dest, ensuring the dest never goes above
// _MAX_PATH!  If the source path has root characters, then it overwrites the dest
// (proper root behavior)
{
    if(!dest) return NULL;
    if(!src)  return dest;

    // Check for Colon / Backslash
    // ---------------------------
    // A colon, in DOS terms, designates a drive specification, which means we should
    // ignore the root path.  Same goes for a backslash as the first character in the
    // path!

    if((strcspn(src, ":") < strlen(src)) || (src[0] == '\\'))
        strcpy(dest, src);
    else
    {
        if(dest[0])
        {
            // Remove Trailing Backslashes on dest
            // -----------------------------------
            // We'll add our own in later and they mess up some of the logic.

            uint    t   = 0;
            uint    len = strlen(dest);
            len--;
            while(len && (dest[len] == '\\')) len--;

            dest[len+1] = 0;

            // Check for Periods in src
            // ------------------------
            // Periods encapsulated by '\' represent folder nesting: '.' is current (ignored),
            // '..' is previous, etc.

            while(src[t] && (src[t] == '.')) t++;
            if((src[t] == '\\') || (src[t] == 0))
                src  += t;
            else
                t     = 0;

            if(t)
            {
                t--;
                while(t && len)
                {
                    if(dest[len] == '\\') t--;
                    len--;
                }
                dest[len+1] = 0;
            }
        }

        if(dest[0] != 0)
            _mm_strcat(dest, "\\", _MAX_PATH);
        _mm_strcat(dest, src, _MAX_PATH);
    }
    return dest;
}


// =====================================================================================
    CHAR *_mmstr_newext(CHAR *path, const CHAR *ext)
// =====================================================================================
// Attaches a new extension to the specified string, removing the old one if it existed.
{
    uint    i;
    uint    slen = strlen(path);

    if(!path) return NULL;

    for(i=slen-1; i; i--)
        if(path[i] == '.') break;

    i++;
    path[i] = 0;
    if(ext)
        strcpy(&path[i], ext);
    return path;
}


