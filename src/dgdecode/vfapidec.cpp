/* 
 *	Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *  Ported to C++ by Mathias Born - May 2001
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

/*
  lots of code modified for YV12 / MPEG2Dec3 - MarcFD
*/

//#define MPEG2DEC_EXPORTS
#include "global.h"
#include "mc.h"

static const int ChromaFormat[4] = {
	0, 6, 8, 12
};

CMPEG2Decoder::CMPEG2Decoder()
{
  VF_File = 0;
  VF_FrameLimit = 0;
  VF_GOPLimit = 0;
  VF_FrameRate = 0;
  prev_frame = 0xfffffffe;
  memset(Rdbfr, 0, sizeof(Rdbfr));
  Rdptr = Rdmax = 0;
  CurrentBfr = NextBfr = BitsLeft = Val = Read = 0;
  Fault_Flag = File_Flag = File_Limit = FO_Flag = IDCT_Flag = SystemStream_Flag = 0;

	MPEG2_Transport_AudioPID = 0x14;  // primary value for many US DTV broadcasters
	MPEG2_Transport_VideoPID = 0x11;  // primary value for many US DTV broadcasters

  Luminance_Flag = false;
  BufferOp = 0;
  memset(intra_quantizer_matrix, 0, sizeof(intra_quantizer_matrix));
  memset(non_intra_quantizer_matrix, 0, sizeof(non_intra_quantizer_matrix));
  memset(chroma_intra_quantizer_matrix, 0, sizeof(chroma_intra_quantizer_matrix));
  memset(chroma_non_intra_quantizer_matrix, 0, sizeof(chroma_non_intra_quantizer_matrix));
  load_intra_quantizer_matrix =
  load_non_intra_quantizer_matrix =
  load_chroma_intra_quantizer_matrix =
  load_chroma_non_intra_quantizer_matrix = 0;
  q_scale_type =
  alternate_scan =
  quantizer_scale = 0;

  AVSenv = NULL;
}

// Open function modified by Donald Graft as part of fix for dropped frames and random frame access.
int CMPEG2Decoder::Open(const char *path, DstFormat dstFormat)
{
	char buf[1024], *buf_p;

	Choose_Prediction(this->fastMC);

	m_dstFormat = dstFormat;
	char ID[80], PASS[80] = "DGIndexProjectFile06";
	DWORD i, j, size, code, type, tff, rff, film, ntsc, gop, top, bottom, mapping;
	int repeat_on, repeat_off, repeat_init;
	int vob_id, cell_id;
	__int64 position;
	int m, n;

	CMPEG2Decoder* out = this;

	out->VF_File = fopen(path, "r");
	if (fgets(ID, 79, out->VF_File)==NULL)
		return 0;
	if (strncmp(ID, PASS, 20))
		return 0;

	fscanf(out->VF_File, "%d", &File_Limit);
	i = File_Limit;
	while (i)
	{
		fscanf(out->VF_File, "%d ", &j);
		Infilename[File_Limit-i] = (char*)aligned_malloc(_MAX_PATH, 16);
		fgets(Infilename[File_Limit-i], j+1, out->VF_File);
		if ((Infile[File_Limit-i] = _open(Infilename[File_Limit-i], _O_RDONLY | _O_BINARY))==-1)
			return 0;
		i--;
	}

	fscanf(out->VF_File, "\nStream_Type=%d\n", &SystemStream_Flag);
	if ( SystemStream_Flag == 2 )
		fscanf(out->VF_File, "MPEG2_Transport_PID=%x,%x\n", &MPEG2_Transport_VideoPID,	&MPEG2_Transport_AudioPID);

	long nCurrentPos = ftell(out->VF_File);
	char szTempText[50];

	fscanf(out->VF_File, "iDCT_Algorithm=%d", &IDCT_Flag);
	fscanf(out->VF_File, "%s", szTempText);
	
	fseek(out->VF_File, nCurrentPos, SEEK_SET);

	fscanf(out->VF_File, "iDCT_Algorithm=%d (1:MMX 2:SSEMMX 3:FPU 4:REF 5:SSE2MMX)\n", &IDCT_Flag);

	switch (IDCT_Flag)
	{
		case IDCT_MMX:
			idctFunc = MMX_IDCT;
			break;

		case IDCT_SSEMMX:
			idctFunc = SSEMMX_IDCT;
			if (!cpu.ssemmx)
			{
				IDCT_Flag = IDCT_MMX;
				idctFunc = MMX_IDCT;
			}
			break;

		case IDCT_FPU:
			if (!fpuinit) 
			{
				Initialize_FPU_IDCT();
				fpuinit = true;
			}
			idctFunc = FPU_IDCT;
			break;

		case IDCT_REF:
			if (!refinit) 
			{
				Initialize_REF_IDCT();
				refinit = true;
			}
			idctFunc = REF_IDCT;
			break;

		case IDCT_SSE2MMX:
			idctFunc = SSE2MMX_IDCT;
			if (!cpu.sse2mmx)
			{
				IDCT_Flag = IDCT_SSEMMX;
				idctFunc = SSEMMX_IDCT;
				if (!cpu.ssemmx)
				{
					IDCT_Flag = IDCT_MMX;
					idctFunc = MMX_IDCT;
				}
			}
			break;
		
		case IDCT_SKALSSE:
			idctFunc = Skl_IDct16_Sparse_SSE; //Skl_IDct16_SSE;
			if (!cpu.ssemmx)
			{
				IDCT_Flag = IDCT_MMX;
				idctFunc = MMX_IDCT;
			}
			break;

		case IDCT_SIMPLEIDCT:
			idctFunc = simple_idct_mmx;
			if (!cpu.ssemmx)
			{
				IDCT_Flag = IDCT_MMX;
				idctFunc = MMX_IDCT;
			}
			break;
	}

	File_Flag = 0;
	_lseeki64(Infile[0], 0, SEEK_SET);
	Initialize_Buffer();

	do
	{
		if (Fault_Flag == OUT_OF_BITS) return 0;
		next_start_code();
		code = Get_Bits(32);
	}
	while (code!=SEQUENCE_HEADER_CODE);

	sequence_header();

	mb_width = (horizontal_size+15)/16;
	mb_height = progressive_sequence ? (vertical_size+15)/16 : 2*((vertical_size+31)/32);

	QP = (int*)aligned_malloc(sizeof(int)*mb_width*mb_height, 128);

	Coded_Picture_Width = 16 * mb_width;
	Coded_Picture_Height = 16 * mb_height;

	Chroma_Width = (chroma_format==CHROMA444) ? Coded_Picture_Width : Coded_Picture_Width>>1;
	Chroma_Height = (chroma_format!=CHROMA420) ? Coded_Picture_Height : Coded_Picture_Height>>1;

	block_count = ChromaFormat[chroma_format];

	for (i=0; i<8; i++)
	{
		p_block[i] = (short *)aligned_malloc(sizeof(short)*64 + 64, 128);
		block[i]   = (short *)((long)p_block[i] + 64 - (long)p_block[i]%64);
	}

	p_fTempArray = (void *)aligned_malloc(sizeof(float)*128 + 64, 128);
	fTempArray = (void *)((long)p_fTempArray + 64 - (long)p_fTempArray%64);

	for (i=0; i<3; i++)
	{
		if (i==0)
			size = Coded_Picture_Width * Coded_Picture_Height;
		else
			size = Chroma_Width * Chroma_Height;

		backward_reference_frame[i] = (unsigned char*)aligned_malloc(2*size+4096, 128);  //>>> cheap safety bump
		forward_reference_frame[i] = (unsigned char*)aligned_malloc(2*size+4096, 128);
		auxframe[i] = (unsigned char*)aligned_malloc(2*size+4096, 128);
	}

	fscanf(out->VF_File, "YUVRGB_Scale=%d (0:TVScale 1:PCScale)\n", &i);

	if (i)
	{
		RGB_Scale = 0x1000254310002543;
		RGB_Offset = 0x0010001000100010;
		RGB_CBU = 0x0000408D0000408D;
		RGB_CGX = 0xF377E5FCF377E5FC;
		RGB_CRV = 0x0000331300003313;
	}
	else
	{
		RGB_Scale = 0x1000200010002000;
		RGB_Offset = 0x0000000000000000;
		RGB_CBU = 0x000038B4000038B4;
		RGB_CGX = 0xF4FDE926F4FDE926;
		RGB_CRV = 0x00002CDD00002CDD;
	}

	fscanf(out->VF_File, "Luminance_Filter=%d,%d (Gamma, Offset)\n", &i, &j);
	i+=128;

	if (i==128 && j==0)
		Luminance_Flag = 0;
	else
	{
		Luminance_Flag = 1;
		LumGainMask = ((__int64)i<<48) + ((__int64)i<<32) + ((__int64)i<<16) + (__int64)i;
		LumOffsetMask = ((__int64)j<<48) + ((__int64)j<<32) + ((__int64)j<<16) + (__int64)j;

		nLumSize = Coded_Picture_Width * Coded_Picture_Height;

		// Luminance Gain on old DVD2AVI used to be set at 128 as default
		// But we use the new Luminance Filter, so 128 would equal 0
		i -= 128;

		InitializeLuminanceFilter(i, j);

		lum = (unsigned char*)aligned_malloc(Coded_Picture_Width * Coded_Picture_Height, 128);
	}

	fscanf(out->VF_File, "Clipping=%d,%d,%d,%d (ClipLeft, ClipRight, ClipTop, ClipBottom)\n", 
		   &Clip_Left, &Clip_Right, &Clip_Top, &Clip_Bottom);
	fscanf(out->VF_File, "Aspect_Ratio=%s\n", Aspect_Ratio);
	fscanf(out->VF_File, "Picture_Size=%s\n", &m, &n);

	Clip_Width = Coded_Picture_Width;
	Clip_Height = Coded_Picture_Height;
	CLIP_AREA = HALF_CLIP_AREA = CLIP_STEP = 0;

	LUM_AREA = Coded_Picture_Width * Clip_Height;
	PROGRESSIVE_HEIGHT = (Coded_Picture_Height>>1) - 2;
	INTERLACED_HEIGHT = (Coded_Picture_Height>>2) - 2;
	HALF_WIDTH = Coded_Picture_Width>>1;
	HALF_WIDTH_D8 = (Coded_Picture_Width>>1) - 8;
	DOUBLE_WIDTH = Coded_Picture_Width<<1;

	u422 = (unsigned char*)aligned_malloc(Coded_Picture_Width * Coded_Picture_Height / 2, 128);
	v422 = (unsigned char*)aligned_malloc(Coded_Picture_Width * Coded_Picture_Height / 2, 128);
	u444 = (unsigned char*)aligned_malloc(Coded_Picture_Width * Coded_Picture_Height, 128);
	v444 = (unsigned char*)aligned_malloc(Coded_Picture_Width * Coded_Picture_Height, 128);

	auxFrame1 = create_YV12PICT(Coded_Picture_Height,Coded_Picture_Width);
	auxFrame2 = create_YV12PICT(Coded_Picture_Height,Coded_Picture_Width);
	saved_active = auxFrame1;
	saved_store = auxFrame2;

	fscanf(out->VF_File, "Field_Operation=%d (0:None 1:ForcedFILM 2:RawFrames)\n", &FO_Flag);

	fscanf(out->VF_File, "Frame_Rate=%d\n", &(out->VF_FrameRate));
	fscanf(out->VF_File, "Location=%d,%X,%d,%X\n", &i, &j, &i, &j);

	ntsc = film = top = bottom = gop = mapping = repeat_on = repeat_off = repeat_init = 0;
	HaveRFFs = false;

	fgets(buf, 1023, out->VF_File);
	buf_p = buf;
	while (true)
	{
		sscanf(buf_p, "%x", &type);
		if (type & 0x8)
			break;
		if (type & 0x4)	// I frame
		{
			GOPList[gop] = reinterpret_cast<GOPLIST*>(calloc(1, sizeof(GOPLIST)));
			GOPList[gop]->number = film;
			while (*buf_p++ != ' ');
			sscanf(buf_p, "%d", &(GOPList[gop]->file));
			while (*buf_p++ != ' ');
			position = _atoi64(buf_p);
			while (*buf_p++ != ' ');
			sscanf(buf_p, "%d %d", &vob_id, &cell_id);
			while (*buf_p++ != ' ');
			while (*buf_p++ != ' ');
			GOPList[gop]->position = position;
			gop++;

			sscanf(buf_p, "%x", &type);
			tff = (type & 0x2) >> 1;
			if (FO_Flag == FO_RAW)
				rff = 0;
			else
				rff = type & 0x1;
		}
		else	// P, B frame
		{
			tff = (type & 0x2) >> 1;
			if (FO_Flag == FO_RAW)
				rff = 0;
			else
				rff = type & 0x1;
		}
		if (FO_Flag != FO_FILM && FO_Flag != FO_RAW && rff) HaveRFFs = true;

		if (!film)
		{
			if (tff)
				Field_Order = 1;
			else
				Field_Order = 0;
		}

		if (FO_Flag==FO_FILM)
		{
			if (rff)
				repeat_on++;
			else
				repeat_off++;

			if (repeat_init)
			{
				if (repeat_off-repeat_on == 5)
				{
					repeat_on = repeat_off = 0;
				}
				else
				{
					FrameList[mapping] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
					FrameList[mapping]->top = FrameList[mapping]->bottom = film;
					mapping ++;
				}

				if (repeat_on-repeat_off == 5)
				{
					repeat_on = repeat_off = 0;
					FrameList[mapping] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
					FrameList[mapping]->top = FrameList[mapping]->bottom = film;
					mapping ++;
				}
			}
			else
			{
				if (repeat_off-repeat_on == 3)
				{
					repeat_on = repeat_off = 0;
					repeat_init = 1;
				}
				else
				{
					FrameList[mapping] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
					FrameList[mapping]->top = FrameList[mapping]->bottom = film;
					mapping ++;
				}

				if (repeat_on-repeat_off == 3)
				{
					repeat_on = repeat_off = 0;
					repeat_init = 1;

					FrameList[mapping] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
					FrameList[mapping]->top = FrameList[mapping]->bottom = film;
					mapping ++;
				}
			}
		}
		else
		{
			if (top)
			{
				FrameList[ntsc]->bottom = film;
				ntsc ++;
				FrameList[ntsc] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
				FrameList[ntsc]->top = film;
			}
			else if (bottom)
			{
				FrameList[ntsc]->top = film;
				ntsc ++;
				FrameList[ntsc] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
				FrameList[ntsc]->bottom = film;
			}
			else
			{
				FrameList[ntsc] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));
				FrameList[ntsc]->top = film;
				FrameList[ntsc]->bottom = film;
				ntsc ++;
			}

			if (rff)
			{
				if (!top && !bottom)
					FrameList[ntsc] = reinterpret_cast<FRAMELIST*>(calloc(1, sizeof(FRAMELIST)));

				if (tff)
				{
					FrameList[ntsc]->top = film;
					top = 1;
				}
				else
				{
					FrameList[ntsc]->bottom = film;
					bottom = 1;
				}

				if (top && bottom)
				{
					top = bottom = 0;
					ntsc ++;
				}
			}
		}

		// Remember if this encoded frame requires the previous GOP to be decoded.
		// The previous GOP is required if DVD2AVI has marked it as such.
		if (!(type & 0x10))
			DirectAccess[film] = 0;
		else
			DirectAccess[film] = 1;

		film++;

		// Move to the next flags digit or get the next line.
		while (*buf_p != '\n' && *buf_p != ' ') buf_p++;
		if (*buf_p == '\n')
		{
			fgets(buf, 1023, out->VF_File);
			buf_p = buf;
		}
		else buf_p++;
	}
// dprintf("gop = %d, film = %d, ntsc = %d\n", gop, film, ntsc);
	out->VF_GOPLimit = gop;

	if (FO_Flag==FO_FILM)
	{
		while (FrameList[mapping-1]->top >= film)
			mapping--;

		out->VF_FrameLimit = mapping;
	}
	else
	{
		while ((FrameList[ntsc-1]->top >= film) || (FrameList[ntsc-1]->bottom >= film))
			ntsc--;

		out->VF_FrameLimit = ntsc;
	}

	// Count the number of nondecodable frames at the start of the clip
	// (due to an open GOP). This will be used to avoid displaying these
	// bad frames.
	File_Flag = 0;
	_lseeki64(Infile[File_Flag], GOPList[0]->position, SEEK_SET);
	Initialize_Buffer();

	BadStartingFrames = 0;
	closed_gop = -1;
	Get_Hdr();
	if (closed_gop != 1)
	{
		// Leading B frames are non-decodable.
		while (true)
		{
			Get_Hdr();
			if (picture_coding_type != B_TYPE) break;
			if (picture_structure != FRAME_PICTURE)
				Get_Hdr();
			BadStartingFrames++;
		}
		// Frames pulled down from non-decodable frames are non-decodable.
		if (BadStartingFrames)
		{
			i = 0;
			while (true)
			{
				if ((FrameList[i]->top > BadStartingFrames - 1) &&
					(FrameList[i]->bottom > BadStartingFrames - 1))
					break;
				i++;
			}
			BadStartingFrames = i;
		}
	}

	return 1;
}

// Decode function rewritten by Donald Graft as part of fix for dropped frames and random frame access.
#define SEEK_THRESHOLD 7

void CMPEG2Decoder::Decode(DWORD frame, YV12PICT *dst)
{
	unsigned int i, f, gop, count, HadI, requested_frame;
	YV12PICT *tmp;

	Fault_Flag = 0;

//	dprintf("MPEG2DEC3: %d: top = %d, bot = %d\n", frame, FrameList[frame]->top, FrameList[frame]->bottom);

	// Skip initial non-decodable frames.
	if (frame < BadStartingFrames) frame = BadStartingFrames;
	requested_frame = frame;

	// Decide whether to use random access or linear play to reach the
	// requested frame. If the seek is just a few frames forward, it will
	// be faster to play linearly to get there. This greatly speeds things up
	// when a decimation like SelectEven() follows this filter.
	if (frame && frame > BadStartingFrames && frame > prev_frame && frame < prev_frame + SEEK_THRESHOLD)
	{
		// Use linear play.
		for (frame = prev_frame + 1; frame <= requested_frame; frame++)
		{

			// If doing force film, we have to skip or repeat a frame as needed.
			// This handles skipping. Repeating is handled below.
			if ((FO_Flag==FO_FILM) &&
				(FrameList[frame]->bottom == FrameList[frame-1]->bottom + 2) &&
				(FrameList[frame]->top == FrameList[frame-1]->top + 2))
			{
				if (!Get_Hdr())
				{
					// Flush the final frame.
					assembleFrame(backward_reference_frame, pf_backward, dst);
					return;
				}
				Decode_Picture(dst);
				if (Fault_Flag == OUT_OF_BITS)
				{
					assembleFrame(backward_reference_frame, pf_backward, dst);
					return;
				}
				if (picture_structure != FRAME_PICTURE)
				{
					Get_Hdr();
					Decode_Picture(dst);
				}
			}

			/* When RFFs are present or we are doing FORCE FILM, we may have already decoded
			   the frame that we need. So decode the next frame only when we need to. */
			if (((Field_Order == 1) && (FrameList[frame]->bottom != FrameList[frame-1]->bottom)) ||
				((Field_Order == 0) && (FrameList[frame]->top != FrameList[frame-1]->top)))
			{			
				if (!Get_Hdr())
				{
					assembleFrame(backward_reference_frame, pf_backward, dst);
					return;
				}
				Decode_Picture(dst);
				if (Fault_Flag == OUT_OF_BITS)
				{
					assembleFrame(backward_reference_frame, pf_backward, dst);
					return;
				}
				if (picture_structure != FRAME_PICTURE)
				{
					Get_Hdr();
					Decode_Picture(dst);
				}
				/* If RFFs are present we have to save the decoded frame to
				   be able to pull down from it when we decode the next frame.
				   If we are doing FORCE FILM, then we may need to repeat the
				   frame, so we'll save it for that purpose. */
				if (FO_Flag == FO_FILM || HaveRFFs == true)
				{
					// Save the current frame without overwriting the last
					// stored frame.
					tmp = saved_active;
					saved_active = saved_store;
					saved_store = tmp;
					Copyall(dst, saved_store);
				}
			}
			else
			{
				// We already decoded the needed frame. Retrieve it.
				Copyall(saved_store, dst);
			}

			// Perform pulldown if required.
			if (HaveRFFs == true)
			{
				if (FrameList[frame]->top > FrameList[frame]->bottom)
				{
					Copyeven(saved_active, dst);
				}	
				else if (FrameList[frame]->top < FrameList[frame]->bottom)
				{
					Copyodd(saved_active, dst);
				}
			}
		}
		prev_frame = requested_frame;
		return;
	}
	else prev_frame = requested_frame;

	// Have to do random access.
	f = FrameList[frame]->top;
	if (FrameList[frame]->top < FrameList[frame]->bottom)
	{
		f = FrameList[frame]->bottom;
	}

	// Determine the GOP that the requested frame is in.
	for (gop = 0; gop < VF_GOPLimit-1; gop++)
	{
		if (f >= GOPList[gop]->number && f < GOPList[gop+1]->number)
		{
			break;
		}
	}

	// Back off by one GOP if required. This ensures enough frames will
	// be decoded that the requested frame is guaranteed to be decodable.
	// The direct flag is written by DVD2AVIdg, who knows which frames
	// require the previous GOP to be decoded. We also test whether
	// the previous encoded frame requires it, because that one might be pulled
	// down for this frame. This can be improved by actually testing if the
	// previous frame will be pulled down.
	if (gop && (DirectAccess[f] == 0 || (f && DirectAccess[f-1] == 0)))
		gop--;

	// Calculate how many frames to decode.
	count = f - GOPList[gop]->number;

	// Seek in the stream to the GOP to start decoding with.
	File_Flag = GOPList[gop]->file;
	_lseeki64(Infile[GOPList[gop]->file],
		      GOPList[gop]->position,
			  SEEK_SET);
	Initialize_Buffer();

	// Start decoding. Stop when the requested frame is decoded.
	// First get the starting I frame of the GOP.
	HadI = 0;
	while (true)
	{
		if (!Get_Hdr())
		{
			// Something is really messed up.
			return;
		}
		Decode_Picture(dst);
		if (picture_coding_type == I_TYPE) HadI = 1;
		if (picture_structure != FRAME_PICTURE)
		{
			Get_Hdr();
			Decode_Picture(dst);
		}
		if (HadI) break;
	}
	if (HaveRFFs == true && count == 0)
	{
		if (Field_Order == 0) Copyeven(dst, saved_active);
		else  Copyodd(dst, saved_active);
	}
	if (!Get_Hdr())
	{
		// Flush the final frame.
		assembleFrame(backward_reference_frame, pf_backward, dst);
		return;
	}
	Decode_Picture(dst);
	if (Fault_Flag == OUT_OF_BITS)
	{
		assembleFrame(backward_reference_frame, pf_backward, dst);
		return;
	}
	if (picture_structure != FRAME_PICTURE)
	{
		Get_Hdr();
		Decode_Picture(dst);
	}
	if (HaveRFFs == true && count == 1)
	{
		if (Field_Order == 0) Copyeven(dst, saved_active);
		else  Copyodd(dst, saved_active);
	}
	for (i = 0; i < count; i++)
	{
		if (!Get_Hdr())
		{
			// Flush the final frame.
			assembleFrame(backward_reference_frame, pf_backward, dst);
			return;
		}
		Decode_Picture(dst);
		if (Fault_Flag == OUT_OF_BITS)
		{
			assembleFrame(backward_reference_frame, pf_backward, dst);
			return;
		}
		if (picture_structure != FRAME_PICTURE)
		{
			Get_Hdr();
			Decode_Picture(dst);
		}
		if ((HaveRFFs == true) && (count > 1) && (i == count - 2))
		{
			if (Field_Order == 0) Copyeven(dst, saved_active);
			else  Copyodd(dst, saved_active);
		}
	}

	if (HaveRFFs == true)
	{
		// Save for transition to non-random mode.
		Copyall(dst, saved_store);

		// Pull down a field if needed.
		if (FrameList[frame]->top > FrameList[frame]->bottom)
		{
			Copyeven(saved_active, dst);
		}	
		else if (FrameList[frame]->top < FrameList[frame]->bottom)
		{
			Copyodd(saved_active, dst);
		}
	}
}

void CMPEG2Decoder::Close()
{
	int i;
	CMPEG2Decoder* in = this;

	if (in != NULL)
		fclose(in->VF_File);

	while (File_Limit)
	{
		File_Limit--;
		_close(Infile[File_Limit]);
	}

	for (i=0; i<File_Limit; i++)
		aligned_free(Infilename[i]);

	for (i=0; i<3; i++)
	{
		aligned_free(backward_reference_frame[i]);
		aligned_free(forward_reference_frame[i]);
		aligned_free(auxframe[i]);
	}

	aligned_free(QP);

	aligned_free(u422);
	aligned_free(v422);
	aligned_free(u444);
	aligned_free(v444);
	
	destroy_YV12PICT(auxFrame1);
	destroy_YV12PICT(auxFrame2);

	if(Luminance_Flag)
		aligned_free(lum);

	for (i=0; i<8; i++)
		aligned_free(p_block[i]);

	aligned_free(p_fTempArray);
}

// mmx YV12 framecpy by MarcFD 25 nov 2002 (okay the macros are ugly, but it's fast ^^)

#define cpylinemmx(src,dst,cnt,n)	\
__asm mov  esi,src	\
__asm mov  edi,dst	\
__asm mov  ecx,cnt	\
__asm push ecx		\
__asm and  ecx,63	\
__asm rep  movsb	\
__asm pop  ecx		\
__asm {shr  ecx,6}	\
mmx_cpy##n:			\
__asm movq mm0,[esi+0]	\
__asm movq mm1,[esi+8]	\
__asm movq [edi+0],mm0	\
__asm movq [edi+8],mm1	\
__asm movq mm2,[esi+16]	\
__asm movq mm3,[esi+24]	\
__asm movq [edi+16],mm2	\
__asm movq [edi+24],mm3	\
__asm movq mm0,[esi+32]	\
__asm movq mm1,[esi+40]	\
__asm movq [edi+32],mm0	\
__asm movq [edi+40],mm1	\
__asm movq mm2,[esi+48]	\
__asm movq mm3,[esi+56]	\
__asm movq [edi+48],mm2	\
__asm movq [edi+56],mm3	\
__asm add esi,64	\
__asm add edi,64	\
__asm dec ecx		\
__asm jnz mmx_cpy##n	\


#define cpy_offset(_src,_srcoffset,_dst,_dstoffset,_dststride,_lines,n) \
unsigned char *src##n = _src;		\
int srcoffset##n = _srcoffset;		\
unsigned char *dst##n = _dst;		\
int dstoffset##n = _dstoffset;		\
int dststride##n = _dststride; \
int lines##n = _lines;		\
__asm mov eax,src##n		\
__asm mov ebx,dst##n		\
__asm {mov edx,lines##n}	\
__asm row_loop##n:		\
__asm cpylinemmx(eax,ebx,dststride##n,n)		\
__asm add eax,srcoffset##n	\
__asm add ebx,dstoffset##n	\
__asm dec edx				\
__asm jnz row_loop##n


#define cpy_oddeven(_odd,_oddoffset,_even,_evenoffset,_dst,_dststride,_lines,n) \
unsigned char *odd##n = _odd; \
int oddoffset##n = _oddoffset; \
unsigned char *even##n = _even; \
int evenoffset##n = _evenoffset; \
unsigned char *dst##n = _dst; \
int dststride##n = _dststride; \
int lines##n = _lines; \
__asm mov eax,odd##n \
__asm mov ebx,even##n \
__asm mov edx,dst##n \
__asm row_loop##n: \
__asm cpylinemmx(eax,edx,dststride##n,n) \
__asm add edx,dststride##n \
__asm cpylinemmx(ebx,edx,dststride##n,##n##n) \
__asm add edx,dststride##n \
__asm add eax,oddoffset##n \
__asm add ebx,evenoffset##n \
__asm mov ecx,lines##n \
__asm dec ecx \
__asm mov lines##n,ecx \
__asm jnz row_loop##n \

void CMPEG2Decoder::Copyall(YV12PICT *src, YV12PICT *dst)
{
	if ( AVSenv )
	{

		AVSenv->BitBlt(dst->y, dst->ypitch, src->y, src->ypitch, src->ypitch, Coded_Picture_Height);
		AVSenv->BitBlt(dst->u, dst->uvpitch, src->u, src->uvpitch, src->uvpitch, Coded_Picture_Height>>1);
		AVSenv->BitBlt(dst->v, dst->uvpitch, src->v, src->uvpitch, src->uvpitch, Coded_Picture_Height>>1);
	}
	else
	{
		cpy_offset(src->y,src->ypitch,dst->y,dst->ypitch,dst->ypitch,Coded_Picture_Height,0);
		cpy_offset(src->u,src->uvpitch,dst->u,dst->uvpitch,dst->uvpitch,Coded_Picture_Height>>1,1);
		cpy_offset(src->v,src->uvpitch,dst->v,dst->uvpitch,dst->uvpitch,Coded_Picture_Height>>1,2);
	}
}

void CMPEG2Decoder::Copyodd(YV12PICT *src, YV12PICT *dst)
{
	if ( AVSenv )
	{
		AVSenv->BitBlt(dst->y, dst->ypitch*2, src->y,src->ypitch*2, src->ypitch, Coded_Picture_Height>>1);
		AVSenv->BitBlt(dst->u, dst->uvpitch*2, src->u,src->uvpitch*2, src->uvpitch, Coded_Picture_Height>>2);
		AVSenv->BitBlt(dst->v, dst->uvpitch*2, src->v,src->uvpitch*2, src->uvpitch, Coded_Picture_Height>>2);
	}
	else
	{
		cpy_offset(src->y,src->ypitch*2,dst->y,dst->ypitch*2,dst->ypitch,Coded_Picture_Height>>1,0);
		cpy_offset(src->u,src->uvpitch*2,dst->u,dst->uvpitch*2,dst->uvpitch,Coded_Picture_Height>>2,1);
		cpy_offset(src->v,src->uvpitch*2,dst->v,dst->uvpitch*2,dst->uvpitch,Coded_Picture_Height>>2,2);
	}
}

void CMPEG2Decoder::Copyeven(YV12PICT *src, YV12PICT *dst)
{
	if ( AVSenv )
	{
		AVSenv->BitBlt(dst->y+dst->ypitch, dst->ypitch*2, src->y+src->ypitch, src->ypitch*2, src->ypitch, Coded_Picture_Height>>1);
		AVSenv->BitBlt(dst->u+dst->uvpitch, dst->uvpitch*2, src->u+src->uvpitch, src->uvpitch*2, src->uvpitch, Coded_Picture_Height>>2);
		AVSenv->BitBlt(dst->v+dst->uvpitch, dst->uvpitch*2, src->v+src->uvpitch, src->uvpitch*2, src->uvpitch, Coded_Picture_Height>>2);
	}
	else
	{
		cpy_offset(src->y+src->ypitch,src->ypitch*2,dst->y+dst->ypitch,dst->ypitch*2,dst->ypitch,Coded_Picture_Height>>1,0);
		cpy_offset(src->u+src->uvpitch,src->uvpitch*2,dst->u+dst->uvpitch,dst->uvpitch*2,dst->uvpitch,Coded_Picture_Height>>2,1);
		cpy_offset(src->v+src->uvpitch,src->uvpitch*2,dst->v+dst->uvpitch,dst->uvpitch*2,dst->uvpitch,Coded_Picture_Height>>2,2);
	}
}

void CMPEG2Decoder::Copyoddeven(YV12PICT *odd, YV12PICT *even, YV12PICT *dst)
{
	cpy_oddeven(odd->y,odd->ypitch*2,even->y+even->ypitch,even->ypitch*2,dst->y,dst->ypitch,Coded_Picture_Height>>1,0);
	cpy_oddeven(odd->u,odd->uvpitch*2,even->u+even->uvpitch,even->uvpitch*2,dst->u,dst->uvpitch,Coded_Picture_Height>>2,1);
	cpy_oddeven(odd->v,odd->uvpitch*2,even->v+even->uvpitch,even->uvpitch*2,dst->v,dst->uvpitch,Coded_Picture_Height>>2,2);
}
