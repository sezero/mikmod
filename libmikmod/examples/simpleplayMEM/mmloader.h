#ifndef _mmloader_h__
#define _mmloader_h__

typedef struct _MY_MEMREADER {
	MREADER core;
	void *buffer;
	long len;
	long pos;
} MY_MEMREADER;

MREADER *my_new_mem_reader (void *buffer, int len);
void my_delete_mem_reader (MREADER* reader);

#endif

