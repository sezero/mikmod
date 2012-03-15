/*
    MMTYPES.H  : Type definitions for the more commonly used type statements.

    By Jake Stine of Divine Entertainment.
    Dated sometime in the early 21st century!

    This file is used by Mikmod for compiling Mikmod.  However, nothing in this
    file is required to USE Mikmod.  Typedefs required for the use of Mikmod
    (those used in mikmod.h) are actually *IN* mikmod.h.  This is done to keep
    the typedef overhead required to use mikmod to a bare minimum.
*/



#ifndef TDEFS_H
#define TDEFS_H

#ifdef __GNUC__
#ifndef __cdecl
#define __cdecl
#endif
#endif

#ifdef __WATCOMC__
#define   inline
#endif


// THE WARNINGS!  THEY ARE FINALLY GONE!!!
// Visual C, the boon devil of my existence, conceeds defeat with the discovery
// of the #pragma warning command.  Begone, stupid warnings!  I won't even go
// in the details how VC's warning level system is ass-backwards already.  Just
// be glad mikmod no longer streams out thousands of innane warnings.

#ifdef _MSC_VER

// Disable the pointless warning: "(big val) converted to (small val). Possible loss of data"
// Move the semi-useful warning "(small value) assigned to (big val), Conversion Supplied"
//   (useful because that conversion is kinda slow on Pentiums, usually requiring a movsx)
// Move more useful warning "local variable initialized but not referenced" to Level 3.

#pragma warning( disable : 4244 )       // (big val) converted to (small val). Possible loss of data
#pragma warning( disable : 4305 )       // truncation from 'const double ' to 'float '
#pragma warning( disable : 4514 )       // Unreferenced inline function has been removed (yeah, we know)
#pragma warning( 4 : 4761 )             // (small value) assigned to (big val), Conversion Supplied
#pragma warning( 4 : 4142 )             // benign redefinition of type (ex: unsigned char to char)

// Release Optimizations / Options
// -------------------------------
// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
// set the 512-byte alignment .. cuts back on whitespace in output files (EXE/DLL/LIB)

#ifndef _DEBUG
#pragma warning( 3 : 4189 )             // local variable initialized but not referenced (moved from lvl 4)
#pragma optimize("gy",on)
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/opt:nowin98")
#endif
#endif

// This included for future unicode support, or something.  Anything in
// mikmod that is an actual character string uses this type.  Things that
// want to be bytes use the std c 'char' type.

#ifndef VOID
typedef unsigned char   CHAR;
#endif

// finally got tired of typing in 'unsigned' all the time!
// Already defined on my copy of Linux GCC. Seems to be
// a BSD thing. [JEL]

#ifdef WIN32
#ifndef uint
typedef unsigned int    uint;    // must be at least 16 bits!
#endif

#ifndef ulong
typedef unsigned long   ulong;   // must be at least 32 bits!
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

// Strict datatypes.

#ifdef __OS2__

typedef signed char     SBYTE;          /* has to be 1 byte signed */
typedef unsigned char   UBYTE;          /* has to be 1 byte unsigned */
typedef signed short    SWORD;          /* has to be 2 bytes signed */
typedef unsigned short  UWORD;          /* has to be 2 bytes unsigned */
typedef signed long     SLONG;          /* has to be 4 bytes signed */
/* ULONG and BOOL are already defined in OS2.H */

#elif defined(__alpha)

typedef signed char     SBYTE;          /* has to be 1 byte signed */
typedef unsigned char   UBYTE;          /* has to be 1 byte unsigned */
typedef signed short    SWORD;          /* has to be 2 bytes signed */
typedef unsigned short  UWORD;          /* has to be 2 bytes unsigned */
/* long is 8 bytes on dec alpha - RCA */
typedef signed int      SLONG;          /* has to be 4 bytes signed */
typedef unsigned int    ULONG;          /* has to be 4 bytes unsigned */

#ifdef _cplusplus
typedef bool            BOOL;           /* doesn't matter.. 0=FALSE, <>0 true */
#else
typedef int             BOOL;           /* doesn't matter.. 0=FALSE, <>0 true */
typedef int             bool;
#endif

typedef unsigned long   INT64U;
typedef long            INT64S;

#else

typedef signed char     SBYTE;          /* has to be 1 byte signed */
typedef unsigned char   UBYTE;          /* has to be 1 byte unsigned */
typedef signed short    SWORD;          /* has to be 2 bytes signed */
typedef unsigned short  UWORD;          /* has to be 2 bytes unsigned */
typedef signed long     SLONG;          /* has to be 4 bytes signed */
typedef unsigned long   ULONG;          /* has to be 4 bytes unsigned */
typedef int             BOOL;           /* doesn't matter.. 0=FALSE, <>0 true */
#ifndef __cplusplus
typedef int             bool;
#endif

#endif


#ifdef __OS2__
#define INCL_DOS
#define INCL_MCIOS2
#define INCL_MMIOOS2
#include <os2.h>
#include <os2me.h>
#include <mmio.h>
#endif

// 64 bit integer support!

#ifdef __WATCOMC__
#ifdef __WATCOM_INT64__
typedef __int64          INT64S;
typedef unsigned __int64 INT64U;
#endif

#elif __DJGPP__
typedef long long          INT64S;
typedef unsigned long long INT64U;

#elif __BORLANDC__
typedef __int64          INT64S;
typedef unsigned __int64 INT64U;

#elif __GNUC__
typedef long long          INT64S;
typedef unsigned long long INT64U;

#elif _MSC_VER // Microsoft Visual C
typedef __int64          INT64S;
typedef unsigned __int64 INT64U;
#endif

// Hardware Port Access

#ifdef __WATCOMC__
#define inportb(x)     inp(x)
#define outportb(x,y)  outp(x,y)
#define inport(x)      inpw(x)
#define outport(x,y)   outpw(x,y)
#define disable()      _disable()
#define enable()       _enable()
#endif

#ifdef __BORLANDC__
#define inp(x)      inportb(x)
#define outp(x,y)   outportb(x,y)
#define inpw(x)     inport(x)
#define outpw(x,y)  outport(x,y)  
#define _disable()  disable() 
#define _enable()   enable()
#endif

#ifdef __DJGPP__
#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#define inp       inportw
#define outport   outportw
#define inport    inportw
#define interrupt 
#endif

// we use memset instead of Windows-specific ZeroMemory because it is more
// cross-platform friendly.

#define CLEAR_STRUCT(s)			memset(&(s),0,sizeof(s))

#define _mmchr_numeric(x)       ( ((x) >= 0x30) && ((x) <= 0x39) )
#define _mmchr_whitespace(x)    ( (x) && ((x <= 32) || (x == 255)) )
#define _mmchr_notwhitespace(x) ( (x > 32) && (x != 255) )

#define _mm_exchange(x, y, type)  \
{ \
    type  __tmp = x; \
    x = y; \
    y = __tmp; \
} \


#endif


