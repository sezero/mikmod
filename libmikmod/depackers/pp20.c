/* MikMod sound library (c) 2003-2015 Raphael Assenat and others -
 * see AUTHORS file for a complete list.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* amiga Powerpack PP20 decompression support
 *
 * Based on public domain versions by Olivier Lapicque <olivierl@jps.net>
 * from the libmodplug library (sezero's fork of libmodplug at github:
 * http://github.com/sezero/libmodplug/tree/sezero), with some extra bits
 * from ppdepack by Stuart Caie <kyzer@4u.net> as it exists in the libxmp
 * library of Claudio Matsuoka.
 *
 * Rewritten for libmikmod by O. Sezer <sezero@users.sourceforge.net>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>

#include "mikmod_internals.h"

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

typedef struct _PPBITBUFFER
{
	ULONG bitcount;
	ULONG bitbuffer;
	const UBYTE *pStart;
	const UBYTE *pSrc;
} PPBITBUFFER;


static ULONG PP20_GetBits(PPBITBUFFER* b, ULONG n)
{
	ULONG result = 0;
	ULONG i;

	for (i=0; i<n; i++)
	{
		if (!b->bitcount)
		{
			b->bitcount = 8;
			if (b->pSrc != b->pStart) b->pSrc--;
			b->bitbuffer = *b->pSrc;
		}
		result = (result<<1) | (b->bitbuffer&1);
		b->bitbuffer >>= 1;
		b->bitcount--;
	}
	return result;
}


static BOOL PP20_DoUnpack(const UBYTE *pSrc, ULONG nSrcLen, UBYTE *pDst, ULONG nDstLen)
{
	PPBITBUFFER BitBuffer;
	ULONG nBytesLeft;

	BitBuffer.pStart = pSrc + 4;
	BitBuffer.pSrc = pSrc + nSrcLen - 4;
	BitBuffer.bitbuffer = 0;
	BitBuffer.bitcount = 0;
	PP20_GetBits(&BitBuffer, pSrc[nSrcLen-1]);
	nBytesLeft = nDstLen;
	while (nBytesLeft > 0)
	{
		if (!PP20_GetBits(&BitBuffer, 1))
		{
			ULONG n = 1, i;
			while (n < nBytesLeft)
			{
				ULONG code = PP20_GetBits(&BitBuffer, 2);
				n += code;
				if (code != 3) break;
			}
			for (i=0; i<n; i++)
			{
				pDst[--nBytesLeft] = (UBYTE)PP20_GetBits(&BitBuffer, 8);
			}
			if (!nBytesLeft) break;
		}
		{
			ULONG n = PP20_GetBits(&BitBuffer, 2)+1;
			ULONG nbits;
			ULONG nofs, i;
			if (n < 1 || n-1 >= nSrcLen) return 0; /* can this ever happen? */
			nbits = pSrc[n-1];
			if (n==4)
			{
				nofs = PP20_GetBits(&BitBuffer, (PP20_GetBits(&BitBuffer, 1)) ? nbits : 7 );
				while (n < nBytesLeft)
				{
					ULONG code = PP20_GetBits(&BitBuffer, 3);
					n += code;
					if (code != 7) break;
				}
			} else
			{
				nofs = PP20_GetBits(&BitBuffer, nbits);
			}
			for (i=0; i<=n; i++)
			{
				pDst[nBytesLeft-1] = (nBytesLeft+nofs < nDstLen) ? pDst[nBytesLeft+nofs] : 0;
				if (!--nBytesLeft) break;
			}
		}
	}
	return 1;
}

BOOL PP20_Unpack(MREADER* reader, void** out, int* outlen)
{
	ULONG srclen, destlen;
	UBYTE *destbuf, *srcbuf;
	UBYTE tmp[4];
	BOOL ret;

	_mm_fseek(reader,0,SEEK_END);
	srclen = _mm_ftell(reader);
	if (srclen < 256) return 0;
	/* file length should be a multiple of 4 */
	if (srclen & 3) return 0;

	_mm_rewind(reader);
	if (_mm_read_I_ULONG(reader) != 0x30325050)	/* 'PP20' */
		return 0;

	_mm_fseek(reader,srclen-4,SEEK_SET);
	_mm_read_UBYTES(tmp,4,reader);
	destlen = tmp[0] << 16;
	destlen |= tmp[1] << 8;
	destlen |= tmp[2];

	_mm_fseek(reader,4,SEEK_SET);
	_mm_read_UBYTES(tmp,4,reader);

	/* original pp20 only support efficiency
	 * from 9 9 9 9 up to 9 10 12 13, afaik,
	 * but the xfd detection code says this...
	 *
	 * move.l 4(a0),d0
	 * cmp.b #9,d0
	 * blo.b .Exit
	 * and.l #$f0f0f0f0,d0
	 * bne.s .Exit
	 */
	if ((tmp[0] < 9) || (tmp[0] & 0xf0)) return 0;
	if ((tmp[1] < 9) || (tmp[1] & 0xf0)) return 0;
	if ((tmp[2] < 9) || (tmp[2] & 0xf0)) return 0;
	if ((tmp[3] < 9) || (tmp[3] & 0xf0)) return 0;

	if ((destlen < 512) || (destlen > 0x400000) || (destlen > 16*srclen))
		return 0;
	if ((destbuf = (UBYTE*)MikMod_malloc(destlen)) == NULL)
		return 0;

	if ((srcbuf = (UBYTE*)MikMod_malloc(srclen)) == NULL) {
		MikMod_free(destbuf);
		return 0;
	}
	_mm_fseek(reader,4,SEEK_SET);
	_mm_read_UBYTES(srcbuf,srclen-4,reader);

	ret = PP20_DoUnpack(srcbuf, srclen-4, destbuf, destlen);
	MikMod_free(srcbuf);

	if (!ret) {
		MikMod_free(destbuf);
	}
	else {
		*out = destbuf;
		*outlen = destlen;
	}
	return ret;
}
