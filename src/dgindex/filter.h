/*
 *  Mutated into DGIndex. Modifications Copyright (C) 2004, Donald Graft
 * 
 *	Copyright (C) Chia-chen Kuo - June 2002
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

#include <math.h>

int Clip_Width, Clip_Height;
static int DOUBLE_WIDTH, HALF_WIDTH, LUM_AREA, PROGRESSIVE_HEIGHT, INTERLACED_HEIGHT;
static int HALF_WIDTH_D8, RGB_DOWN1, RGB_DOWN2;

static int NINE_CLIP_WIDTH, QUAD_CLIP_WIDTH, DOUBLE_CLIP_WIDTH, HALF_CLIP_WIDTH;
static int CLIP_AREA, HALF_CLIP_AREA, CLIP_STEP, CLIP_HALF_STEP;

static const __int64 mmmask_0001 = 0x0001000100010001;
static const __int64 mmmask_0002 = 0x0002000200020002;
static const __int64 mmmask_0003 = 0x0003000300030003;
static const __int64 mmmask_0004 = 0x0004000400040004;
static const __int64 mmmask_0005 = 0x0005000500050005;
static const __int64 mmmask_0007 = 0x0007000700070007;
static const __int64 mmmask_0064 = 0x0040004000400040;
static const __int64 mmmask_0128 = 0x0080008000800080;

static unsigned char LuminanceTable[256];

static void InitializeFilter()
{
	int i;
	double value;

	for (i=0; i<256; i++)
	{
		value = 255.0 * pow(i/255.0, pow(2.0, -LumGamma/128.0)) + LumOffset + 0.5;

		if (value < 0)
			LuminanceTable[i] = 0;
		else if (value > 255.0)
			LuminanceTable[i] = 255;
		else
			LuminanceTable[i] = (unsigned char)value;
	}
}

static void LuminanceFilter(unsigned char *src, unsigned char *dst)
{
	int i;

	src += CLIP_AREA;
	dst += CLIP_AREA;

	for (i=0; i<LUM_AREA; i++)
		*(dst++) = LuminanceTable[*(src++)];
}

static void conv422to444(unsigned char *src, unsigned char *dst)
{
	src += HALF_CLIP_AREA;
	dst += CLIP_AREA;

	__asm
	{
		mov			eax, [src]
		mov			ebx, [dst]
		mov			edi, [Clip_Height]

		movq		mm1, [mmmask_0001]
		pxor		mm0, mm0

convyuv444init:
		movq		mm7, [eax]
		mov			esi, 0x00

convyuv444:
		movq		mm2, mm7
		movq		mm7, [eax+esi+8]
		movq		mm3, mm2
		movq		mm4, mm7

		psrlq		mm3, 8
		psllq		mm4, 56
		por			mm3, mm4

		movq		mm4, mm2
		movq		mm5, mm3

		punpcklbw	mm4, mm0
		punpcklbw	mm5, mm0

		movq		mm6, mm4
		paddusw		mm4, mm1
		paddusw		mm4, mm5
		psrlw		mm4, 1
		psllq		mm4, 8
		por			mm4, mm6

		punpckhbw	mm2, mm0
		punpckhbw	mm3, mm0

		movq		mm6, mm2
		paddusw		mm2, mm1
		paddusw		mm2, mm3

		movq		[ebx+esi*2], mm4

		psrlw		mm2, 1
		psllq		mm2, 8
		por			mm2, mm6

		add			esi, 0x08
		cmp			esi, [HALF_WIDTH_D8]
		movq		[ebx+esi*2-8], mm2
		jl			convyuv444

		movq		mm2, mm7
		punpcklbw	mm2, mm0
		movq		mm3, mm2

		psllq		mm2, 8
		por			mm2, mm3

		movq		[ebx+esi*2], mm2

		punpckhbw	mm7, mm0
		movq		mm6, mm7

		psllq		mm6, 8
		por			mm6, mm7

		movq		[ebx+esi*2+8], mm6

		add			eax, [HALF_WIDTH]		
		add			ebx, [Coded_Picture_Width]
		dec			edi
		cmp			edi, 0x00
		jg			convyuv444init
	}
}

static void conv420to422(unsigned char *src, unsigned char *dst, bool frame_type)
{
	if (frame_type)
	{
		__asm
		{
			mov			eax, [src]
			mov			ebx, [dst]
			mov			ecx, ebx
			add			ecx, [HALF_WIDTH]
			mov			esi, 0x00
			movq		mm3, [mmmask_0003]
			pxor		mm0, mm0
			movq		mm4, [mmmask_0002]

			mov			edx, eax
			add			edx, [HALF_WIDTH]
convyuv422topp:
			movd		mm1, [eax+esi]
			movd		mm2, [edx+esi]
			movd		[ebx+esi], mm1
			punpcklbw	mm1, mm0
			pmullw		mm1, mm3
			paddusw		mm1, mm4
			punpcklbw	mm2, mm0
			paddusw		mm2, mm1
			psrlw		mm2, 0x02
			packuswb	mm2, mm0

			add			esi, 0x04
			cmp			esi, [HALF_WIDTH]
			movd		[ecx+esi-4], mm2
			jl			convyuv422topp

			add			eax, [HALF_WIDTH]
			add			ebx, [Coded_Picture_Width]
			add			ecx, [Coded_Picture_Width]
			mov			esi, 0x00

			mov			edi, [PROGRESSIVE_HEIGHT]
convyuv422p:
			movd		mm1, [eax+esi]

			punpcklbw	mm1, mm0
			mov			edx, eax

			pmullw		mm1, mm3
			sub			edx, [HALF_WIDTH]

			movd		mm5, [edx+esi]
			movd		mm2, [edx+esi]

			punpcklbw	mm5, mm0
			punpcklbw	mm2, mm0
			paddusw		mm5, mm1
			paddusw		mm2, mm1
			paddusw		mm5, mm4
			paddusw		mm2, mm4
			psrlw		mm5, 0x02
			psrlw		mm2, 0x02
			packuswb	mm5, mm0
			packuswb	mm2, mm0

			mov			edx, eax
			add			edx, [HALF_WIDTH]
			add			esi, 0x04
			cmp			esi, [HALF_WIDTH]
			movd		[ebx+esi-4], mm5
			movd		[ecx+esi-4], mm2

			jl			convyuv422p

			add			eax, [HALF_WIDTH]
			add			ebx, [Coded_Picture_Width]
			add			ecx, [Coded_Picture_Width]
			mov			esi, 0x00
			dec			edi
			cmp			edi, 0x00
			jg			convyuv422p

			mov			edx, eax
			sub			edx, [HALF_WIDTH]
convyuv422bottomp:
			movd		mm1, [eax+esi]
			movd		mm5, [edx+esi]
			punpcklbw	mm5, mm0
			movd		[ecx+esi], mm1

			punpcklbw	mm1, mm0
			pmullw		mm1, mm3
			paddusw		mm5, mm1
			paddusw		mm5, mm4
			psrlw		mm5, 0x02
			packuswb	mm5, mm0

			add			esi, 0x04
			cmp			esi, [HALF_WIDTH]
			movd		[ebx+esi-4], mm5
			jl			convyuv422bottomp
		}
	}
	else
	{
		__asm
		{
			mov			eax, [src]
			mov			ecx, [dst]
			mov			esi, 0x00
			pxor		mm0, mm0
			movq		mm3, [mmmask_0003]
			movq		mm4, [mmmask_0004]
			movq		mm5, [mmmask_0005]

convyuv422topi:
			movd		mm1, [eax+esi]
			mov			ebx, eax
			add			ebx, [HALF_WIDTH]
			movd		mm2, [ebx+esi]
			movd		[ecx+esi], mm1
			punpcklbw	mm1, mm0
			movq		mm6, mm1
			pmullw		mm1, mm3

			punpcklbw	mm2, mm0
			movq		mm7, mm2
			pmullw		mm2, mm5
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			mov			edx, ecx
			add			edx, [HALF_WIDTH]
			pmullw		mm6, mm5
			movd		[edx+esi], mm2

			add			ebx, [HALF_WIDTH]
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			pmullw		mm2, mm3
			paddusw		mm2, mm6
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, [HALF_WIDTH]
			add			ebx, [HALF_WIDTH]
			pmullw		mm7, [mmmask_0007]
			movd		[edx+esi], mm2

			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			paddusw		mm2, mm7
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, [HALF_WIDTH]
			add			esi, 0x04
			cmp			esi, [HALF_WIDTH]
			movd		[edx+esi-4], mm2

			jl			convyuv422topi

			add			eax, [Coded_Picture_Width]
			add			ecx, [DOUBLE_WIDTH]
			mov			esi, 0x00

			mov			edi, [INTERLACED_HEIGHT]
convyuv422i:
			movd		mm1, [eax+esi]
			punpcklbw	mm1, mm0
			movq		mm6, mm1
			mov			ebx, eax
			sub			ebx, [Coded_Picture_Width]
			movd		mm3, [ebx+esi]
			pmullw		mm1, [mmmask_0007]
			punpcklbw	mm3, mm0
			paddusw		mm3, mm1
			paddusw		mm3, mm4
			psrlw		mm3, 0x03
			packuswb	mm3, mm0

			add			ebx, [HALF_WIDTH]
			movq		mm1, [ebx+esi]
			add			ebx, [Coded_Picture_Width]
			movd		[ecx+esi], mm3

			movq		mm3, [mmmask_0003]
			movd		mm2, [ebx+esi]

			punpcklbw	mm1, mm0
			pmullw		mm1, mm3
			punpcklbw	mm2, mm0
			movq		mm7, mm2
			pmullw		mm2, mm5
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			pmullw		mm6, mm5
			mov			edx, ecx
			add			edx, [HALF_WIDTH]
			movd		[edx+esi], mm2

			add			ebx, [HALF_WIDTH]
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			pmullw		mm2, mm3
			paddusw		mm2, mm6
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			pmullw		mm7, [mmmask_0007]
			add			edx, [HALF_WIDTH]
			add			ebx, [HALF_WIDTH]
 			movd		[edx+esi], mm2

			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			paddusw		mm2, mm7
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, [HALF_WIDTH]
			add			esi, 0x04
			cmp			esi, [HALF_WIDTH]
			movd		[edx+esi-4], mm2

			jl			convyuv422i
			add			eax, [Coded_Picture_Width]
			add			ecx, [DOUBLE_WIDTH]
			mov			esi, 0x00
			dec			edi
			cmp			edi, 0x00
			jg			convyuv422i

convyuv422bottomi:
			movd		mm1, [eax+esi]
			movq		mm6, mm1
			punpcklbw	mm1, mm0
			mov			ebx, eax
			sub			ebx, [Coded_Picture_Width]
			movd		mm3, [ebx+esi]
			punpcklbw	mm3, mm0
			pmullw		mm1, [mmmask_0007]
			paddusw		mm3, mm1
			paddusw		mm3, mm4
			psrlw		mm3, 0x03
			packuswb	mm3, mm0

			add			ebx, [HALF_WIDTH]
			movq		mm1, [ebx+esi]
			punpcklbw	mm1, mm0
			movd		[ecx+esi], mm3

			pmullw		mm1, [mmmask_0003]
			add			ebx, [Coded_Picture_Width]
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			movq		mm7, mm2
			pmullw		mm2, mm5
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			mov			edx, ecx
			add			edx, [HALF_WIDTH]
			pmullw		mm7, [mmmask_0007]
			movd		[edx+esi], mm2

			add			edx, [HALF_WIDTH]
			movd		[edx+esi], mm6

			punpcklbw	mm6, mm0
			paddusw		mm6, mm7
			paddusw		mm6, mm4
			psrlw		mm6, 0x03
			packuswb	mm6, mm0

			add			edx, [HALF_WIDTH]
			add			esi, 0x04
			cmp			esi, [HALF_WIDTH]
			movd		[edx+esi-4], mm6

			jl			convyuv422bottomi
		}
	}
}

static void conv422toyuy2odd(unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst)
{
	py += CLIP_STEP;
	pu += CLIP_HALF_STEP;
	pv += CLIP_HALF_STEP;

	__asm
	{
		mov			eax, [py]
		mov			ebx, [pu]
		mov			ecx, [pv]
		mov			edx, [dst]
		mov			esi, 0x00
		mov			edi, [Clip_Height]

yuy2conv:
		movd		mm2, [ebx+esi]
		movd		mm3, [ecx+esi]
		punpcklbw	mm2, mm3
		movq		mm1, [eax+esi*2]
		movq		mm4, mm1
		punpcklbw	mm1, mm2
		punpckhbw	mm4, mm2

		add			esi, 0x04
		cmp			esi, [HALF_CLIP_WIDTH]
		movq		[edx+esi*4-16], mm1
		movq		[edx+esi*4-8], mm4
		jl			yuy2conv

		add			eax, [DOUBLE_WIDTH]
		add			ebx, [Coded_Picture_Width]
		add			ecx, [Coded_Picture_Width]
		add			edx, [QUAD_CLIP_WIDTH]
		sub			edi, 0x02
		mov			esi, 0x00
		cmp			edi, 0x00
		jg			yuy2conv

		emms
	}
}

static void conv422toyuy2even(unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst)
{
	py += Coded_Picture_Width + CLIP_STEP;
	pu += HALF_WIDTH + CLIP_HALF_STEP;
	pv += HALF_WIDTH + CLIP_HALF_STEP;
	dst += DOUBLE_CLIP_WIDTH;

	__asm
	{
		mov			eax, [py]
		mov			ebx, [pu]
		mov			ecx, [pv]
		mov			edx, [dst]
		mov			esi, 0x00
		mov			edi, [Clip_Height]

yuy2conv:
		movd		mm2, [ebx+esi]
		movd		mm3, [ecx+esi]
		punpcklbw	mm2, mm3
		movq		mm1, [eax+esi*2]
		movq		mm4, mm1
		punpcklbw	mm1, mm2
		punpckhbw	mm4, mm2

		add			esi, 0x04
		cmp			esi, [HALF_CLIP_WIDTH]
		movq		[edx+esi*4-16], mm1
		movq		[edx+esi*4-8], mm4
		jl			yuy2conv

		add			eax, [DOUBLE_WIDTH]
		add			ebx, [Coded_Picture_Width]
		add			ecx, [Coded_Picture_Width]
		add			edx, [QUAD_CLIP_WIDTH]
		sub			edi, 0x02
		mov			esi, 0x00
		cmp			edi, 0x00
		jg			yuy2conv

		emms
	}
}

static void conv444toRGB24odd(unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst)
{
	py += CLIP_STEP;
	pu += CLIP_STEP;
	pv += CLIP_STEP;
	dst += RGB_DOWN1;

	__asm
	{
		mov			eax, [py]
		mov			ebx, [pu]
		mov			ecx, [pv]
		mov			edx, [dst]
		mov			edi, [Clip_Height]
		mov			esi, 0x00
		pxor		mm0, mm0

convRGB24:
		movd		mm1, [eax+esi]
		movd		mm3, [ebx+esi]
		punpcklbw	mm1, mm0
		punpcklbw	mm3, mm0
		movd		mm5, [ecx+esi]
		punpcklbw	mm5, mm0
		movq		mm7, [mmmask_0128]
		psubw		mm3, mm7
		psubw		mm5, mm7

		psubw		mm1, [RGB_Offset]
		movq		mm2, mm1
		movq		mm7, [mmmask_0001]
		punpcklwd	mm1, mm7
		punpckhwd	mm2, mm7
		movq		mm7, [RGB_Scale]
		pmaddwd		mm1, mm7
		pmaddwd		mm2, mm7

		movq		mm4, mm3
		punpcklwd	mm3, mm0
		punpckhwd	mm4, mm0
		movq		mm7, [RGB_CBU]
		pmaddwd		mm3, mm7
		pmaddwd		mm4, mm7
		paddd		mm3, mm1
		paddd		mm4, mm2
		psrld		mm3, 13
		psrld		mm4, 13
		packuswb	mm3, mm0
		packuswb	mm4, mm0

		movq		mm6, mm5
		punpcklwd	mm5, mm0
		punpckhwd	mm6, mm0
		movq		mm7, [RGB_CRV]
		pmaddwd		mm5, mm7
		pmaddwd		mm6, mm7
		paddd		mm5, mm1
		paddd		mm6, mm2

		psrld		mm5, 13
		psrld		mm6, 13
		packuswb	mm5, mm0
		packuswb	mm6, mm0

		punpcklbw	mm3, mm5
		punpcklbw	mm4, mm6
		movq		mm5, mm3
		movq		mm6, mm4
		psrlq		mm5, 16
		psrlq		mm6, 16
		por			mm3, mm5
		por			mm4, mm6

		movd		mm5, [ebx+esi]
		movd		mm6, [ecx+esi]
		punpcklbw	mm5, mm0
		punpcklbw	mm6, mm0
		movq		mm7, [mmmask_0128]
		psubw		mm5, mm7
		psubw		mm6, mm7

		movq		mm7, mm6
		punpcklwd	mm6, mm5
		punpckhwd	mm7, mm5		
		movq		mm5, [RGB_CGX]
		pmaddwd		mm6, mm5
		pmaddwd		mm7, mm5
		paddd		mm6, mm1
		paddd		mm7, mm2

		psrld		mm6, 13
		psrld		mm7, 13
		packuswb	mm6, mm0
		packuswb	mm7, mm0

		punpcklbw	mm3, mm6
		punpcklbw	mm4, mm7

		movq		mm1, mm3
		movq		mm5, mm4
		movq		mm6, mm4

		psrlq		mm1, 32
		psllq		mm1, 24
		por			mm1, mm3

		psrlq		mm3, 40
		psllq		mm6, 16
		por			mm3, mm6
		movd		[edx], mm1

		psrld		mm4, 16
		psrlq		mm5, 24
		por			mm5, mm4
		movd		[edx+4], mm3

		add			edx, 0x0c
		add			esi, 0x04
		cmp			esi, [Clip_Width]
		movd		[edx-4], mm5

		jl			convRGB24

		add			eax, [DOUBLE_WIDTH]
		add			ebx, [DOUBLE_WIDTH]
		add			ecx, [DOUBLE_WIDTH]
		sub			edx, [NINE_CLIP_WIDTH]
		mov			esi, 0x00
		sub			edi, 0x02
		cmp			edi, 0x00
		jg			convRGB24

		emms
	}
}

static void conv444toRGB24even(unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst)
{
	py += Coded_Picture_Width + CLIP_STEP;
	pu += Coded_Picture_Width + CLIP_STEP;
	pv += Coded_Picture_Width + CLIP_STEP;
	dst += RGB_DOWN2;

	__asm
	{
		mov			eax, [py]
		mov			ebx, [pu]
		mov			ecx, [pv]
		mov			edx, [dst]
		mov			edi, [Clip_Height]
		mov			esi, 0x00
		pxor		mm0, mm0

convRGB24:
		movd		mm1, [eax+esi]
		movd		mm3, [ebx+esi]
		punpcklbw	mm1, mm0
		punpcklbw	mm3, mm0
		movd		mm5, [ecx+esi]
		punpcklbw	mm5, mm0
		movq		mm7, [mmmask_0128]
		psubw		mm3, mm7
		psubw		mm5, mm7

		psubw		mm1, [RGB_Offset]
		movq		mm2, mm1
		movq		mm7, [mmmask_0001]
		punpcklwd	mm1, mm7
		punpckhwd	mm2, mm7
		movq		mm7, [RGB_Scale]
		pmaddwd		mm1, mm7
		pmaddwd		mm2, mm7

		movq		mm4, mm3
		punpcklwd	mm3, mm0
		punpckhwd	mm4, mm0
		movq		mm7, [RGB_CBU]
		pmaddwd		mm3, mm7
		pmaddwd		mm4, mm7
		paddd		mm3, mm1
		paddd		mm4, mm2
		psrld		mm3, 13
		psrld		mm4, 13
		packuswb	mm3, mm0
		packuswb	mm4, mm0

		movq		mm6, mm5
		punpcklwd	mm5, mm0
		punpckhwd	mm6, mm0
		movq		mm7, [RGB_CRV]
		pmaddwd		mm5, mm7
		pmaddwd		mm6, mm7
		paddd		mm5, mm1
		paddd		mm6, mm2
		psrld		mm5, 13
		psrld		mm6, 13
		packuswb	mm5, mm0
		packuswb	mm6, mm0

		punpcklbw	mm3, mm5
		punpcklbw	mm4, mm6
		movq		mm5, mm3
		movq		mm6, mm4
		psrlq		mm5, 16
		psrlq		mm6, 16
		por			mm3, mm5
		por			mm4, mm6

		movd		mm5, [ebx+esi]
		movd		mm6, [ecx+esi]
		punpcklbw	mm5, mm0
		punpcklbw	mm6, mm0
		movq		mm7, [mmmask_0128]
		psubw		mm5, mm7
		psubw		mm6, mm7

		movq		mm7, mm6
		punpcklwd	mm6, mm5
		punpckhwd	mm7, mm5		
		movq		mm5, [RGB_CGX]
		pmaddwd		mm6, mm5
		pmaddwd		mm7, mm5
		paddd		mm6, mm1
		paddd		mm7, mm2

		psrld		mm6, 13
		psrld		mm7, 13
		packuswb	mm6, mm0
		packuswb	mm7, mm0

		punpcklbw	mm3, mm6
		punpcklbw	mm4, mm7

		movq		mm1, mm3
		movq		mm5, mm4
		movq		mm6, mm4

		psrlq		mm1, 32
		psllq		mm1, 24
		por			mm1, mm3

		psrlq		mm3, 40
		psllq		mm6, 16
		por			mm3, mm6
		movd		[edx], mm1

		psrld		mm4, 16
		psrlq		mm5, 24
		por			mm5, mm4
		movd		[edx+4], mm3

		add			edx, 0x0c
		add			esi, 0x04
		cmp			esi, [Clip_Width]
		movd		[edx-4], mm5

		jl			convRGB24

		add			eax, [DOUBLE_WIDTH]
		add			ebx, [DOUBLE_WIDTH]
		add			ecx, [DOUBLE_WIDTH]
		sub			edx, [NINE_CLIP_WIDTH]
		mov			esi, 0x00
		sub			edi, 0x02
		cmp			edi, 0x00
		jg			convRGB24

		emms
	}
}
