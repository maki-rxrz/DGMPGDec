

/*****************************************************************************
 * mcmmx.c : Created by vlad59 - 04/05/2002 
 * Based on Jackey's Form_Prediction code (GetPic.c)
 *****************************************************************************
 * I only made some copy-paste of Jackey's code and some clean up
 *****************************************************************************/

 void MC_put_8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq mm0, [eax]
	movq [ebx], mm0
	add eax, [stride]
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_put_16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq mm0, [eax]
	movq mm1, [eax+8]
	add eax, [stride]
	movq [ebx], mm0
	movq [ebx+8], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

static const __int64 mmmask_0001 = 0x0001000100010001;
static const __int64 mmmask_0002 = 0x0002000200020002;
static const __int64 mmmask_0003 = 0x0003000300030003;
static const __int64 mmmask_0006 = 0x0006000600060006;

 void MC_avg_8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
      movq			mm7, [mmmask_0001]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [ebx]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	movq [ebx], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

 void MC_avg_16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0001]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [ebx]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [ebx+8]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3


	add eax, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_put_x8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0001]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	movq [ebx], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_put_y8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0001]
	mov			eax, [ref]
	mov			ecx, eax
	add			ecx, [offs]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [ecx]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_put_x16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0001]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [eax+9]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

 void MC_put_y16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0001]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [ecx]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [ecx+8]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	psrlw			mm1, 1
	psrlw			mm3, 1

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

 void MC_avg_x8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0003]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ebx]

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	movq			mm6, mm5
	punpcklbw		mm5, mm0
	punpckhbw		mm6, mm0

	psllw			mm5, 1
	psllw			mm6, 1

	paddsw		mm1, mm5
	paddsw		mm3, mm6

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	movq [ebx], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_avg_y8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0003]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [ecx]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ebx]

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	movq			mm6, mm5
	punpcklbw		mm5, mm0
	punpckhbw		mm6, mm0

	psllw			mm5, 1
	psllw			mm6, 1

	paddsw		mm1, mm5
	paddsw		mm3, mm6

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_avg_x16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0003]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ebx]

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	movq			mm6, mm5
	punpcklbw		mm5, mm0
	punpckhbw		mm6, mm0

	psllw			mm5, 1
	psllw			mm6, 1

	paddsw		mm1, mm5
	paddsw		mm3, mm6

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [eax+9]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ebx+8]

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	movq			mm6, mm5
	punpcklbw		mm5, mm0
	punpckhbw		mm6, mm0

	psllw			mm5, 1
	psllw			mm6, 1

	paddsw		mm1, mm5
	paddsw		mm3, mm6

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

 void MC_avg_y16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0003]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [ecx]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ebx]

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	movq			mm6, mm5
	punpcklbw		mm5, mm0
	punpckhbw		mm6, mm0

	psllw			mm5, 1
	psllw			mm6, 1

	paddsw		mm1, mm5
	paddsw		mm3, mm6

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [ecx+8]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ebx+8]

	paddsw		mm1, mm7
	paddsw		mm3, mm7

	movq			mm6, mm5
	punpcklbw		mm5, mm0
	punpckhbw		mm6, mm0

	psllw			mm5, 1
	psllw			mm6, 1

	paddsw		mm1, mm5
	paddsw		mm3, mm6

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]
	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}


// Accurate function
 void MC_put_xy8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0002]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
	
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ecx]
	paddsw		mm1, mm7

	movq			mm6, [ecx+1]
	paddsw		mm3, mm7

	movq			mm2, mm5
	movq			mm4, mm6

	punpcklbw		mm2, mm0
	punpckhbw		mm5, mm0

	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0
					
	paddsw		mm2, mm4
	paddsw		mm5, mm6

	paddsw		mm1, mm2
	paddsw		mm3, mm5

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx], mm1
	add ebx, [stride]

	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

// Accurate function
 void MC_put_xy16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0002]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ecx]
	paddsw		mm1, mm7

	movq			mm6, [ecx+1]
	paddsw		mm3, mm7

	movq			mm2, mm5
	movq			mm4, mm6

	punpcklbw		mm2, mm0
	punpckhbw		mm5, mm0

	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0
					
	paddsw		mm2, mm4
	paddsw		mm5, mm6

	paddsw		mm1, mm2
	paddsw		mm3, mm5

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [eax+9]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ecx+8]
	paddsw		mm1, mm7

	movq			mm6, [ecx+9]
	paddsw		mm3, mm7

	movq			mm2, mm5
	movq			mm4, mm6

	punpcklbw		mm2, mm0
	punpckhbw		mm5, mm0

	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0
					
	paddsw		mm2, mm4
	paddsw		mm5, mm6

	paddsw		mm1, mm2
	paddsw		mm3, mm5

	psrlw			mm1, 2
	psrlw			mm3, 2

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]

	dec edi
	cmp edi, 0x00
	jg mc0

	//emms
}}

// Accurate function
 void MC_avg_xy8_mmx (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0006]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ecx]
	paddsw		mm1, mm7

	movq			mm6, [ecx+1]
	paddsw		mm3, mm7

	movq			mm2, mm5
	movq			mm4, mm6

	punpcklbw		mm2, mm0
	punpckhbw		mm5, mm0

	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0
					
	paddsw		mm2, mm4
	paddsw		mm5, mm6

	paddsw		mm1, mm2
	paddsw		mm3, mm5

	movq			mm6, [ebx]

	movq			mm4, mm6
	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0

	psllw			mm4, 2
	psllw			mm6, 2

	paddsw		mm1, mm4
	paddsw		mm3, mm6

	psrlw			mm1, 3
	psrlw			mm3, 3

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx], mm1
	add ebx, [stride]

	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

// Accurate function
 void MC_avg_xy16_mmx (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{__asm {
	pxor			mm0, mm0
	movq			mm7, [mmmask_0006]
	mov			eax, [ref]
	mov			ebx, [dest]
	mov			ecx, eax
	add			ecx, [offs]
	mov			edi, [height]
mc0:
	movq			mm1, [eax]
	movq			mm2, [eax+1]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ecx]
	paddsw		mm1, mm7

	movq			mm6, [ecx+1]
	paddsw		mm3, mm7

	movq			mm2, mm5
	movq			mm4, mm6

	punpcklbw		mm2, mm0
	punpckhbw		mm5, mm0

	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0
					
	paddsw		mm2, mm4
	paddsw		mm5, mm6

	paddsw		mm1, mm2
	paddsw		mm3, mm5

	movq			mm6, [ebx]

	movq			mm4, mm6
	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0

	psllw			mm4, 2
	psllw			mm6, 2

	paddsw		mm1, mm4
	paddsw		mm3, mm6

	psrlw			mm1, 3
	psrlw			mm3, 3

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	movq [ebx], mm1

	movq			mm1, [eax+8]
	movq			mm2, [eax+9]
	movq			mm3, mm1
	movq			mm4, mm2

	punpcklbw		mm1, mm0
	punpckhbw		mm3, mm0

	punpcklbw		mm2, mm0
	punpckhbw		mm4, mm0

	paddsw		mm1, mm2
	paddsw		mm3, mm4

	movq			mm5, [ecx+8]
	paddsw		mm1, mm7

	movq			mm6, [ecx+9]
	paddsw		mm3, mm7

	movq			mm2, mm5
	movq			mm4, mm6

	punpcklbw		mm2, mm0
	punpckhbw		mm5, mm0

	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0
					
	paddsw		mm2, mm4
	paddsw		mm5, mm6

	paddsw		mm1, mm2
	paddsw		mm3, mm5

	movq			mm6, [ebx+8]

	movq			mm4, mm6
	punpcklbw		mm4, mm0
	punpckhbw		mm6, mm0

	psllw			mm4, 2
	psllw			mm6, 2

	paddsw		mm1, mm4
	paddsw		mm3, mm6

	psrlw			mm1, 3
	psrlw			mm3, 3

	packuswb		mm1, mm0
	packuswb		mm3, mm0

	psllq			mm3, 32
	por			mm1, mm3

	add eax, [stride]
	add ecx, [stride]
	movq [ebx+8], mm1
	add ebx, [stride]

	dec edi
	cmp edi, 0x00
	jg mc0

	//emms

}}

