/* 
 *	Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *	
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

// replace with one that doesn't need fixed size table - trbarry 3-22-2002

#include <stdlib.h>
#include "global.h"


#define ptr_t unsigned int

/*
struct B { BYTE* yy; };

extern "C" void *aligned_malloc(size_t size, size_t alignment)
{
	size_t algn = 4;
	size_t mask = 0xfffffffc;
	B* pB;

	BYTE* x;
	BYTE* y;

	while (algn < alignment)		// align to next power of 2
	{
		algn <<= 1;
		mask <<= 1;
	}

	x = (BYTE*)malloc(size+algn);
	y = (BYTE*) (((unsigned int) (x+algn) & mask) - 4);
	pB = (B*) y;
	pB->yy = x;
	return  y+4;
}

extern "C" void aligned_free(void *x)
{
	struct B { BYTE* yy; };
	B* pB = (B*) ((BYTE*)x-4);
	free(pB->yy);
}
*/

extern "C" void *aligned_malloc(size_t size, size_t alignment)
{
	BYTE *mem_ptr;

	if (!alignment)
	{

		/* We have not to satisfy any alignment */
		if ((mem_ptr = (BYTE *) malloc(size + 1)) != NULL) {

			/* Store (mem_ptr - "real allocated memory") in *(mem_ptr-1) */
			*mem_ptr = 1;

			/* Return the mem_ptr pointer */
			return (void *) (mem_ptr+1);

		}

	} else {
		BYTE *tmp;

		/*
		 * Allocate the required size memory + alignment so we
		 * can realign the data if necessary
		 */

		if ((tmp = (BYTE *) malloc(size + alignment)) != NULL) {

			/* Align the tmp pointer */
			mem_ptr =
				(BYTE *) ((ptr_t) (tmp + alignment - 1) &
							 (~(ptr_t) (alignment - 1)));

			/*
			 * Special case where malloc have already satisfied the alignment
			 * We must add alignment to mem_ptr because we must store
			 * (mem_ptr - tmp) in *(mem_ptr-1)
			 * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
			 * to a forbidden memory space
			 */
			if (mem_ptr == tmp)
				mem_ptr += alignment;

			/*
			 * (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
			 * the real malloc block allocated and free it in xvid_free
			 */
			*(mem_ptr - 1) = (BYTE) (mem_ptr - tmp);

			/* Return the aligned pointer */
			return (void *) mem_ptr;

		}
	}

	return NULL;

}

extern "C" void aligned_free(void *mem_ptr)
{
	/* *(mem_ptr - 1) give us the offset to the real malloc block */
	if (mem_ptr)
		free((BYTE *) mem_ptr - *((BYTE *) mem_ptr - 1));

}


//  YV12 pictures memory allocation for MPEG2Dec3. MarcFD 25 nov 2002


YV12PICT* create_YV12PICT(int height, int width) 
{
	YV12PICT* pict;
	pict = (YV12PICT*)malloc(sizeof(YV12PICT));
	pict->y = (unsigned char*)aligned_malloc(height*width,128);
	pict->u = (unsigned char*)aligned_malloc(height*width/4,128);
	pict->v = (unsigned char*)aligned_malloc(height*width/4,128);
	pict->ypitch = width;
	pict->uvpitch = width/2;
	return pict;
}
void destroy_YV12PICT(YV12PICT * pict) 
{
	aligned_free(pict->y);
	aligned_free(pict->u);
	aligned_free(pict->v);
	free(pict);
}
