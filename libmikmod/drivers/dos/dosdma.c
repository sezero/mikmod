/* Implementation of DMA routines on DOS.
 * Copyright (C) 1999 by Andrew Zabolotny <bit@eltech.ru>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <dos.h>
#include <malloc.h>

#ifdef __DJGPP__
#include <sys/nearptr.h>
#endif

#include "dosdma.h"
#include "mikmod.h"

__dma_regs dma[8] = {
/* *INDENT-OFF* */
	{DMA_ADDR_0, DMA_PAGE_0, DMA_SIZE_0,
	 DMA1_MASK_REG, DMA1_CLEAR_FF_REG, DMA1_MODE_REG},
	{DMA_ADDR_1, DMA_PAGE_1, DMA_SIZE_1,
	 DMA1_MASK_REG, DMA1_CLEAR_FF_REG, DMA1_MODE_REG},

	{DMA_ADDR_2, DMA_PAGE_2, DMA_SIZE_2,
	 DMA1_MASK_REG, DMA1_CLEAR_FF_REG, DMA1_MODE_REG},
	{DMA_ADDR_3, DMA_PAGE_3, DMA_SIZE_3,
	 DMA1_MASK_REG, DMA1_CLEAR_FF_REG, DMA1_MODE_REG},

	{DMA_ADDR_4,          0, DMA_SIZE_4,
	 DMA2_MASK_REG, DMA2_CLEAR_FF_REG, DMA2_MODE_REG},
	{DMA_ADDR_5, DMA_PAGE_5, DMA_SIZE_5,
	 DMA2_MASK_REG, DMA2_CLEAR_FF_REG, DMA2_MODE_REG},

	{DMA_ADDR_6, DMA_PAGE_6, DMA_SIZE_6,
	 DMA2_MASK_REG, DMA2_CLEAR_FF_REG, DMA2_MODE_REG},
	{DMA_ADDR_7, DMA_PAGE_7, DMA_SIZE_7,
	 DMA2_MASK_REG, DMA2_CLEAR_FF_REG, DMA2_MODE_REG}
/* *INDENT-ON* */
};

static int __initialized = 0;
static int __buffer_count = 0;

int dma_initialize()
{
#ifdef __DJGPP__
	if (!__djgpp_nearptr_enable())
		return 0;

	/* Trick: Avoid re-setting DS selector limit on each memory allocation
	   call */
	__djgpp_selector_limit = 0xffffffff;
#endif

	if (dpmi_lock_linear_region_base(dma, sizeof(dma)))
		return 0;

	return (__initialized = 1);
}

void dma_finalize()
{
	if (!__initialized)
		return;
	dpmi_unlock_linear_region_base(dma, sizeof(dma));
#ifdef __DJGPP__
	__djgpp_nearptr_disable();
#endif
}

dma_buffer *dma_allocate(unsigned int channel, unsigned int size)
{
	int parsize = (size + 15) >> 4;	/* size in paragraphs */
	int par = 0;				/* Real-mode paragraph */
	int selector = 0;			/* Protected-mode selector */
	int mask = channel <= 3 ? 0xfff : 0x1fff;	/* Alignment mask in para. */
	int allocsize = parsize;	/* Allocated size in paragraphs */
	int count;					/* Try count */
	int bound = 0;				/* Nearest bound address */
	int maxsize;				/* Maximal possible block size */
	dma_buffer *buffer = NULL;

	if (!dma_initialize())
		return NULL;

	/* Loop until we'll get a properly aligned memory block */
	for (count = 8; count; count--) {
		int resize = (selector != 0);

		/* Try first to resize (possibly previously) allocated block */
		if (resize) {
			maxsize = (bound + parsize) - par;
			if (maxsize > parsize * 2)
				maxsize = parsize * 2;
			if (maxsize == allocsize)
				resize = 0;
			else {
				allocsize = maxsize;
				if (dpmi_resize_dos_memory(selector, allocsize, &maxsize) !=
					0) resize = 0;
			}
		}

		if (!resize) {
			if (selector)
				dpmi_free_dos_memory(selector), selector = 0;
			par = dpmi_allocate_dos_memory(allocsize, &selector);
		}

		if ((par == 0) || (par == -1))
			goto exit;

		/* If memory block contains a properly aligned portion, quit loop */
		bound = (par + mask + 1) & ~mask;
		if (par + parsize <= bound)
			break;
		if (bound + parsize <= par + allocsize) {
			par = bound;
			break;
		}
	}
	if (!count) {
		dpmi_free_dos_memory(selector);
		goto exit;
	}

	buffer = (dma_buffer *) MikMod_malloc(sizeof(dma_buffer));
#ifdef __DJGPP__
	buffer->linear = (unsigned char *)(__djgpp_conventional_base + bound * 16);
#else
	buffer->linear = (unsigned char *)(bound * 16);
#endif
	buffer->physical = bound * 16;
	buffer->size = parsize * 16;
	buffer->selector = selector;
	buffer->channel = channel;

	/*
	   Don't pay attention to return code since under plain DOS it often
	   returns error (at least under HIMEM/CWSDPMI and EMM386/DPMI)
	 */
	dpmi_lock_linear_region(buffer->physical, buffer->size);

	/* Lock the DMA buffer control structure as well */
	if (dpmi_lock_linear_region_base(buffer, sizeof(dma_buffer))) {
		dpmi_unlock_linear_region(buffer->physical, buffer->size);
		dpmi_free_dos_memory(selector);
		MikMod_free(buffer);
		buffer = NULL;
		goto exit;
	}

  exit:
	if (buffer)
		__buffer_count++;
	else if (--__buffer_count == 0)
		dma_finalize();
	return buffer;
}

void dma_free(dma_buffer * buffer)
{
	if (!buffer)
		return;

	dpmi_unlock_linear_region(buffer->physical, buffer->size);
	dpmi_free_dos_memory(buffer->selector);
	MikMod_free(buffer);

	if (--__buffer_count == 0)
		dma_finalize();
}

void dma_start(dma_buffer * buffer, unsigned long count, unsigned char mode)
{
	/* Disable interrupts */
	int old_ints = disable();
	dma_disable(buffer->channel);
	dma_set_mode(buffer->channel, mode);
	dma_clear_ff(buffer->channel);
	dma_set_addr(buffer->channel, buffer->physical);
	dma_clear_ff(buffer->channel);
	dma_set_count(buffer->channel, count);
	dma_enable(buffer->channel);
	/* Re-enable interrupts */
	if (old_ints)
		enable();
}
