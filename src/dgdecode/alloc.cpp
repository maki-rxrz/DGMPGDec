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


// memory allocation for MPEG2Dec3.
//
// Changed this to handle/track both width/pitch for when
// width != pitch and it simply makes things easier to have all
// information in this struct.  It now uses 32y/16uv byte alignment
// by default, which makes internal bugs easier to catch.  This can 
// easily be changed if needed.
//
// The definition of YV12PICT is in global.h
//
// tritical - May 16, 2005
YV12PICT* create_YV12PICT(int height, int width, int chroma_format) 
{
	YV12PICT* pict;
	pict = (YV12PICT*)malloc(sizeof(YV12PICT));
	int uvwidth, uvheight;
	if (chroma_format == 1) // 4:2:0
	{
		uvwidth = width>>1;
		uvheight = height>>1;
	}
	else if (chroma_format == 2) // 4:2:2
	{
		uvwidth = width>>1;
		uvheight = height;
	}
	else // 4:4:4
	{
		uvwidth = width;
		uvheight = height;
	}
	int uvpitch = (((uvwidth+15)>>4)<<4);
	int ypitch = uvpitch*2;
	pict->y = (unsigned char*)aligned_malloc(height*ypitch,32);
	pict->u = (unsigned char*)aligned_malloc(uvheight*uvpitch,16);
	pict->v = (unsigned char*)aligned_malloc(uvheight*uvpitch,16);
	pict->ypitch = ypitch;
	pict->uvpitch = uvpitch;
	pict->ywidth = width;
	pict->uvwidth = uvwidth;
	pict->yheight = height;
	pict->uvheight = uvheight;
	return pict;
}

void destroy_YV12PICT(YV12PICT * pict) 
{
	aligned_free(pict->y);
	aligned_free(pict->u);
	aligned_free(pict->v);
	free(pict);
}