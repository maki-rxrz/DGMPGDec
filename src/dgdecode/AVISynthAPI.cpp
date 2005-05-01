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
 *  This file is part of DGMPGDec, a free MPEG-2 decoder
 *	
 *  DGMPGDec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DGMPGDec is distributed in the hope that it will be useful,
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
#include "utilities.h"
#include <string.h>

#define VERSION "DGDecode 1.3.0"

MPEG2Source::MPEG2Source(const char* d2v, int cpu, int idct, int iPP, int moderate_h, int moderate_v, bool showQ, bool fastMC, const char* _cpu2, int _info, bool _upConv, bool _i420, IScriptEnvironment* env)
{
	CheckCPU();

	#ifdef PROFILING
		init_first(&tim);
	#endif

	/* override */
	m_decoder.refinit = false;
	m_decoder.fpuinit = false;
	m_decoder.luminit = false;

	if (iPP != -1 && iPP != 0 && iPP != 1)
		env->ThrowError("MPEG2Source : iPP must be set to -1, 0, or 1!");

	ovr_idct = idct;
	m_decoder.iPP = iPP;
	m_decoder.info = _info;
	m_decoder.showQ = showQ;
	m_decoder.fastMC = fastMC;
	m_decoder.upConv = _upConv;
	m_decoder.i420 = _i420;
	m_decoder.moderate_h = moderate_h;
	m_decoder.moderate_v = moderate_v;
	m_decoder.maxquant = m_decoder.minquant = m_decoder.avgquant = 0;

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
	if (m_decoder.chroma_format == 1 && !_upConv)
	{
		if (m_decoder.i420 == true)
			vi.pixel_type = VideoInfo::CS_I420;
		else
			vi.pixel_type = VideoInfo::CS_YV12;
	}
	else if (m_decoder.chroma_format == 1 && _upConv) vi.pixel_type = VideoInfo::CS_YUY2;
	else if (m_decoder.chroma_format == 2) vi.pixel_type = VideoInfo::CS_YUY2;
	else env->ThrowError("MPEG2Source:  currently unsupported input color format (4:4:4)");
	vi.fps_numerator = m_decoder.VF_FrameRate;
	vi.fps_denominator = 1000;
	vi.num_frames = m_decoder.VF_FrameLimit;
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

	out = NULL;
	out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);
	if (out == NULL) env->ThrowError("MPEG2Source:  malloc failure (yv12 pic out)!");

	m_decoder.AVSenv = env;
	m_decoder.pp_mode = _PP_MODE;

	bufY = bufU = bufV = NULL;
	if (m_decoder.chroma_format != 1 || (m_decoder.chroma_format == 1 && _upConv))
	{
		bufY = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,128);
		bufU = (unsigned char*)aligned_malloc(m_decoder.Chroma_Width*vi.height+2048,128);
		bufV =  (unsigned char*)aligned_malloc(m_decoder.Chroma_Width*vi.height+2048,128);
		if (bufY == NULL || bufU == NULL || bufV == NULL)
			env->ThrowError("MPEG2Source:  malloc failure (bufY, bufU, bufV)!");
	}
}

MPEG2Source::~MPEG2Source()
{
	m_decoder.Close();
	if (out != NULL) { aligned_free(out); out = NULL; }
	if (bufY != NULL) { aligned_free(bufY); bufY = NULL; }
	if (bufU != NULL) { aligned_free(bufU); bufU = NULL; }
	if (bufV != NULL) { aligned_free(bufV); bufV = NULL; }
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
	int gop, pct;
	char Matrix_s[40];
	unsigned int raw;
	unsigned int hint;

    PVideoFrame frame = env->NewVideoFrame(vi);

	if (m_decoder.chroma_format == 1 && !m_decoder.upConv) // YV12
	{
		out->y = frame->GetWritePtr(PLANAR_Y);
		out->u = frame->GetWritePtr(PLANAR_U);
		out->v = frame->GetWritePtr(PLANAR_V);
		out->ypitch = frame->GetPitch(PLANAR_Y);
		out->uvpitch = frame->GetPitch(PLANAR_U);
	}
	else // its 4:2:2, pass our own buffers
	{
		out->y = bufY;
		out->u = bufU;
		out->v = bufV;
		out->ypitch = vi.width;
		out->uvpitch = m_decoder.Chroma_Width;
	}

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

	if (m_decoder.chroma_format != 1 ||
		(m_decoder.chroma_format == 1 && m_decoder.upConv)) // convert 4:2:2 (planar) to YUY2 (packed)
	{
		conv422toYUV422_2(out->y,out->u,out->v,frame->GetWritePtr(),out->ypitch,out->uvpitch,
			frame->GetPitch(),vi.width,vi.height);
	}

	if (m_decoder.info != 0)
	{
		raw = max(m_decoder.FrameList[n].bottom, m_decoder.FrameList[n].top);
		if (raw < (int)m_decoder.BadStartingFrames) raw = m_decoder.BadStartingFrames;

		for (gop = 0; gop < (int) m_decoder.VF_GOPLimit - 1; gop++)
		{
			if (raw >= (int)m_decoder.GOPList[gop]->number && raw < (int)m_decoder.GOPList[gop+1]->number)
			{
				break;
			}
		}
		pct = m_decoder.FrameList[raw].pct == I_TYPE ? 'I' : m_decoder.FrameList[raw].pct == B_TYPE ? 'B' : 'P';

		switch (m_decoder.GOPList[gop]->matrix)
		{
		case 0:
			strcpy(Matrix_s, "Forbidden");
			break;
		case 1:
			strcpy(Matrix_s, "ITU-R BT.709");
			break;
		case 2:
			strcpy(Matrix_s, "Unspecified Video");
			break;
		case 3:
			strcpy(Matrix_s, "Reserved");
			break;
		case 4:
			strcpy(Matrix_s, "FCC");
			break;
		case 5:
			strcpy(Matrix_s, "ITU-R BT.470-2 System B, G");
			break;
		case 6:
			strcpy(Matrix_s, "SMPTE 170M");
			break;
		case 7:
			strcpy(Matrix_s, "SMPTE 240M (1987)");
			break;
		default:
			strcpy(Matrix_s, "Reserved");
			break;
		}
	}

	if (m_decoder.info == 1)
	{
		char msg1[1024];
		sprintf(msg1,"%s (c) 2005 Donald A. Graft\n" \
					 "---------------------------------------\n" \
					 "Source:        %s\n" \
					 "Frame Rate:    %3.3f fps %s\n" \
			         "Field Order:   %s\n" \
			         "Picture Size:  %d x %d\n" \
 					 "Aspect Ratio:  %s\n"  \
					 "Progr Seq:     %s\n" \
					 "GOP Number:    %d (%d)  GOP Pos = %I64d\n" \
					 "Closed GOP:    %s\n" \
					 "Display Frame: %d\n" \
					 "Encoded Frame: %d (top) %d (bottom)\n" \
 					 "Frame Type:    %c (%d)\n" \
					 "Progr Frame:   %s\n" \
 					 "Colorimetry:   %s (%d)\n" \
 					 "Quants:        %d/%d/%d (avg/min/max)\n",
		VERSION,
 		m_decoder.Infilename[m_decoder.GOPList[gop]->file],
		m_decoder.VF_FrameRate / 1000.0f, m_decoder.VF_FrameRate == 25000 ? "(PAL)" : m_decoder.VF_FrameRate == 29970 ? "(NTSC)" : "", 
		m_decoder.Field_Order == 1 ? "Top Field First" : "Bottom Field First",
		m_decoder.Coded_Picture_Width, m_decoder.Coded_Picture_Height,
		m_decoder.Aspect_Ratio,
		m_decoder.GOPList[gop]->progressive ? "True" : "False",
		gop, m_decoder.GOPList[gop]->number, m_decoder.GOPList[gop]->position,
		m_decoder.GOPList[gop]->closed ? "True" : "False",
		n,
		m_decoder.FrameList[n].top, m_decoder.FrameList[n].bottom,
		pct, raw,
		m_decoder.FrameList[raw].pf ? "True" : "False",
		Matrix_s, m_decoder.GOPList[gop]->matrix,
		m_decoder.avgquant, m_decoder.minquant, m_decoder.maxquant);
		ApplyMessage(&frame, vi, msg1, 150, 0xdfffbf, 0x0, 0x0, env);
	}
	else if (m_decoder.info == 2)
	{
		dprintf("DGDecode: DGDecode %s (c) 2005 Donald A. Graft\n", VERSION);
		dprintf("DGDecode: Source:            %s\n", m_decoder.Infilename[m_decoder.GOPList[gop]->file]);
		dprintf("DGDecode: Frame Rate:        %3.3f fps %s\n", m_decoder.VF_FrameRate / 1000.0f, m_decoder.VF_FrameRate == 25000 ? "(PAL)" : m_decoder.VF_FrameRate == 29970 ? "(NTSC)" : "");
		dprintf("DGDecode: Field Order:       %s\n", m_decoder.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
		dprintf("DGDecode: Picture Size:      %d x %d\n", m_decoder.Coded_Picture_Width, m_decoder.Coded_Picture_Height);
		dprintf("DGDecode: Aspect Ratio:      %s\n", m_decoder.Aspect_Ratio);
		dprintf("DGDecode: Progressive Seq:   %s\n", m_decoder.GOPList[gop]->progressive ? "True" : "False");
		dprintf("DGDecode: GOP Number:        %d (%d)  GOP Pos = %I64d\n", gop, m_decoder.GOPList[gop]->number, m_decoder.GOPList[gop]->position);
		dprintf("DGDecode: Closed GOP:        %s\n", m_decoder.GOPList[gop]->closed ? "True" : "False");
		dprintf("DGDecode: Display Frame:     %d\n", n);
		dprintf("DGDecode: Encoded Frame:     %d (top) %d (bottom)\n", m_decoder.FrameList[n].top, m_decoder.FrameList[n].bottom);
		dprintf("DGDecode: Frame Type:        %c (%d)\n", pct, raw);
		dprintf("DGDecode: Progressive Frame: %s\n", m_decoder.FrameList[raw].pf ? "True" : "False");
		dprintf("DGDecode: Colorimetry:       %s (%d)\n", Matrix_s, m_decoder.GOPList[gop]->matrix);
		dprintf("DGDecode: Quants:            %d/%d/%d (avg/min/max)\n", m_decoder.avgquant, m_decoder.minquant, m_decoder.maxquant);
	}
	else if (m_decoder.info == 3)
	{
		hint = 0;
		if (m_decoder.FrameList[raw].pf == 1) hint |= PROGRESSIVE;
		hint |= ((m_decoder.GOPList[gop]->matrix & 7) << COLORIMETRY_SHIFT);
		PutHintingData(frame->GetWritePtr(PLANAR_Y), hint);
	}

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

			emms
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

			emms
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

		emms
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

void MPEG2Source::conv422toYUV422_2(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, 
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	int widthdiv2 = width >> 1;
	__asm
	{
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov edi,[dst]
		mov ecx,widthdiv2
yloop:
		xor eax,eax
		align 16
xloop:
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		punpcklbw mm0,mm1      ;VYUYVYUY
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4],mm0   ;VYUYVYUY
		movq [edi+eax*4+8],mm3 ;store
		add eax,4
		cmp eax,ecx
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
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
	if (!vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("BlindPP : Only YV12 and YUY2 colorspace supported");

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
		if (cpu2[0]=='x' || cpu2[0]=='X') { PP_MODE |= PP_DEBLOCK_Y_H; }
		if (cpu2[1]=='x' || cpu2[1]=='X') { PP_MODE |= PP_DEBLOCK_Y_V; }
		if (cpu2[2]=='x' || cpu2[2]=='X') { PP_MODE |= PP_DEBLOCK_C_H; }
		if (cpu2[3]=='x' || cpu2[3]=='X') { PP_MODE |= PP_DEBLOCK_C_V; }
		if (cpu2[4]=='x' || cpu2[4]=='X') { PP_MODE |= PP_DERING_Y; }
		if (cpu2[5]=='x' || cpu2[5]=='X') { PP_MODE |= PP_DERING_C; }
	}

	iPP = args[4].AsBool(false);
	moderate_h = args[5].AsInt(20);
	moderate_v = args[6].AsInt(40);

	if (vi.IsYUY2()) 
	{
		out = create_YV12PICT(vi.height,vi.width,2);
		PP_MODE |= PP_DONT_COPY;
	}
	else out = NULL;
}

PVideoFrame __stdcall BlindPP::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame dstf = env->NewVideoFrame(vi);
	PVideoFrame cf = child->GetFrame(n,env);

	if (vi.IsYV12())
	{
		uc* src[3];
		uc* dst[3];
		src[0] = (uc*) cf->GetReadPtr(PLANAR_Y);
		src[1] = (uc*) cf->GetReadPtr(PLANAR_U);
		src[2] = (uc*) cf->GetReadPtr(PLANAR_V);
		dst[0] = dstf->GetWritePtr(PLANAR_Y);
		dst[1] = dstf->GetWritePtr(PLANAR_U);
		dst[2] = dstf->GetWritePtr(PLANAR_V);
		if (iPP) postprocess(src, cf->GetPitch(), dst, dstf->GetPitch()*2, vi.width*2, vi.height/2, QP, 
								vi.width/16, PP_MODE, moderate_h, moderate_v, false); 
		else postprocess(src, cf->GetPitch(), dst, dstf->GetPitch(), vi.width, vi.height, QP, vi.width/16, 
								PP_MODE, moderate_h, moderate_v, false);
	}
	else
	{
		uc* dst[3];
		convYUV422to422(cf->GetReadPtr(),out->y,out->u,out->v,cf->GetPitch(),out->ypitch,
			out->uvpitch,vi.width,vi.height); // 4:2:2 packed to 4:2:2 planar
		dst[0] = out->y;
		dst[1] = out->u;
		dst[2] = out->v;
		if (iPP) postprocess(dst, 0, dst, out->ypitch*2, vi.width*2, vi.height/2, QP, vi.width/16, 
								PP_MODE, moderate_h, moderate_v, true);
		else postprocess(dst, 0, dst, out->ypitch, vi.width, vi.height, QP, vi.width/16, PP_MODE, 
								moderate_h, moderate_v, true);
		conv422toYUV422b(out->y,out->u,out->v,dstf->GetWritePtr(),out->ypitch,out->uvpitch,
			dstf->GetPitch(),vi.width,vi.height);  // 4:2:2 planar to 4:2:2 packed
	}

	return dstf;
}

BlindPP::~BlindPP() 
{ 
	delete[] QP;
	if (out != NULL) destroy_YV12PICT(out);
}

void BlindPP::convYUV422to422(const unsigned char *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
       int pitch1, int pitch2y, int pitch2uv, int width, int height)
{
	int widthdiv2 = width>>1;
	__int64 Ymask = 0x00FF00FF00FF00FFi64;
	__asm
	{
		mov edi,[src]
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov ecx,widthdiv2
		movq mm5,Ymask
	yloop:
		xor eax,eax
		align 16
	xloop:
		movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
		movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
		movq mm2,mm0           ; VYUYVYUY - 1
		movq mm3,mm1           ; VYUYVYUY - 2
		pand mm0,mm5           ; 0Y0Y0Y0Y - 1
		psrlw mm2,8 	       ; 0V0U0V0U - 1
		pand mm1,mm5           ; 0Y0Y0Y0Y - 2
		psrlw mm3,8            ; 0V0U0V0U - 2
		packuswb mm0,mm1       ; YYYYYYYY
		packuswb mm2,mm3       ; VUVUVUVU
		movq mm4,mm2           ; VUVUVUVU
		pand mm2,mm5           ; 0U0U0U0U
		psrlw mm4,8            ; 0V0V0V0V
		packuswb mm2,mm2       ; xxxxUUUU
		packuswb mm4,mm4       ; xxxxVVVV
		movq [ebx+eax*2],mm0   ; store y
		movd [edx+eax],mm2     ; store u
		movd [esi+eax],mm4     ; store v
		add eax,4
		cmp eax,ecx
		jl xloop
		add edi,pitch1
		add ebx,pitch2y
		add edx,pitch2uv
		add esi,pitch2uv
		dec height
		jnz yloop
		emms
	}
}

void BlindPP::conv422toYUV422b(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, 
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	int widthdiv2 = width>>1;
	__asm
	{
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov edi,[dst]
		mov ecx,widthdiv2
yloop:
		xor eax,eax
		align 16
xloop:
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		punpcklbw mm0,mm1      ;VYUYVYUY
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4],mm0   ;VYUYVYUY
		movq [edi+eax*4+8],mm3 ;store
		add eax,4
		cmp eax,ecx
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
		emms
	}
}

Deblock::Deblock(PClip _child, int q, int aOff, int bOff, bool _mmx, bool _isse,
                 IScriptEnvironment* env) :
GenericVideoFilter(_child)
{
    nQuant = sat(q, 0, 51);
    nAOffset = sat(aOff, -nQuant, 51 - nQuant);
    nBOffset = sat(bOff, -nQuant, 51 - nQuant);
    nWidth = vi.width;
    nHeight = vi.height;

    if ( !vi.IsYV12() )
        env->ThrowError("Deblock : need YV12 input");
    if (( nWidth & 7 ) || ( nHeight & 7 ))
        env->ThrowError("Deblock : width and height must be mod 8");
}

Deblock::~Deblock()
{
}

const int alphas[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 4, 4,
    5, 6, 7, 8, 9, 10,
    12, 13, 15, 17, 20,
    22, 25, 28, 32, 36,
    40, 45, 50, 56, 63, 
    71, 80, 90, 101, 113,
    127, 144, 162, 182,
    203, 226, 255, 255
};

const int betas[52] = {
    0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 2,
    2, 3, 3, 3, 3, 4,
    4, 4, 6, 6, 
    7, 7, 8, 8, 9, 9,
    10, 10, 11, 11, 12,
    12, 13, 13, 14, 14, 
    15, 15, 16, 16, 17, 
    17, 18, 18
};

const int cs[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 
    1, 2, 2, 2, 2, 3, 
    3, 3, 4, 4, 5, 5,
    6, 7, 8, 8, 10, 
    11, 12, 13, 15, 17
};



void Deblock::DeblockPicture(unsigned char *srcp, int srcPitch, int w, int h,
                             int q, int aOff, int bOff)
{
    int indexa, indexb;
    for ( int j = 0; j < h; j += 4 )
    {
        for ( int i = 0; i < w; i += 4 )
        {
            indexa = sat(q + aOff, 0, 51);
            indexb = sat(q + bOff, 0, 51);
            if ( j > 0 )
                DeblockHorEdge(srcp + i, srcPitch, indexa, indexb);
            if ( i > 0 )
                DeblockVerEdge(srcp + i, srcPitch, indexa, indexb);
        }
        srcp += 4 * srcPitch;
    }
}

void Deblock::DeblockHorEdge(unsigned char *srcp, int srcPitch, int ia, int ib)
{
    int alpha = alphas[ia];
    int beta = betas[ib];
    int c, c0 = cs[ia];
    unsigned char *sq0 = srcp;
    unsigned char *sq1 = srcp + srcPitch;
    unsigned char *sq2 = srcp + 2 * srcPitch;
    unsigned char *sp0 = srcp - srcPitch;
    unsigned char *sp1 = srcp - 2 * srcPitch;
    unsigned char *sp2 = srcp - 3 * srcPitch;
    int delta, ap, aq, deltap1, deltaq1;

    for ( int i = 0; i < 4; i ++ )
    {
        if (( abs(sp0[i] - sq0[i]) < alpha ) && ( abs(sp1[i] - sp0[i]) < beta ) && ( abs(sq0[i] - sq1[i]) < beta ))
        {
            ap = abs(sp2[i] - sp0[i]);
            aq = abs(sq2[i] - sq0[i]);
            c = c0;
            if ( aq < beta ) c++;
            if ( ap < beta ) c++;
            delta = sat((((sq0[i] - sp0[i]) << 2) + (sp1[i] - sq1[i]) + 4) >> 3, -c, c);
            deltap1 = sat((sp2[i] + ((sp0[i] + sq0[i] + 1) >> 1) - (sp1[i] << 1)) >> 1, -c0, c0);
            deltaq1 = sat((sq2[i] + ((sp0[i] + sq0[i] + 1) >> 1) - (sq1[i] << 1)) >> 1, -c0, c0);
            sp0[i] = (unsigned char)sat(sp0[i] + delta, 0, 255);
            sq0[i] = (unsigned char)sat(sq0[i] - delta, 0, 255);
            if ( ap < beta )
                sp1[i] = (unsigned char)(sp1[i] + deltap1);
            if ( aq < beta )
                sq1[i] = (unsigned char)(sq1[i] + deltaq1);
        }
    }
}

void Deblock::DeblockVerEdge(unsigned char *srcp, int srcPitch, int ia, int ib)
{
    int alpha = alphas[ia];
    int beta = betas[ib];
    int c, c0 = cs[ia];
    unsigned char *s = srcp;

    int delta, ap, aq, deltap1, deltaq1;

    for ( int i = 0; i < 4; i ++ )
    {
        if (( abs(s[0] - s[-1]) < alpha ) && ( abs(s[1] - s[0]) < beta ) && ( abs(s[-1] - s[-2]) < beta ))
        {
            ap = abs(s[2] - s[0]);
            aq = abs(s[-3] - s[-1]);
            c = c0;
            if ( aq < beta ) c++;
            if ( ap < beta ) c++;
            delta = sat((((s[0] - s[-1]) << 2) + (s[-2] - s[1]) + 4) >> 3, -c, c);
            deltaq1 = sat((s[2] + ((s[0] + s[-1] + 1) >> 1) - (s[1] << 1)) >> 1, -c0, c0);
            deltap1 = sat((s[-3] + ((s[0] + s[-1] + 1) >> 1) - (s[-2] << 1)) >> 1, -c0, c0);
            s[0] = (unsigned char)sat(s[0] - delta, 0, 255);
            s[-1] = (unsigned char)sat(s[-1] + delta, 0, 255);
            if ( ap < beta )
                s[1] = (unsigned char)(s[1] + deltaq1);
            if ( aq < beta )
                s[-2] = (unsigned char)(s[-2] + deltap1);
        }
        s += srcPitch;
    }
}



PVideoFrame __stdcall Deblock::GetFrame(int n, IScriptEnvironment *env)
{
    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);

    DeblockPicture(YWPLAN(src), YPITCH(src), nWidth, nHeight,
                   nQuant, nAOffset, nBOffset);
    DeblockPicture(UWPLAN(src), UPITCH(src), nWidth / 2, nHeight / 2,
                   nQuant, nAOffset, nBOffset);
    DeblockPicture(VWPLAN(src), VPITCH(src), nWidth / 2, nHeight / 2,
                   nQuant, nAOffset, nBOffset);

    return src;
}

AVSValue __cdecl Create_MPEG2Source(AVSValue args, void*, IScriptEnvironment* env) 
{
//	char path[1024];
	char buf[80], *p;

	char d2v[255];
	int cpu = 0;
	int idct = -1;
	int iPP = args[3].IsBool() ? (args[3].AsBool() ? 1 : 0) : -1;
	int moderate_h = 20;
	int moderate_v = 40;
	bool showQ = false;
	bool fastMC = false;
	char cpu2[255];
	int info = 0;
	bool upConv = false;
	bool i420 = false;

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

		if ((f = fopen("DGDecode.def", "r")) != NULL)
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
				LOADINT(info,"info=",5);
				LOADBOOL(upConv,"upConv=",7);
				LOADBOOL(i420,"i420=",5);
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
	if (strlen(cpu2)>=255) cpu2[0]=0;

	MPEG2Source *dec = new MPEG2Source( args[0].AsString(d2v),
										args[1].AsInt(cpu),
										args[2].AsInt(idct),
										iPP,
										args[4].AsInt(moderate_h),
										args[5].AsInt(moderate_v),
										args[6].AsBool(showQ),
										args[7].AsBool(fastMC),
										args[8].AsString(cpu2),
										args[9].AsInt(info),
										args[10].AsBool(upConv),
										args[11].AsBool(i420),
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

AVSValue __cdecl Create_Deblock(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Deblock(
		args[0].AsClip(),
      args[1].AsInt(25),
      args[2].AsInt(0),
      args[3].AsInt(0),
		args[4].AsBool(true),
      args[5].AsBool(true),
    	env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("MPEG2Source", "[d2v]s[cpu]i[idct]i[iPP]b[moderate_h]i[moderate_v]i[showQ]b[fastMC]b[cpu2]s[info]i[upConv]b[i420]b", Create_MPEG2Source, 0);
    env->AddFunction("LumaFilter", "c[lumoff]i[lumgain]f", Create_LumaFilter, 0);
    env->AddFunction("YV12toRGB24", "c[interlaced]b[TVscale]b", Create_YV12toRGB24, 0);
    env->AddFunction("YV12toYUY2", "c[interlaced]b", Create_YV12toYUY2, 0);
    env->AddFunction("BlindPP", "c[quant]i[cpu]i[cpu2]s[iPP]b[moderate_h]i[moderate_v]i", Create_BlindPP, 0);
    env->AddFunction("Deblock", "c[quant]i[aOffset]i[bOffset]i[mmx]b[isse]b", Create_Deblock, 0);
    return 0;
}

// Code for backward compatibility and Gordian Knot support
// (running DGDecode functions without Avisynth).
YUVRGBScale ext_cscale;
MPEG2Source* vf = NULL;
unsigned char* buffer = NULL;
unsigned char* picture = NULL;
unsigned char* u422 = NULL, *u444 = NULL;
unsigned char* v422 = NULL, *v444 = NULL;

VideoInfo pvi;
int pitch = 0;
int vfapi_progressive;

MPEG2Source::MPEG2Source(const char* d2v)
{
	int cpu = 0;
	int idct = -1;
	int iPP = -1;
	int moderate_h = 20;
	int moderate_v = 40;
	bool showQ = false;
	bool fastMC = false;
	int info = 0;

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
	m_decoder.info = 0;
	m_decoder.showQ = showQ;
	m_decoder.fastMC = fastMC;
	m_decoder.upConv = false;
	m_decoder.i420 = false;

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
	vi.pixel_type = VideoInfo::CS_YV12;
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

	bufY = bufU = bufV = NULL;

	out = NULL;
	out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);
}

void MPEG2Source::GetFrame(int n, unsigned char *buffer, int pitch)
{
	out->y = buffer;
	out->u = out->y + (pitch * pvi.height);
	out->v = out->u + ((pitch * pvi.height)/4);
	out->ypitch = pitch;
	out->uvpitch = pitch/2;

	m_decoder.Decode(n, out);

	__asm emms;
}

extern "C" __declspec(dllexport) VideoInfo* __cdecl openMPEG2Source(char* file)
{
	char *p;

//dprintf("openMPEG2Source\n");

	// If the D2V filename has _P just before the extension, force
	// progressive upsampling; otherwise, use interlaced.
	p = file + strlen(file);
	while (*p != '.') p--;
	p -= 2;
	if (p[0] == '_' && p[1] == 'P')
		vfapi_progressive = 1;
	else
		vfapi_progressive = 0;
	
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
//dprintf("getRGBFrame\n");
//dprintf("frame %x\n", frame);
//dprintf("buffer %p\n", buffer);
	vf->GetFrame(frame, buffer, pitch);

	if (picture == NULL) picture = (unsigned char*)malloc(pvi.height * pvi.width * 3);

	unsigned char *y = buffer;
	unsigned char *u = y + (pitch * pvi.height);
	unsigned char *v = u + ((pitch * pvi.height)/4);

	conv420to422(u, u422, vfapi_progressive, pvi.width, pvi.height);
	conv422to444(u422, u444, pvi.width, pvi.height);
	conv420to422(v, v422, vfapi_progressive, pvi.width, pvi.height);
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
