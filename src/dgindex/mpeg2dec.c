/* 
 *  Mutated into DGIndex. Modifications Copyright (C) 2004, Donald Graft
 * 
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

#include "global.h"
#include "getbit.h"

static BOOL GOPBack(void);
static void InitialDecoder(void);

DWORD WINAPI MPEG2Dec(LPVOID n)
{
	int i = (int) n; // Prevent compiler warning.
	extern int closed_gop, VideoPTS;
	extern FILE *mpafp, *mpvfp;
	__int64 saveloc;
    int field;

__try
{
	Pause_Flag = Stop_Flag = Start_Flag = Fault_Flag = false;
	Frame_Number = Second_Field = 0;
	VOB_ID = CELL_ID = 0;
	Sound_Max = 1; Bitrate_Monitor = 0;

	for (i=0; i<CHANNEL; i++)
	{
		ZeroMemory(&ac3[i], sizeof(AC3Stream));
		ZeroMemory(&mpa[i], sizeof(RAWStream));	
		ZeroMemory(&dts[i], sizeof(RAWStream));
	}

	ZeroMemory(&pcm, sizeof(struct PCMStream));
	ZeroMemory(&Channel, sizeof(Channel));
	mpafp = mpvfp = NULL;

	switch (process.locate)
	{
		case LOCATE_FORWARD:
			process.startfile = process.file;
			process.startloc = (process.lba + 1) * BUFFER_SIZE;

			process.end = Infiletotal - BUFFER_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE - 1) * BUFFER_SIZE;
			break;

		case LOCATE_BACKWARD:
			process.startfile = process.file;
			process.startloc = process.lba * BUFFER_SIZE;
			process.end = Infiletotal - BUFFER_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE - 1) * BUFFER_SIZE;
			if (GOPBack() == false && CurrentFile > 0)
			{
				// Trying to step back to previous VOB file.
				process.startfile--;
				CurrentFile--;
				process.startloc = Infilelength[process.startfile];
				process.run = 0;
				for (i=0; i<process.startfile; i++)
					process.run += Infilelength[i];
				process.start = process.run + process.startloc;
				GOPBack();
			}
			break;

		case LOCATE_RIP:
			process.startfile = process.leftfile;
			process.startloc = process.leftlba * BUFFER_SIZE;
			process.endfile = process.rightfile;
			process.endloc = (process.rightlba - 1) * BUFFER_SIZE;
			goto do_rip_play;
		case LOCATE_PLAY:
			process.startfile = process.file;
			process.startloc = process.lba * BUFFER_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.locate = LOCATE_RIP;
do_rip_play:
			process.run = 0;
			for (i=0; i<process.startfile; i++)
				process.run += Infilelength[i];
			process.start = process.run + process.startloc;

			process.end = 0;
			for (i=0; i<process.endfile; i++)
				process.end += Infilelength[i];
			process.end += process.endloc;
			break;

		case LOCATE_SCROLL:
			CurrentFile = process.startfile;
			_lseeki64(Infile[process.startfile], (process.startloc/BUFFER_SIZE)*BUFFER_SIZE, SEEK_SET);
			Initialize_Buffer();

			timing.op = 0;
			saveloc = process.startloc;

			for (;;)
			{
				Get_Hdr(0);
				if (Stop_Flag == true)
					ThreadKill();
				if (picture_coding_type == I_TYPE)
				{
					process.startloc = saveloc;
					break;
				}
			}
			break;
	}

	// Check validity of the input file and collect some needed information.
	if (!Check_Flag)
	{
		int code;
		int i;
		unsigned int show;

		// Position to start of the first file.
		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();

		// If the file does not contain a sequence header start code, it can't be an MPEG file.
		// We're already byte aligned at the start of the file.
		while ((show = Show_Bits(32)) != 0x1b3)
		{
			if (Stop_Flag)
			{
				// We reached EOF without ever seeing a sequence header.
				MessageBox(hWnd, "No video sequence header found!", NULL, MB_OK | MB_ICONERROR);
				ThreadKill();
			}
			Flush_Buffer(8);
		}

		// Position to start of the first file.
		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();

		// Determine the stream type and the MPEG type.

		// First look for transport streams.
		// Search for a sync byte. Gives some protection against emulation.
		for(i = 0; i < 188; i++)
		{
			if (Get_Byte() != 0x47) continue;
			if (Rdptr + 187 >= Rdbfr + BUFFER_SIZE && Rdptr[-189] == 0x47)
			{
				SystemStream_Flag = TRANSPORT_STREAM;
				mpeg_type = IS_MPEG2;
				is_program_stream = 0;
				break;
			}
			else if (Rdptr + 187 < Rdbfr + BUFFER_SIZE && Rdptr[+187] == 0x47)
			{
				SystemStream_Flag = TRANSPORT_STREAM;
				mpeg_type = IS_MPEG2;
				is_program_stream = 0;
				break;
			}
			else
				continue;
		}

		// Now try for PVA streams. Look for a packet start in the first 1024 bytes.
		if (SystemStream_Flag != TRANSPORT_STREAM)
		{
			CurrentFile = 0;
			_lseeki64(Infile[0], 0, SEEK_SET);
			Initialize_Buffer();

			for(i = 0; i < 1024; i++)
			{
				if (Rdbfr[i] == 0x41 && Rdbfr[i+1] == 0x56 && (Rdbfr[i+2] == 0x01 || Rdbfr[i+2] == 0x02))
				{
					SystemStream_Flag = PVA_STREAM;
					mpeg_type = IS_MPEG2;
					is_program_stream = 0;
					break;
				}
				Rdptr++;
			}
		}

		// Determine whether this is an MPEG2 file and whether it is a program stream.
		if (SystemStream_Flag != TRANSPORT_STREAM && SystemStream_Flag != PVA_STREAM)
		{
			mpeg_type = IS_NOT_MPEG;
			is_program_stream = 0;
			if (initial_parse(Infilename[0], &mpeg_type, &is_program_stream) == -1)
			{
				MessageBox(hWnd, "File open problem!", NULL, MB_OK | MB_ICONERROR);
				return 0;
			}
			if (is_program_stream)
			{
				SystemStream_Flag = PROGRAM_STREAM;
			}
		}

		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();

		// We know the stream type now, so our parsing is immune to
		// emulated video sequence headers in the pack layer.
		while (!Check_Flag)
		{
			// Move to the next video start code.
			next_start_code();
			code = Get_Bits(32);
			switch (code)
			{
			case SEQUENCE_HEADER_CODE:
				if (Stop_Flag == true)
					ThreadKill();
				sequence_header(0);
				InitialDecoder();
				Check_Flag = true;
				break;
			}
		}

		// Determine the number of leading B frames in display order.
		// This will be used to warn the user if required and to
		// adjust the VideoPTS.
		saveloc = process.startloc;
		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();
		while (Get_Hdr(0) && picture_coding_type != I_TYPE)
		LeadingBFrames = 0;
		for (;;)
		{
			if (Get_Hdr(1) == 1)
			{
				// We hit a sequence end code, so there are no
				// more frames.
				break;
			}
			if (picture_coding_type != B_TYPE) break;
			LeadingBFrames++;
		}
		if (LeadingBFrames > 0 && gop_warned == false && closed_gop == 0)
		{
			MessageBox(hWnd, "WARNING! Opening GOP is not closed.\nThe first few frames may not be decoded correctly.", NULL, MB_OK | MB_ICONERROR);
			gop_warned = true;
		}
		// Position back to the first GOP.
		process.startloc = saveloc;
		Stop_Flag = false;
	}

	Frame_Rate = (FO_Flag==FO_FILM) ? frame_rate * 0.8f : frame_rate;

	if (D2V_Flag)
	{
		i = NumLoadedFiles;

		// The first parameter is the format version number.
		// It must be coordinated with the test in DGDecode
		// and used to reject obsolete D2V files.
		fprintf(D2VFile, "DGIndexProjectFile%d\n%d\n", D2V_FILE_VERSION, i);
		while (i)
		{
			fprintf(D2VFile, "%d %s\n", strlen(Infilename[NumLoadedFiles-i]), Infilename[NumLoadedFiles-i]);
			i--;
		}

		fprintf(D2VFile, "\nStream_Type=%d\n", SystemStream_Flag);
		if (SystemStream_Flag == TRANSPORT_STREAM)
			fprintf(D2VFile, "MPEG2_Transport_PID=%x,%x\n", MPEG2_Transport_VideoPID, MPEG2_Transport_AudioPID);

		fprintf(D2VFile, "MPEG_Type=%d\n", mpeg_type == IS_MPEG2 ? 2 : 1);
		fprintf(D2VFile, "iDCT_Algorithm=%d\n", iDCT_Flag);
		fprintf(D2VFile, "YUVRGB_Scale=%d\n", Scale_Flag);
		if (Luminance_Flag)
		{
			fprintf(D2VFile, "Luminance_Filter=%d,%d\n", LumGamma, LumOffset);
		}
		else
		{
			fprintf(D2VFile, "Luminance_Filter=0,0\n");
		}

		if (ClipResize_Flag)
		{
			fprintf(D2VFile, "Clipping=%d,%d,%d,%d\n", Clip_Left, Clip_Right, Clip_Top, Clip_Bottom);
		}
		else
		{
			fprintf(D2VFile, "Clipping=0,0,0,0\n");
		}

		if (mpeg_type == IS_MPEG2)
			fprintf(D2VFile, "Aspect_Ratio=%s\n", AspectRatio[aspect_ratio_information]);
		else
			fprintf(D2VFile, "Aspect_Ratio=%s\n", AspectRatioMPEG1[aspect_ratio_information]);
		fprintf(D2VFile, "Picture_Size=%dx%d\n", Coded_Picture_Width, Coded_Picture_Height);
		fprintf(D2VFile, "Field_Operation=%d\n", FO_Flag);
		fprintf(D2VFile, "Frame_Rate=%d\n", (int)(Frame_Rate*1000));
		fprintf(D2VFile, "Location=%d,%X,%d,%X\n\n", process.leftfile, (int)process.leftlba, 
				process.rightfile, (int)process.rightlba);

		// Default to ITU-709.
		matrix_coefficients = 1;
	}

	// Start normal decoding from the start position.
	StartTemporalReference = -1;
	PTSAdjustDone = 0;
	CurrentFile = process.startfile;
	_lseeki64(Infile[process.startfile], (process.startloc/BUFFER_SIZE)*BUFFER_SIZE, SEEK_SET);
	Initialize_Buffer();

	timing.op = 0;

	for (;;)
	{
		Get_Hdr(0);
		if (Stop_Flag == true)
			ThreadKill();
		if (picture_coding_type == I_TYPE) break;
	}
	Start_Flag = true;
	process.file = d2v_current.file;
	process.lba = d2v_current.lba;

	Decode_Picture();

	timing.op = timing.mi = timeGetTime();

    field = 0;
	for (;;)
	{
		Get_Hdr(0);
		if (Stop_Flag && (!field || picture_structure == FRAME_PICTURE))
		{
			// Stop only after every two fields if we are decoding field pictures.
			Write_Frame(current_frame, d2v_current, Frame_Number - 1);
			ThreadKill();
		}
		Decode_Picture();
		field ^= 1;
		if (Stop_Flag && (!field || picture_structure == FRAME_PICTURE))
		{
			// Stop only after every two fields if we are decoding field pictures.
			Write_Frame(current_frame, d2v_current, Frame_Number - 1);
			ThreadKill();
		}
	}
}
__except (EXCEPTION_EXECUTE_HANDLER)
{
	MessageBox(hWnd, "Caught an exception during decoding! See help file.", NULL, MB_OK | MB_ICONERROR);
	ThreadKill();
}
	return 0;
}

static BOOL GOPBack()
{
	int i;
	int startfile;
	__int64 startloc, endloc, curloc, startlba;

	startfile = process.startfile;
	startloc = process.startloc;
	startlba = process.lba;

	for (;;)
	{
		endloc = startloc;
		startloc -= BUFFER_SIZE<<4;

		if (startloc >= 0)
		{
			process.startfile = startfile;
			process.startloc = startloc;
		}
		else
		{
			process.startloc = 0;
			return false;
		}

		_lseeki64(Infile[process.startfile], process.startloc, SEEK_SET);
		Initialize_Buffer();

		for (;;)
		{
			curloc = _telli64(Infile[process.startfile]);
			if (curloc >= endloc) break;
			Get_Hdr(0);
			if (Stop_Flag == true)
				ThreadKill();
			if (picture_structure != FRAME_PICTURE)
			{
				Get_Hdr(0);
				if (Stop_Flag == true)
					ThreadKill();
			}
			if (picture_coding_type==I_TYPE)
			{
				if (d2v_current.file > startfile)
				{
					// We've crossed back into the next file and found
					// the first I frame. That's not the one we are looking for,
					// so go back again and keep looking.
					CurrentFile = startfile;
					process.run = 0;
					for (i=0; i < startfile; i++)
						process.run += Infilelength[i];
					break;
				}
				if (d2v_current.lba == startlba)
				{
					// We've found the I frame we are trying to step back from.
					// That's not the one we want, so keep looking.
					break;
				}
				// We've found the I frame we want!
				process.startfile = d2v_current.file;
				process.startloc = d2v_current.lba * BUFFER_SIZE;
				// This is a kludge. For PES streams, the pack start
				// might be in the previous LBA!
				if (SystemStream_Flag != ELEMENTARY_STREAM)
					process.startloc -= BUFFER_SIZE;
				return true;
			}
		}
	}
}

static int ChromaFormat[4] = {
	0, 6, 8, 12
};

static void InitialDecoder()
{
	int i, size;
	extern int Clip_Width, Clip_Height;

	mb_width = (horizontal_size+15)/16;
	mb_height = progressive_sequence ? (vertical_size+15)/16 : 2*((vertical_size+31)/32);

	Coded_Picture_Width = 16 * mb_width;
	Coded_Picture_Height = 16 * mb_height;

	Chroma_Width = (chroma_format==CHROMA444) ? Coded_Picture_Width : Coded_Picture_Width>>1;
	Chroma_Height = (chroma_format!=CHROMA420) ? Coded_Picture_Height : Coded_Picture_Height>>1;

	block_count = ChromaFormat[chroma_format];

	for (i=0; i<3; i++)
	{
		if (i==0)
			size = Coded_Picture_Width * Coded_Picture_Height;
		else
			size = Chroma_Width * Chroma_Height;

		backward_reference_frame[i] = (unsigned char*)_aligned_malloc(size, 64);
		forward_reference_frame[i] = (unsigned char*)_aligned_malloc(size, 64);
		auxframe[i] = (unsigned char*)_aligned_malloc(size, 64);
	}

	u422 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height/2, 64);
	v422 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height/2, 64);
	u444 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height, 64);
	v444 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height, 64);
	rgb24 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height*3, 64);
	rgb24small = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height*3, 64);
	yuy2 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height*2, 64);
	lum = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height, 64);
	Clip_Width = Coded_Picture_Width;
	Clip_Height = Coded_Picture_Height;

	CheckDirectDraw();
}
