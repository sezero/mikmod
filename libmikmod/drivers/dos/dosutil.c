/* DOS glue code for DJGPP / Watcom compatibility
 * Written by Cameron Cawley <ccawley2011@gmail.com>
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

#include "dosutil.h"

#if defined(__DJGPP__)

#include <sys/version.h>
#include <dpmi.h>
#include <sys/nearptr.h>

/* BUG WARNING:  there is an error in DJGPP libraries <= 2.01:
 * src/libc/dpmi/api/d0102.s loads the selector and allocsize
 * arguments in the wrong order.  DJGPP >= 2.02 have it fixed. */
#if (!defined(__DJGPP_MINOR__) || (__DJGPP_MINOR__+0) < 2)
#error __dpmi_resize_dos_memory() from DJGPP <= 2.01 is broken!
#endif

int dpmi_allocate_dos_memory(int paragraphs, int *ret_selector_or_max) {
	return __dpmi_allocate_dos_memory(paragraphs, ret_selector_or_max);
}

int dpmi_free_dos_memory(int selector) {
	return __dpmi_free_dos_memory(selector);
}

int dpmi_resize_dos_memory(int selector, int newpara, int *ret_max) {
	return __dpmi_resize_dos_memory(selector, newpara, ret_max);
}

int dpmi_lock_linear_region(unsigned long address, unsigned long size) {
	__dpmi_meminfo info;

	info.address = address;
	info.size = size;
	return __dpmi_lock_linear_region(&info);
}

int dpmi_unlock_linear_region(unsigned long address, unsigned long size) {
	__dpmi_meminfo info;

	info.address = address;
	info.size = size;
	return __dpmi_unlock_linear_region(&info);
}

int dpmi_lock_linear_region_base(void *address, unsigned long size) {
	return dpmi_lock_linear_region(__djgpp_base_address + (unsigned long)address, size);
}

int dpmi_unlock_linear_region_base(void *address, unsigned long size) {
	return dpmi_unlock_linear_region(__djgpp_base_address + (unsigned long)address, size);
}

#elif defined(__WATCOMC__)

#include <i86.h>

int dpmi_allocate_dos_memory(int paragraphs, int *ret_selector_or_max) {
	union REGS r;

	r.x.eax = 0x0100;              /* DPMI allocate DOS memory */
	r.x.ebx = paragraphs;          /* Number of paragraphs */
	int386 (0x31, &r, &r);
	if (r.w.cflag & 1)
		return (-1);

	*ret_selector_or_max = r.w.dx;
	return (r.w.ax);              /* Return segment address */
}

int dpmi_free_dos_memory(int selector) {
	union REGS r;

	r.x.eax = 0x101;              /* DPMI free DOS memory */
	r.x.edx = selector;           /* Selector to free */
	int386 (0x31, &r, &r);
	return (r.w.cflag & 1) ? -1 : 0;
}

int dpmi_resize_dos_memory(int selector, int newpara, int *ret_max) {
	union REGS r;

	r.x.eax = 0x0102;              /* DPMI resize DOS memory */
	r.x.ebx = newpara;          /* Number of paragraphs */
	r.x.edx = selector;            /* Selector to free */
	int386 (0x31, &r, &r);
	if (r.w.cflag & 1) {
		*ret_max = r.x.ebx;
		return (-1);
	}
	return 0;
}

int dpmi_lock_linear_region(unsigned long address, unsigned long size) {
	union REGS r;

	r.w.ax = 0x600;                /* DPMI Lock Linear Region */
	r.w.bx = (address >> 16);      /* Linear address in BX:CX */
	r.w.cx = (address & 0xFFFF);
	r.w.si = (size >> 16);         /* Length in SI:DI */
	r.w.di = (size & 0xFFFF);
	int386 (0x31, &r, &r);
	return (r.w.cflag & 1) ? -1 : 0;
}

int dpmi_unlock_linear_region(unsigned long address, unsigned long size) {
	union REGS r;

	r.w.ax = 0x601;                /* DPMI Unlock Linear Region */
	r.w.bx = (address >> 16);      /* Linear address in BX:CX */
	r.w.cx = (address & 0xFFFF);
	r.w.si = (size >> 16);         /* Length in SI:DI */
	r.w.di = (size & 0xFFFF);
	int386 (0x31, &r, &r);
	return (r.w.cflag & 1) ? -1 : 0;
}

int dpmi_lock_linear_region_base(void *address, unsigned long size) {
	return dpmi_lock_linear_region((unsigned long)address, size);
}

int dpmi_unlock_linear_region_base(void *address, unsigned long size) {
	return dpmi_unlock_linear_region((unsigned long)address, size);
}

#endif
