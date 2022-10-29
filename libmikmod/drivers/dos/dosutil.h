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

#ifndef __DOSUTIL_H__
#define __DOSUTIL_H__

extern int dpmi_allocate_dos_memory(int paragraphs, int *ret_selector_or_max);
extern int dpmi_free_dos_memory(int selector);
extern int dpmi_resize_dos_memory(int selector, int newpara, int *ret_max);

extern int dpmi_lock_linear_region(unsigned long address, unsigned long size);
extern int dpmi_unlock_linear_region(unsigned long address, unsigned long size);
extern int dpmi_lock_linear_region_base(void *address, unsigned long size);
extern int dpmi_unlock_linear_region_base(void *address, unsigned long size);

#ifdef __WATCOMC__
#include <conio.h>

#define inportb(x)    inp(x)
#define outportb(x,y) outp(x,y)
#define inportw(x)    inpw(x)
#define outportw(x,y) outpw(x,y)
#define inportl(x)    inpl(x)
#define outportl(x,y) outpl(x,y)

extern int enable();
extern int disable();
#pragma aux enable = "sti" "mov eax,1"
#pragma aux disable = "cli" "mov eax,1"

#else
#include <pc.h>
#endif

#endif
