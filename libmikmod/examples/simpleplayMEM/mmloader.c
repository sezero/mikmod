/* mmloader.c
 * Example on how to implement a MLOADER that reads from
 * memory for libmikmod.
 * (C) 2004, Raphael Assenat (raph@raphnet.net)
 *
 * This example is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRENTY; without event the implied warrenty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <mikmod.h>
#include "mmloader.h"

static BOOL _mm_MemReader_Eof(MREADER* reader);
static BOOL _mm_MemReader_Read(MREADER* reader,void* ptr,size_t size);
static int _mm_MemReader_Get(MREADER* reader);
static BOOL _mm_MemReader_Seek(MREADER* reader,long offset,int whence);
static long _mm_MemReader_Tell(MREADER* reader);

void my_delete_mem_reader(MREADER* reader)
{
	if (reader) { MikMod_free(reader); }
}

MREADER *my_new_mem_reader(void *buffer, int len)
{
	MY_MEMREADER* reader=(MY_MEMREADER*)MikMod_malloc(sizeof(MY_MEMREADER));
	if (reader)
	{
		reader->core.Eof =&_mm_MemReader_Eof;
		reader->core.Read=&_mm_MemReader_Read;
		reader->core.Get =&_mm_MemReader_Get;
		reader->core.Seek=&_mm_MemReader_Seek;
		reader->core.Tell=&_mm_MemReader_Tell;
		reader->buffer = buffer;
		reader->len = len;
		reader->pos = 0;
	}
	return (MREADER*)reader;
}

static BOOL _mm_MemReader_Eof(MREADER* reader)
{
	if (!reader) { return 1; }
	if ( ((MY_MEMREADER*)reader)->pos > ((MY_MEMREADER*)reader)->len ) {
		return 1;
	}
	return 0;
}

static BOOL _mm_MemReader_Read(MREADER* reader,void* ptr,size_t size)
{
	unsigned char *d=ptr, *s;

	if (!reader) { return 0; }

	if (reader->Eof(reader)) { return 0; }

	s = ((MY_MEMREADER*)reader)->buffer;
	s += ((MY_MEMREADER*)reader)->pos;

	if ( ((MY_MEMREADER*)reader)->pos + size >= ((MY_MEMREADER*)reader)->len)
	{
		((MY_MEMREADER*)reader)->pos = ((MY_MEMREADER*)reader)->len;
		return 0; /* not enough remaining bytes */
	}

	((MY_MEMREADER*)reader)->pos += size;

	while (size--)
	{
		*d = *s;
		s++;
		d++;
	}

	return 1;
}

static int _mm_MemReader_Get(MREADER* reader)
{
	int pos;

	if (reader->Eof(reader)) { return 0; }

	pos = ((MY_MEMREADER*)reader)->pos;
	((MY_MEMREADER*)reader)->pos++;

	return ((unsigned char*)(((MY_MEMREADER*)reader)->buffer))[pos];
}

static BOOL _mm_MemReader_Seek(MREADER* reader,long offset,int whence)
{
	if (!reader) { return -1; }

	switch(whence)
	{
		case SEEK_CUR:
			((MY_MEMREADER*)reader)->pos += offset;
			break;
		case SEEK_SET:
			((MY_MEMREADER*)reader)->pos = offset;
			break;
		case SEEK_END:
			((MY_MEMREADER*)reader)->pos = ((MY_MEMREADER*)reader)->len - offset - 1;
			break;
	}
	if ( ((MY_MEMREADER*)reader)->pos < 0) { ((MY_MEMREADER*)reader)->pos = 0; }
	if ( ((MY_MEMREADER*)reader)->pos > ((MY_MEMREADER*)reader)->len ) {
		((MY_MEMREADER*)reader)->pos = ((MY_MEMREADER*)reader)->len;
	}
	return 0;
}

static long _mm_MemReader_Tell(MREADER* reader)
{
	if (reader) {
		return ((MY_MEMREADER*)reader)->pos;
	}
	return 0;
}
