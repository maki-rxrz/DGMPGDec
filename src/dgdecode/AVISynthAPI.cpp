/* 
 *  Avisynth 2.5 API for MPEG2Dec3
 *
 *  Changes to fix frame dropping and random frame access are
 *  Copyright (C) 2003 Donald A. Graft
 *
 *	Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *	based of the intial MPEG2Dec Avisytnh API Copyright (C) Mathias Born - May 2001
 *
 *  This file is part of MPEG2Dec3, a free MPEG-2 decoder
 *	
 *  MPEG2Dec3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  MPEG2Dec3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "postprocess.h"
#include "AvisynthAPI.h"

//#include <memory.h>
#include <string.h>

MPEG2Source::MPEG2Source(const char* d2v, int cpu, int idct, bool iPP, int moderate_h, int moderate_v, bool showQ, bool fastMC, const char* _cpu2, IScriptEnvironment* env)
{
	CheckCPU();

	#ifdef PROFILING
		init_first(&tim);
	#endif

	/* override */
	m_decoder.refinit = false;
	m_decoder.fpuinit = false;
	m_decoder.luminit = false;

	ovr_idct = idct;
	m_decoder.iPP = iPP;
	m_decoder.showQ = showQ;
	m_decoder.fastMC = fastMC;
	m_decoder.moderate_h = moderate_h;
	m_decoder.moderate_v = moderate_v;

	if (ovr_idct > 7) 
	{
		env->ThrowError("MPEG2Source : iDCT invalid (1:MMX,2:SSEMMX,3:FPU,4:REF;5:SSE2;6:Skal's;7:SimpleiDCT)");
	}

	const char *path = d2v;

	FILE *f;
	if ((f = fopen(path,"r"))==NULL)
		env->ThrowError("MPEG2Source : unable to load D2V file \"%s\" ",path);
	
	fclose(f);

	if (!m_decoder.Open(path, CMPEG2Decoder::YUV422))
		env->ThrowError("MPEG2Source: couldn't open source file, or obsolete D2V file");

	memset(&vi, 0, sizeof(vi));
	vi.width = m_decoder.Clip_Width;
	vi.height = m_decoder.Clip_Height;
	vi.pixel_type = VideoInfo::CS_I420;
	vi.fps_numerator = m_decoder.VF_FrameRate;
	vi.fps_denominator = 1000;
	vi.num_frames = m_decoder.VF_FrameLimit;
//	vi.field_based = false;
	vi.SetFieldBased(false);

	switch (cpu) {
		case 0 : _PP_MODE = 0; break;
		case 1 : _PP_MODE = PP_DEBLOCK_Y_H; break;
		case 2 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
		case 3 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
		case 4 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
		case 5 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
		case 6 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
		default : env->ThrowError("MPEG2Source : cpu level invalid (0 to 6)");
	}

	const char* cpu2 = _cpu2;
	if (strlen(cpu2)==6) {
		_PP_MODE = 0;
		if (cpu2[0]=='x' || cpu2[0] == 'X') { _PP_MODE |= PP_DEBLOCK_Y_H; }
		if (cpu2[1]=='x' || cpu2[1] == 'X') { _PP_MODE |= PP_DEBLOCK_Y_V; }
		if (cpu2[2]=='x' || cpu2[2] == 'X') { _PP_MODE |= PP_DEBLOCK_C_H; }
		if (cpu2[3]=='x' || cpu2[3] == 'X') { _PP_MODE |= PP_DEBLOCK_C_V; }
		if (cpu2[4]=='x' || cpu2[4] == 'X') { _PP_MODE |= PP_DERING_Y; }
		if (cpu2[5]=='x' || cpu2[5] == 'X') { _PP_MODE |= PP_DERING_C; }
	}

	if ( ovr_idct != m_decoder.IDCT_Flag && ovr_idct != -1 )
	{
		dprintf("Overiding iDCT With: %d", ovr_idct);
		override(ovr_idct);
	}

	out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);

	m_decoder.AVSenv = env;
	m_decoder.pp_mode = _PP_MODE;
}

MPEG2Source::~MPEG2Source()
{
	m_decoder.Close();
	aligned_free(out);
}

bool __stdcall MPEG2Source::GetParity(int)
{
	return m_decoder.Field_Order == 1;
}

void MPEG2Source::override(int ovr_idct) 
{
	if (ovr_idct > 0)
		m_decoder.IDCT_Flag = ovr_idct;

	switch (m_decoder.IDCT_Flag)
	{
		case IDCT_MMX:
			m_decoder.idctFunc = MMX_IDCT;
			break;

		case IDCT_SSEMMX:
			m_decoder.idctFunc = SSEMMX_IDCT;
			if (!cpu.ssemmx)
			{
				m_decoder.IDCT_Flag = IDCT_MMX;
				m_decoder.idctFunc = MMX_IDCT;
			}
			break;

		case IDCT_FPU:
			if (!m_decoder.fpuinit) 
			{
				Initialize_FPU_IDCT();
				m_decoder.fpuinit = true;
			}
			m_decoder.idctFunc = FPU_IDCT;
			break;

		case IDCT_REF:
			if (!m_decoder.refinit) 
			{
				Initialize_REF_IDCT();
				m_decoder.refinit = true;
			}
			m_decoder.idctFunc = REF_IDCT;
			break;

		case IDCT_SSE2MMX:
			if (!cpu.sse2mmx)
			{
				m_decoder.IDCT_Flag = IDCT_SSEMMX;
				m_decoder.idctFunc = SSEMMX_IDCT;
				if (!cpu.ssemmx)
				{
					m_decoder.IDCT_Flag = IDCT_MMX;
					m_decoder.idctFunc = MMX_IDCT;
				}
			}
			m_decoder.idctFunc = SSEMMX_IDCT;
			break;
		
		case IDCT_SKALSSE:
			m_decoder.idctFunc = Skl_IDct16_Sparse_SSE; //Skl_IDct16_SSE;
			if (!cpu.ssemmx)
			{
				m_decoder.IDCT_Flag = IDCT_MMX;
				m_decoder.idctFunc = MMX_IDCT;
			}
			break;

		case IDCT_SIMPLEIDCT:
			m_decoder.idctFunc = simple_idct_mmx;
			if (!cpu.ssemmx)
			{
				m_decoder.IDCT_Flag = IDCT_MMX;
				m_decoder.idctFunc = MMX_IDCT;
			}
			break;
	}
}

PVideoFrame __stdcall MPEG2Source::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame frame = env->NewVideoFrame(vi);

	out->y = frame->GetWritePtr(PLANAR_Y);
	out->u = frame->GetWritePtr(PLANAR_U);
	out->v = frame->GetWritePtr(PLANAR_V);
	out->ypitch = frame->GetPitch(PLANAR_Y);
	out->uvpitch = frame->GetPitch(PLANAR_U);

#ifdef PROFILING
	init_timers(&tim);
	start_timer2(&tim.overall);
#endif

	m_decoder.Decode(n, out);

	if ( m_decoder.Luminance_Flag )
	m_decoder.LuminanceFilter(out->y);

#ifdef PROFILING
	stop_timer2(&tim.overall);
	tim.sum += tim.overall;
	tim.div++;
	timer_debug(&tim);
#endif

	__asm emms;

	return frame;
}


LumaFilter::LumaFilter(AVSValue args, IScriptEnvironment* env) : GenericVideoFilter(args[0].AsClip()) 
{
	lumoff = args[1].AsInt(-2);
	lumgain = (int)(args[2].AsFloat(1)*128);
}

static const __int64 m0040w4 = 0x0040004000400040;

PVideoFrame __stdcall LumaFilter::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame cf = child->GetFrame(n,env);
	env->MakeWritable(&cf);
	uc* yplane = cf->GetWritePtr(PLANAR_Y);
	int area = cf->GetHeight(PLANAR_Y)*cf->GetPitch(PLANAR_Y);

	if (lumgain==128 && lumoff<256 && lumoff>-256) { // fast mode (8xbyte SIMD)
		if (lumoff>=0) {
			__declspec(align(8)) const unsigned __int8 
			LumOffsetMask[8] = {lumoff,lumoff,lumoff,lumoff,lumoff,lumoff,lumoff,lumoff};
			__asm
			{
				mov			eax, [yplane]
				mov			ecx, area
				shr			ecx, 3
				pxor		mm0, mm0
				movq		mm7, LumOffsetMask

addlum:
				movq		mm1, [eax]
				paddusb		mm1, mm7
				movq		[eax], mm1
				add				eax,8
				dec			ecx
				jnz			addlum
				emms
			}
		} else {
			__declspec(align(8)) const unsigned __int8 
			LumOffsetMask[8] = {-lumoff,-lumoff,-lumoff,-lumoff,-lumoff,-lumoff,-lumoff,-lumoff};
			__asm
			{
				mov			eax, [yplane]
				mov			ecx, area
				shr			ecx, 3
				pxor		mm0, mm0
				movq		mm7, LumOffsetMask

sublum:
				movq		mm1, [eax]
				psubusb		mm1, mm7
				movq		[eax], mm1
				add			eax,8
				dec			ecx
				jnz			sublum
				emms
			}
		}

	} else {

		__declspec(align(8)) static __int16 
		LumGainMask[4] = {lumgain,lumgain,lumgain,lumgain};
		__declspec(align(8)) static __int16 
		LumOffsetMask[4] = {lumoff,lumoff,lumoff,lumoff};

		__asm
		{
			mov			eax, [yplane]
			mov			ecx, area
			shr			ecx, 3
			pxor		mm0, mm0
			movq		mm5, LumOffsetMask
			movq		mm6, LumGainMask
			movq		mm7, m0040w4
	
lumfilter:
			movq		mm1, [eax]
			movq		mm2, mm1
			punpcklbw	mm1, mm0
			punpckhbw	mm2, mm0
			pmullw		mm1, mm6
			pmullw		mm2, mm6
			paddw		mm1, mm7
			paddw		mm2, mm7
			psrlw		mm1, 7
			psrlw		mm2, 7
			paddw		mm1, mm5
			paddw		mm2, mm5
			packuswb	mm1, mm2
			movq		[eax], mm1
			add			eax,8
			dec			ecx
			jnz			lumfilter
			emms
		}
	}

	return cf;
}

/*

  MPEG2Dec's colorspace convertions Copyright (C) Chia-chen Kuo - April 2001

*/

static const __int64 mmmask_0001 = 0x0001000100010001;
static const __int64 mmmask_0002 = 0x0002000200020002;
static const __int64 mmmask_0003 = 0x0003000300030003;
static const __int64 mmmask_0004 = 0x0004000400040004;
static const __int64 mmmask_0005 = 0x0005000500050005;
static const __int64 mmmask_0007 = 0x0007000700070007;
static const __int64 mmmask_0016 = 0x0010001000100010;
static const __int64 mmmask_0128 = 0x0080008000800080;


void conv420to422(const unsigned char *src, unsigned char *dst, int frame_type, int width, int height)
{
	int INTERLACED_HEIGHT = (height>>2) - 2;
	int PROGRESSIVE_HEIGHT = (height>>1) - 2;
	int DOUBLE_WIDTH = width<<1;
	int HALF_WIDTH = width>>1;

	if (frame_type)
	{
		__asm
		{
			mov			eax, [src]
			mov			ebx, [dst]
			mov			ecx, ebx
			add			ecx, HALF_WIDTH
			mov			esi, 0x00
			movq		mm3, [mmmask_0003]
			pxor		mm0, mm0
			movq		mm4, [mmmask_0002]

			mov			edx, eax
			add			edx, HALF_WIDTH
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
			cmp			esi, HALF_WIDTH
			movd		[ecx+esi-4], mm2
			jl			convyuv422topp

			add			eax, HALF_WIDTH
			add			ebx, width
			add			ecx, width
			mov			esi, 0x00

			mov			edi, PROGRESSIVE_HEIGHT
convyuv422p:
			movd		mm1, [eax+esi]

			punpcklbw	mm1, mm0
			mov			edx, eax

			pmullw		mm1, mm3
			sub			edx, HALF_WIDTH

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
			add			edx, HALF_WIDTH
			add			esi, 0x04
			cmp			esi, HALF_WIDTH
			movd		[ebx+esi-4], mm5
			movd		[ecx+esi-4], mm2

			jl			convyuv422p

			add			eax, HALF_WIDTH
			add			ebx, width
			add			ecx, width
			mov			esi, 0x00
			dec			edi
			cmp			edi, 0x00
			jg			convyuv422p

			mov			edx, eax
			sub			edx, HALF_WIDTH
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
			cmp			esi, HALF_WIDTH
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
			add			ebx, HALF_WIDTH
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
			add			edx, HALF_WIDTH
			pmullw		mm6, mm5
			movd		[edx+esi], mm2

			add			ebx, HALF_WIDTH
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			pmullw		mm2, mm3
			paddusw		mm2, mm6
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, HALF_WIDTH
			add			ebx, HALF_WIDTH
			pmullw		mm7, [mmmask_0007]
			movd		[edx+esi], mm2

			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			paddusw		mm2, mm7
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, HALF_WIDTH
			add			esi, 0x04
			cmp			esi, HALF_WIDTH
			movd		[edx+esi-4], mm2

			jl			convyuv422topi

			add			eax, width
			add			ecx, DOUBLE_WIDTH
			mov			esi, 0x00

			mov			edi, INTERLACED_HEIGHT
convyuv422i:
			movd		mm1, [eax+esi]
			punpcklbw	mm1, mm0
			movq		mm6, mm1
			mov			ebx, eax
			sub			ebx, width
			movd		mm3, [ebx+esi]
			pmullw		mm1, [mmmask_0007]
			punpcklbw	mm3, mm0
			paddusw		mm3, mm1
			paddusw		mm3, mm4
			psrlw		mm3, 0x03
			packuswb	mm3, mm0

			add			ebx, HALF_WIDTH
			movq		mm1, [ebx+esi]
			add			ebx, width
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
			add			edx, HALF_WIDTH
			movd		[edx+esi], mm2

			add			ebx, HALF_WIDTH
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			pmullw		mm2, mm3
			paddusw		mm2, mm6
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			pmullw		mm7, [mmmask_0007]
			add			edx, HALF_WIDTH
			add			ebx, HALF_WIDTH
 			movd		[edx+esi], mm2

			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			paddusw		mm2, mm7
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			add			edx, HALF_WIDTH
			add			esi, 0x04
			cmp			esi, HALF_WIDTH
			movd		[edx+esi-4], mm2

			jl			convyuv422i
			add			eax, width
			add			ecx, DOUBLE_WIDTH
			mov			esi, 0x00
			dec			edi
			cmp			edi, 0x00
			jg			convyuv422i

convyuv422bottomi:
			movd		mm1, [eax+esi]
			movq		mm6, mm1
			punpcklbw	mm1, mm0
			mov			ebx, eax
			sub			ebx, width
			movd		mm3, [ebx+esi]
			punpcklbw	mm3, mm0
			pmullw		mm1, [mmmask_0007]
			paddusw		mm3, mm1
			paddusw		mm3, mm4
			psrlw		mm3, 0x03
			packuswb	mm3, mm0

			add			ebx, HALF_WIDTH
			movq		mm1, [ebx+esi]
			punpcklbw	mm1, mm0
			movd		[ecx+esi], mm3

			pmullw		mm1, [mmmask_0003]
			add			ebx, width
			movd		mm2, [ebx+esi]
			punpcklbw	mm2, mm0
			movq		mm7, mm2
			pmullw		mm2, mm5
			paddusw		mm2, mm1
			paddusw		mm2, mm4
			psrlw		mm2, 0x03
			packuswb	mm2, mm0

			mov			edx, ecx
			add			edx, HALF_WIDTH
			pmullw		mm7, [mmmask_0007]
			movd		[edx+esi], mm2

			add			edx, HALF_WIDTH
 			movd		[edx+esi], mm6

			punpcklbw	mm6, mm0
			paddusw		mm6, mm7
			paddusw		mm6, mm4
			psrlw		mm6, 0x03
			packuswb	mm6, mm0

			add			edx, HALF_WIDTH
			add			esi, 0x04
			cmp			esi, HALF_WIDTH
			movd		[edx+esi-4], mm6

			jl			convyuv422bottomi
		}
	}
}

void conv422to444(const unsigned char *src, unsigned char *dst, int width, int height)
{
	int HALF_WIDTH = (width/2);
	int HALF_WIDTH_D8 = (width/2)-8;
	
	__asm
	{
		mov			eax, [src]
		mov			ebx, [dst]
		mov			edi, height

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
		cmp			esi, HALF_WIDTH_D8
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

		add			eax, HALF_WIDTH
		add			ebx, width
		dec			edi
		cmp			edi, 0x00
		jg			convyuv444init
	}
}


void conv444toRGB24(const unsigned char *py, const unsigned char *pu, const unsigned char *pv, unsigned char *dst, int pitch, int width, int height, YUVRGBScale* scale)
{
	int PWIDTH = 0; //- width *2; 
	__int64 RGB_Offset = scale->RGB_Offset;
	__int64 RGB_Scale = scale->RGB_Scale;
	__int64 RGB_CBU = scale->RGB_CBU;
	__int64 RGB_CRV = scale->RGB_CRV;
	__int64 RGB_CGX = scale->RGB_CGX;

	__asm
	{
		mov			eax, [py]
		mov			ebx, [pu]
		mov			ecx, [pv]
		mov			edx, [dst]
		mov			edi, height
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

		psubw		mm1, RGB_Offset
		movq		mm2, mm1
		movq		mm7, [mmmask_0001]
		punpcklwd	mm1, mm7
		punpckhwd	mm2, mm7
		movq		mm7, RGB_Scale
		pmaddwd		mm1, mm7
		pmaddwd		mm2, mm7

		movq		mm4, mm3
		punpcklwd	mm3, mm0
		punpckhwd	mm4, mm0
		movq		mm7, RGB_CBU
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
		movq		mm7, RGB_CRV
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
		movq		mm5, RGB_CGX
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
		cmp			esi, width
		movd		[edx-4], mm5

		jl			convRGB24

		add			eax, width
		add			ebx, width
		add			ecx, width
		add			edx, PWIDTH
		mov			esi, 0x00
		dec			edi
		cmp			edi, 0x00
		jg			convRGB24

		emms
	}
}

void conv422toYUV422(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, int pitch, int width, int height)
{
	int y = height;
	int Clip_Width_2 = width / 2;
	int Coded_Picture_Width = width;
	int Coded_Picture_Width_2 = width / 2;

	__asm
	{
		emms
		mov			eax, [py]
		mov			ebx, [pu]
		mov			ecx, [pv]
		mov			edx, [dst]
		mov			edi, Clip_Width_2
	yloop:
		xor			esi, esi
	xloop:
		movd		mm1, [eax+esi*2]		;0000YYYY
		movd		mm2, [ebx+esi]			;0000UUUU
		movd		mm3, [ecx+esi]			;0000VVVV
		;interleave this to VYUYVYUY
		punpcklbw	mm2, mm3				;VUVUVUVU
		punpcklbw	mm1, mm2				;VYUYVYUY
		movq		[edx+esi*4], mm1
		movd		mm1, [eax+esi*2+4]		;0000YYYY
		punpckhdq	mm2, mm2				;xxxxVUVU
		punpcklbw	mm1, mm2				;VYUYVYUY
		movq		[edx+esi*4+8], mm1
		add			esi, 4
		cmp			esi, edi
		jb			xloop
		add			edx, pitch
		add			eax, Coded_Picture_Width
		add			ebx, Coded_Picture_Width_2
		add			ecx, Coded_Picture_Width_2
		dec			y
		jnz			yloop

		emms
	}
}

/*
	End of MPEG2Dec's colorspace convertions 
*/

YV12toRGB24::YV12toRGB24(AVSValue args, IScriptEnvironment* env) : GenericVideoFilter(args[0].AsClip()) 
{
	TVscale = args[2].AsBool(false);
	if (!TVscale)
	{
		cscale.RGB_Scale  = 0x1000254310002543;
		cscale.RGB_Offset = 0x0010001000100010;
		cscale.RGB_CBU    = 0x0000408D0000408D;
		cscale.RGB_CGX    = 0xF377E5FCF377E5FC;
		cscale.RGB_CRV    = 0x0000331300003313;
	}
	else
	{
		cscale.RGB_Scale  = 0x1000200010002000;
		cscale.RGB_Offset = 0x0000000000000000;
		cscale.RGB_CBU    = 0x000038B4000038B4;
		cscale.RGB_CGX    = 0xF4FDE926F4FDE926;
		cscale.RGB_CRV    = 0x00002CDD00002CDD;
	}

	if (!vi.IsYV12()) // BAD !!
		env->ThrowError("YV12toRGB24 : need YV12 input");
	vi.pixel_type = VideoInfo::CS_BGR24;

	u422 = (unsigned char*)aligned_malloc(vi.width * vi.height/2, 128);
	v422 = (unsigned char*)aligned_malloc(vi.width * vi.height/2, 128);
	u444 = (unsigned char*)aligned_malloc(vi.width * vi.height, 128);
	v444 = (unsigned char*)aligned_malloc(vi.width * vi.height, 128);
	interlaced = args[1].AsBool(true);
}

YV12toRGB24::~YV12toRGB24() 
{
	aligned_free(u422);
	aligned_free(v422);
	aligned_free(u444);
	aligned_free(v444);
}
    
PVideoFrame __stdcall YV12toRGB24::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame src = child->GetFrame(n,env);
	PVideoFrame dst = env->NewVideoFrame(vi);

	conv420to422(src->GetReadPtr(PLANAR_U), u422, !interlaced, vi.width, vi.height);
	conv422to444(u422, u444, vi.width, vi.height);
	conv420to422(src->GetReadPtr(PLANAR_V), v422, !interlaced, vi.width, vi.height);
	conv422to444(v422, v444, vi.width, vi.height);
	conv444toRGB24(src->GetReadPtr(PLANAR_Y), u444, v444, dst->GetWritePtr(), dst->GetPitch(),vi.width,vi.height,&cscale);

	return dst;
}

YV12toYUY2::YV12toYUY2(AVSValue args, IScriptEnvironment* env) : GenericVideoFilter(args[0].AsClip()) 
{
	if (!vi.IsYV12()) // BAD !!
		env->ThrowError("YV12toYUY2 : need YV12 input");
	vi.pixel_type = VideoInfo::CS_YUY2;
	u422 = (unsigned char*)aligned_malloc(vi.width * vi.height/2, 128);
	v422 = (unsigned char*)aligned_malloc(vi.width * vi.height/2, 128);
	interlaced = args[1].AsBool(true);
}

YV12toYUY2::~YV12toYUY2() 
{
	aligned_free(u422);
	aligned_free(v422);
}

PVideoFrame __stdcall YV12toYUY2::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame src = child->GetFrame(n,env);
	PVideoFrame dst = env->NewVideoFrame(vi);

	conv420to422(src->GetReadPtr(PLANAR_U), u422, !interlaced, vi.width, vi.height);
	conv420to422(src->GetReadPtr(PLANAR_V), v422, !interlaced, vi.width, vi.height);
	conv422toYUV422(src->GetReadPtr(PLANAR_Y), u422, v422, dst->GetWritePtr(), dst->GetPitch(),vi.width,vi.height);

	return dst;
}

BlindPP::BlindPP(AVSValue args, IScriptEnvironment* env)  : GenericVideoFilter(args[0].AsClip()) 
{
	int quant = args[1].AsInt(2);
	if (vi.width%16!=0)
		env->ThrowError("BlindPP : Need mod16 width");
	if (vi.height%16!=0)
		env->ThrowError("BlindPP : Need mod16 height");
	if (!vi.IsYV12())
		env->ThrowError("BlindPP : Work in YV12 colorspace");

	QP = new int[vi.width*vi.height/256];
	for (int i=0;i<vi.width*vi.height/256;i++)
		QP[i] = quant;

	switch (args[2].AsInt(6)) {
		case 0 : PP_MODE = 0; break;
		case 1 : PP_MODE = PP_DEBLOCK_Y_H; break;
		case 2 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
		case 3 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
		case 4 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
		case 5 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
		case 6 : PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
		default : env->ThrowError("BlindPP : cpu level invalid (0 to 6)");
	}

	const char* cpu2 = args[3].AsString("");
	if (strlen(cpu2)==6) {
		PP_MODE = 0;
		if (cpu2[0]=='x' || cpu2[0]=='x') { PP_MODE |= PP_DEBLOCK_Y_H; }
		if (cpu2[1]=='x' || cpu2[1]=='x') { PP_MODE |= PP_DEBLOCK_Y_V; }
		if (cpu2[2]=='x' || cpu2[2]=='x') { PP_MODE |= PP_DEBLOCK_C_H; }
		if (cpu2[3]=='x' || cpu2[3]=='x') { PP_MODE |= PP_DEBLOCK_C_V; }
		if (cpu2[4]=='x' || cpu2[4]=='x') { PP_MODE |= PP_DERING_Y; }
		if (cpu2[5]=='x' || cpu2[5]=='x') { PP_MODE |= PP_DERING_C; }
	}
	iPP = args[4].AsBool(false);
	PP_MODE |= PP_DONT_COPY;
	moderate_h = args[5].AsInt(20);
	moderate_v = args[6].AsInt(40);
}

PVideoFrame __stdcall BlindPP::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame cf = child->GetFrame(n,env);
	env->MakeWritable(&cf);

	uc* src[3];

	src[0] = cf->GetWritePtr(PLANAR_Y);
	src[1] = cf->GetWritePtr(PLANAR_U);
	src[2] = cf->GetWritePtr(PLANAR_V);

	if (iPP) {	// Field Based PP
		postprocess(src, 0, src, cf->GetPitch()*2, vi.width*2, vi.height/2, QP, vi.width/16, PP_MODE, moderate_h, moderate_v);
	} else {	// Image Based PP
		postprocess(src, 0, src, cf->GetPitch(), vi.width, vi.height, QP, vi.width/16, PP_MODE, moderate_h, moderate_v);
	}
	return cf;
}

BlindPP::~BlindPP() { delete[] QP; }


AVSValue __cdecl Create_MPEG2Source(AVSValue args, void*, IScriptEnvironment* env) 
{
//	char path[1024];
	char buf[80], *p;

	char d2v[255];
	int cpu = 0;
	int idct = -1;
	bool iPP = false;
	int moderate_h = 20;
	int moderate_v = 40;
	bool showQ = false;
	bool fastMC = false;
	char cpu2[255];

	/* Based on D.Graft Msharpen default files code */
	/* Load user defaults if they exist. */ 
#define LOADINT(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
	p = buf; \
	while(*p++ != '='); \
	var = atoi(p); \
}

#define LOADSTR(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
	p = buf; \
	while(*p++ != '='); \
	char* dst = var; \
	int i=0; \
	if (*p++ == '"') while('"' != *p++) i++; \
	var[i] = 0; \
	strncpy(var,(p-i-1),i); \
}

#define LOADBOOL(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
	p = buf; \
	while(*p++ != '='); \
	if (*p == 't') var = true; \
	else var = false; \
}
	try
	{
		FILE *f;

	//	no autoloading in avisynth 2.5 yet.

	//	const char* plugin_dir = env->GetVar("$PluginDir$").AsString();
	//	strcpy(path, plugin_dir);
	//	strcat(path, "\\MPEG2Dec3.def");
	//	if ((f = fopen(path, "r")) != NULL)

		if ((f = fopen("MPEG2Dec3.def", "r")) != NULL)
		{
			while(fgets(buf, 80, f) != 0)
			{
				LOADSTR(d2v,"d2v=",4)
				LOADINT(cpu,"cpu=",4)
				LOADINT(idct,"idct=",5)
				LOADBOOL(iPP,"iPP=",4)
				LOADINT(moderate_h,"moderate_h=",11)
				LOADINT(moderate_v,"moderate_v=",11)
				LOADBOOL(showQ,"showQ=",6)
				LOADBOOL(fastMC,"fastMC=",7)
				LOADSTR(cpu2,"cpu2=",5)
			}
		}
	}
	catch (...)
	{
		// plugin directory not set
		// probably using an older version avisynth
	}

	// check for uninitialised strings
	if (strlen(d2v)>=255) d2v[0]=0;
//		env->ThrowError("MPEG2Dec3 : Need d2v project file path");
	if (strlen(cpu2)>=255) cpu2[0]=0;

	MPEG2Source *dec = new MPEG2Source( args[0].AsString(d2v),
										args[1].AsInt(cpu),
										args[2].AsInt(idct),
										args[3].AsBool(iPP),
										args[4].AsInt(moderate_h),
										args[5].AsInt(moderate_v),
										args[6].AsBool(showQ),
										args[7].AsBool(fastMC),
										args[8].AsString(cpu2),
										env );
	// Only bother invokeing crop if we have to
	if ( dec->m_decoder.Clip_Top    || 
		 dec->m_decoder.Clip_Bottom || 
		 dec->m_decoder.Clip_Left   || 
		 dec->m_decoder.Clip_Right )
	{
		AVSValue CropArgs[5] = { dec,
								 dec->m_decoder.Clip_Left, 
								 dec->m_decoder.Clip_Top, 
								 -dec->m_decoder.Clip_Right,
								 -dec->m_decoder.Clip_Bottom };

		return env->Invoke("crop",AVSValue(CropArgs,5));
	}

	return dec;
}

AVSValue __cdecl Create_LumaFilter(AVSValue args, void* user_data, IScriptEnvironment* env) 
{	
	return new LumaFilter(args,env);	
}

AVSValue __cdecl Create_YV12toRGB24(AVSValue args, void* user_data, IScriptEnvironment* env) 
{	
	return new YV12toRGB24(args, env);	
}

AVSValue __cdecl Create_YV12toYUY2(AVSValue args, void* user_data, IScriptEnvironment* env) 
{	
	return new YV12toYUY2(args, env);	
}


AVSValue __cdecl Create_BlindPP(AVSValue args, void* user_data, IScriptEnvironment* env) 
{	
	return new BlindPP(args,env);	
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("MPEG2Source", "[d2v]s[cpu]i[idct]i[iPP]b[moderate_h]i[moderate_v]i[showQ]b[fastMC]b[cpu2]s", Create_MPEG2Source, 0);
    env->AddFunction("LumaFilter", "c[lumoff]i[lumgain]f", Create_LumaFilter, 0);
    env->AddFunction("YV12toRGB24", "c[interlaced]b[TVscale]b", Create_YV12toRGB24, 0);
    env->AddFunction("YV12toYUY2", "c[interlaced]b", Create_YV12toYUY2, 0);
    env->AddFunction("BlindPP", "c[quant]i[cpu]i[cpu2]s[iPP]b[moderate_h]i[moderate_v]i", Create_BlindPP, 0);
    return 0;
}

// Code for backward compatibility and Gordian Knot support
// (running MPEG2DEC3 functions without Avisynth).
YUVRGBScale ext_cscale;
IScriptEnvironment *DLLEnv = NULL;
MPEG2Source* vf = NULL;
unsigned char* buffer = NULL;
unsigned char* picture = NULL;
unsigned char* u422 = NULL, *u444 = NULL;
unsigned char* v422 = NULL, *v444 = NULL;

VideoInfo pvi;
int pitch = 0;

MPEG2Source::MPEG2Source(const char* d2v)
{
	int cpu = 0;
	int idct = -1;
	bool iPP = false;
	int moderate_h = 20;
	int moderate_v = 40;
	bool showQ = false;
	bool fastMC = false;

	unsigned char* buffer = NULL;
	unsigned char* picture = NULL;
	unsigned char* u422 = NULL, *u444 = NULL;
	unsigned char* v422 = NULL, *v444 = NULL;

	CheckCPU();

	#ifdef PROFILING
		init_first(&tim);
	#endif

	/* override */
	m_decoder.refinit = false;
	m_decoder.fpuinit = false;
	m_decoder.luminit = false;

	ovr_idct = idct;
	m_decoder.iPP = iPP;
	m_decoder.showQ = showQ;
	m_decoder.fastMC = fastMC;

	if (ovr_idct > 7) 
	{
		dprintf("MPEG2Source : iDCT invalid (1:MMX,2:SSEMMX,3:FPU,4:REF;5:SSE2;6:Skal's;7:SimpleiDCT)");
		return;
	}

	const char *path = d2v;

	FILE *f;
	if ((f = fopen(path,"r"))==NULL)
	{
		dprintf("MPEG2Source : unable to load file \"%s\" ",path);
		return;
	}
	
	fclose(f);

	if (!m_decoder.Open(path, CMPEG2Decoder::YUV422))
	{
		dprintf("MPEG2Source: couldn't open file");
		return;
	}

	memset(&vi, 0, sizeof(vi));
	vi.width = m_decoder.Clip_Width;
	vi.height = m_decoder.Clip_Height;
	vi.pixel_type = VideoInfo::CS_I420;
	vi.fps_numerator = m_decoder.VF_FrameRate;
	vi.fps_denominator = 1000;
	vi.num_frames = m_decoder.VF_FrameLimit;
//	vi.field_based = false;
	vi.SetFieldBased(false);

	switch (cpu) {
		case 0 : _PP_MODE = 0; break;
		case 1 : _PP_MODE = PP_DEBLOCK_Y_H; break;
		case 2 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
		case 3 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
		case 4 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
		case 5 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
		case 6 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
		default : dprintf("MPEG2Source : cpu level invalid (0 to 6)"); return;
	}

	if ( ovr_idct != m_decoder.IDCT_Flag && ovr_idct != -1 )
	{
		dprintf("Overiding iDCT With: %d", ovr_idct);
		override(ovr_idct);
	}

	out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);
}

int PF;

void MPEG2Source::GetFrame(int n, unsigned char *buffer, int pitch)
{
	out->y = buffer;
	out->u = out->y + (pitch * pvi.height);
	out->v = out->u + ((pitch * pvi.height)/4);
	out->ypitch = pitch;
	out->uvpitch = pitch/2;

	m_decoder.Decode(n, out);

	PF = out->pf;

	__asm emms;
}

extern "C" __declspec(dllexport) VideoInfo* __cdecl openMPEG2Source(char* file)
{
//dprintf("openMPEG2Source\n");
	ext_cscale.RGB_Scale  = 0x1000254310002543;
	ext_cscale.RGB_Offset = 0x0010001000100010;
	ext_cscale.RGB_CBU    = 0x0000408D0000408D;
	ext_cscale.RGB_CGX    = 0xF377E5FCF377E5FC;
	ext_cscale.RGB_CRV    = 0x0000331300003313;

	vf = new MPEG2Source(file);
	pvi = vf->GetVideoInfo();
	buffer = (unsigned char*)malloc(pvi.height * pvi.width * 2);
//dprintf("buffer %p\n", buffer);

	u422 = (unsigned char*)malloc(pvi.height * pvi.width);
	u444 = (unsigned char*)malloc(pvi.height * pvi.width);

	v422 = (unsigned char*)malloc(pvi.height * pvi.width);
	v444 = (unsigned char*)malloc(pvi.height * pvi.width);

	pitch = pvi.width;
	
	return &pvi;
}

extern "C" __declspec(dllexport) unsigned char* __cdecl getFrame(int frame)
{
//dprintf("getFrame\n");
	vf->GetFrame(frame, buffer, pitch);
	return buffer;
}

extern "C" __declspec(dllexport) unsigned char* __cdecl getRGBFrame(int frame)
{
#if 0
	if (PF == 1)
		OutputDebugString("PF=1\n");
	else
		OutputDebugString("PF=0\n");
#endif

//dprintf("getRGBFrame\n");
//dprintf("frame %x\n", frame);
//dprintf("buffer %p\n", buffer);
	vf->GetFrame(frame, buffer, pitch);

	if (picture == NULL) picture = (unsigned char*)malloc(pvi.height * pvi.width * 3);

	unsigned char *y = buffer;
	unsigned char *u = y + (pitch * pvi.height);
	unsigned char *v = u + ((pitch * pvi.height)/4);

	conv420to422(u, u422, PF, pvi.width, pvi.height);
	conv422to444(u422, u444, pvi.width, pvi.height);
	conv420to422(v, v422, PF, pvi.width, pvi.height);
	conv422to444(v422, v444, pvi.width, pvi.height);
	conv444toRGB24(y, u444, v444, picture, pvi.width * 2, pvi.width, pvi.height, &ext_cscale);

	return picture;
}

extern "C" __declspec(dllexport) void __cdecl closeVideo(void)
{
//dprintf("closeVideo\n");
	if (buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
//dprintf("free buffer %p\n", buffer);
	}
	
	if (picture != NULL)
	{
		free(picture);
		picture = NULL;
	}

	if (vf != NULL)
	{
		delete vf;
		vf = NULL;
	}

	if (u422) free(u422);
	if (u444) free(u444);
	if (v422) free(v422);
	if (v444) free(v444);
	u422 = u444 = v422 = v444 = NULL;
}
