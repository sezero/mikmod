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
#include <limits.h>

#include <mikmod.h>
#include "mmloader.h"

static BOOL _mm_MemReader_Eof(MREADER* reader);
static BOOL _mm_MemReader_Read(MREADER* reader,void* ptr,size_t size);
static int _mm_MemReader_Get(MREADER* reader);
static int _mm_MemReader_Seek(MREADER* reader,long offset,int whence);
static long _mm_MemReader_Tell(MREADER* reader);

void my_delete_mem_reader(MREADER* reader)
{
	if (reader) MikMod_free(reader);
}

MREADER *my_new_mem_reader(const void *buffer, int len)
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
	MY_MEMREADER* mr = (MY_MEMREADER*) reader;
	if (!mr) return 1;
	if (mr->pos >= mr->len) return 1;
	return 0;
}

static BOOL _mm_MemReader_Read(MREADER* reader,void* ptr,size_t size)
{
	unsigned char *d;
	const unsigned char *s;
	MY_MEMREADER* mr;
	long siz;
	BOOL ret;

	if (!reader || !size || (size > (size_t) LONG_MAX))
		return 0;

	mr = (MY_MEMREADER*) reader;
	siz = (long) size;
	if (mr->pos >= mr->len) return 0;	/* @ eof */
	if (mr->pos + siz > mr->len) {
		siz = mr->len - mr->pos;
		ret = 0; /* not enough remaining bytes */
	}
	else {
		ret = 1;
	}

	s = mr->buffer;
	s += mr->pos;
	mr->pos += siz;
	d = ptr;

	while (siz) {
		*d++ = *s++;
		siz--;
	}

	return ret;
}

static int _mm_MemReader_Get(MREADER* reader)
{
	MY_MEMREADER* mr;
	int c;

	mr = (MY_MEMREADER*) reader;
	if (mr->pos >= mr->len) return EOF;
	c = ((const unsigned char*) mr->buffer)[mr->pos];
	mr->pos++;

	return c;
}

static int _mm_MemReader_Seek(MREADER* reader,long offset,int whence)
{
	MY_MEMREADER* mr;

	if (!reader) return -1;
	mr = (MY_MEMREADER*) reader;
	switch(whence)
	{
	case SEEK_CUR:
		mr->pos += offset;
		break;
	case SEEK_SET:
		mr->pos = reader->iobase + offset;
		break;
	case SEEK_END:
		mr->pos = mr->len + offset;
		break;
	}
	if (mr->pos < reader->iobase) {
		mr->pos = mr->core.iobase;
		return -1;
	}
	if (mr->pos > mr->len) {
		mr->pos = mr->len;
	}
	return 0;
}

static long _mm_MemReader_Tell(MREADER* reader)
{
	if (reader) {
		return ((MY_MEMREADER*)reader)->pos - reader->iobase;
	}
	return 0;
}

