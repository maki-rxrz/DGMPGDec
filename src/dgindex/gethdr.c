/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include "global.h"
#include "getbit.h"

static int load_intra_quantizer_matrix;
static int load_non_intra_quantizer_matrix;
static int load_chroma_intra_quantizer_matrix;
static int load_chroma_non_intra_quantizer_matrix;
static int frame_rate_code;

static double frame_rate_Table[16] = 
{
	0.0,
	((24.0*1000.0)/1001.0),
	24.0,
	25.0,
	((30.0*1000.0)/1001.0),
	30.0,
	50.0,
	((60.0*1000.0)/1001.0),
	60.0,

	-1,		// reserved
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

__forceinline static void group_of_pictures_header(void);
__forceinline static void picture_header(__int64);

static void sequence_extension(void);
static void sequence_display_extension(void);
static void quant_matrix_extension(void);
static void picture_display_extension(void);
static void picture_coding_extension(void);
static void copyright_extension(void);
static int  extra_bit_information(void);
static void extension_and_user_data(void);

/* decode headers from one input stream */
int Get_Hdr()
{
	int code;
	__int64 position;

	for (;;)
	{
		// Pick up the index location to use for the picture header.
		// Do it before executing next_start_code() because that may
		// force Fill_Next(), which could encounter a new transport
		// packet and thus invalidate our index location.
		position = PackHeaderPosition;

		// Look for next_start_code.
		next_start_code();

		code = Get_Bits(32);
		switch (code)
		{
			case SEQUENCE_HEADER_CODE:
				sequence_header();
				break;

			case GROUP_START_CODE:
				group_of_pictures_header();
				break;

			case PICTURE_START_CODE:
				picture_header(position);
				return 1;

			default:
				break;
		}
	}
}

/* decode sequence header */
void sequence_header()
{
	int constrained_parameters_flag;
	int bit_rate_value;
	int vbv_buffer_size;
	int i;

	horizontal_size             = Get_Bits(12);
	vertical_size               = Get_Bits(12);
	aspect_ratio_information    = Get_Bits(4);
	frame_rate_code             = Get_Bits(4);
	bit_rate_value              = Get_Bits(18);
	Flush_Buffer(1);	// marker bit
	vbv_buffer_size             = Get_Bits(10);
	constrained_parameters_flag = Get_Bits(1);

	if (load_intra_quantizer_matrix = Get_Bits(1))
	{
		for (i=0; i<64; i++)
			intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
	}
	else
	{
		for (i=0; i<64; i++)
			intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
	}

	if (load_non_intra_quantizer_matrix = Get_Bits(1))
	{
		for (i=0; i<64; i++)
			non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
	}
	else
	{
		for (i=0; i<64; i++)
			non_intra_quantizer_matrix[i] = 16;
	}

	/* copy luminance to chrominance matrices */
	for (i=0; i<64; i++)
	{
		chroma_intra_quantizer_matrix[i] = intra_quantizer_matrix[i];
		chroma_non_intra_quantizer_matrix[i] = non_intra_quantizer_matrix[i];
	}

	extension_and_user_data();
}

/* decode group of pictures header */
/* ISO/IEC 13818-2 section 6.2.2.6 */
int closed_gop;
static void group_of_pictures_header()
{
	int gop_hour;
	int gop_minute;
	int gop_sec;
	int gop_frame;

	int drop_flag;
	int broken_link;

	drop_flag   = Get_Bits(1);
	gop_hour    = Get_Bits(5);
	gop_minute  = Get_Bits(6);
	Flush_Buffer(1);	// marker bit
	gop_sec     = Get_Bits(6);
	gop_frame	= Get_Bits(6);
	closed_gop  = Get_Bits(1);
	broken_link = Get_Bits(1);

//	extension_and_user_data();
}

/* decode picture header */
/* ISO/IEC 13818-2 section 6.2.3 */
static void picture_header(__int64 start)
{
	int temporal_reference;
	int vbv_delay;
	int full_pel_forward_vector;
	int forward_f_code;
	int full_pel_backward_vector;
	int backward_f_code;
	int Extra_Information_Byte_Count;
	int trackpos;
	__int64 position;

	if (SystemStream_Flag)
		position = start;
	else
	{
		position = _telli64(Infile[File_Flag])
			- (__int64)BUFFER_SIZE  + (__int64)Rdptr - (__int64)Rdbfr - 12 + ((32 - (__int64)BitsLeft) / 8);
	}
	temporal_reference  = Get_Bits(10);
	picture_coding_type = Get_Bits(3);

	d2v_current.type = picture_coding_type;

	if (d2v_current.type == I_TYPE)
	{
		d2v_current.file = process.startfile = File_Flag;
		process.startloc = _telli64(Infile[File_Flag]);
		d2v_current.lba = process.startloc/BUFFER_SIZE - 1;
		trackpos = (int)((process.run+d2v_current.lba*BUFFER_SIZE)*TRACK_PITCH/process.total);
		SendMessage(hTrack, TBM_SETPOS, (WPARAM)true, trackpos);

		// Defensive programming. This should never happen.
		if (d2v_current.lba < 0) d2v_current.lba = 0;

		if (process.locate==LOCATE_RIP)
		{
			process.file = d2v_current.file;
			process.lba = d2v_current.lba;
		}

		// This triggers if we reach the right marker position.
		if (File_Flag==process.endfile && process.startloc>=process.endloc)		// D2V END
		{
			Fault_Flag = 97;
			Write_Frame(NULL, d2v_current, 0);
		}

		if (Info_Flag)
			UpdateInfo();
	}

	vbv_delay = Get_Bits(16);

	if (picture_coding_type == P_TYPE || picture_coding_type == B_TYPE)
	{
		full_pel_forward_vector = Get_Bits(1);
		forward_f_code = Get_Bits(3);
	}

	if (picture_coding_type == B_TYPE)
	{
		full_pel_backward_vector = Get_Bits(1);
		backward_f_code = Get_Bits(3);
	}

	Extra_Information_Byte_Count = extra_bit_information();
	extension_and_user_data();

	// Indexing for the D2V file.
	if (picture_coding_type == I_TYPE && picture_structure != BOTTOM_FIELD) 
		d2v_current.position = position;
}

/* decode slice header */
/* ISO/IEC 13818-2 section 6.2.4 */
int slice_header()
{
	int slice_vertical_position_extension;
	int quantizer_scale_code;
	int slice_picture_id_enable = 0;
	int slice_picture_id = 0;
	int extra_information_slice = 0;

	slice_vertical_position_extension = vertical_size>2800 ? Get_Bits(3) : 0;

	quantizer_scale_code = Get_Bits(5);
	quantizer_scale = q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] : quantizer_scale_code<<1;

	if (Get_Bits(1))
	{
		Get_Bits(1);	// intra slice

		slice_picture_id_enable = Get_Bits(1);
		slice_picture_id = Get_Bits(6);

		extra_information_slice = extra_bit_information();
	}

	return slice_vertical_position_extension;
}

/* decode extension and user data */
/* ISO/IEC 13818-2 section 6.2.2.2 */
static void extension_and_user_data()
{
	int code, ext_ID;

	next_start_code();

	while ((code = Show_Bits(32))==EXTENSION_START_CODE || code==USER_DATA_START_CODE)
	{
		if (code==EXTENSION_START_CODE)
		{
			Flush_Buffer(32);
			ext_ID = Get_Bits(4);

			switch (ext_ID)
			{
				case SEQUENCE_EXTENSION_ID:
					sequence_extension();
					break;
				case SEQUENCE_DISPLAY_EXTENSION_ID:
					sequence_display_extension();
					break;
				case QUANT_MATRIX_EXTENSION_ID:
					quant_matrix_extension();
					break;
				case PICTURE_DISPLAY_EXTENSION_ID:
					picture_display_extension();
					break;
				case PICTURE_CODING_EXTENSION_ID:
					picture_coding_extension();
					break;
				case COPYRIGHT_EXTENSION_ID:
					copyright_extension();
					break;
			}
			next_start_code();
		}
		else
		{
			Flush_Buffer(32);	// ISO/IEC 13818-2  sections 6.3.4.1 and 6.2.2.2.2
			next_start_code();	// skip user data
		}
	}
}

/* decode sequence extension */
/* ISO/IEC 13818-2 section 6.2.2.3 */
static void sequence_extension()
{
	int profile_and_level_indication;
	int low_delay;
	int frame_rate_extension_n;
	int frame_rate_extension_d;

	int horizontal_size_extension;
	int vertical_size_extension;
	int bit_rate_extension;
	int vbv_buffer_size_extension;

	profile_and_level_indication = Get_Bits(8);
	progressive_sequence         = Get_Bits(1);
	chroma_format                = Get_Bits(2);
	horizontal_size_extension    = Get_Bits(2);
	vertical_size_extension      = Get_Bits(2);
	bit_rate_extension           = Get_Bits(12);
	Flush_Buffer(1);	// marker bit
	vbv_buffer_size_extension    = Get_Bits(8);
	low_delay                    = Get_Bits(1);
 
	frame_rate_extension_n       = Get_Bits(2);
	frame_rate_extension_d       = Get_Bits(5);
	frame_rate = (float)(frame_rate_Table[frame_rate_code] * (frame_rate_extension_n+1)/(frame_rate_extension_d+1));

	horizontal_size = (horizontal_size_extension<<12) | (horizontal_size&0x0fff);
	vertical_size = (vertical_size_extension<<12) | (vertical_size&0x0fff);
}

/* decode sequence display extension */
static void sequence_display_extension()
{
	int video_format;  
	int color_description;
	int color_primaries;
	int transfer_characteristics;
	int matrix_coefficients;
	int display_horizontal_size;
	int display_vertical_size;

	video_format      = Get_Bits(3);
	color_description = Get_Bits(1);

	if (color_description)
	{
		color_primaries          = Get_Bits(8);
		transfer_characteristics = Get_Bits(8);
		matrix_coefficients      = Get_Bits(8);
	}

	display_horizontal_size = Get_Bits(14);
	Flush_Buffer(1);	// marker bit
	display_vertical_size   = Get_Bits(14);
}

/* decode quant matrix entension */
/* ISO/IEC 13818-2 section 6.2.3.2 */
static void quant_matrix_extension()
{
	int i;

	if (load_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
				= intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (load_non_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
				= non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (load_chroma_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (load_chroma_non_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
}

/* decode picture display extension */
/* ISO/IEC 13818-2 section 6.2.3.3. */
static void picture_display_extension()
{
	int frame_center_horizontal_offset[3];
	int frame_center_vertical_offset[3];

	int i;
	int number_of_frame_center_offsets;

	/* based on ISO/IEC 13818-2 section 6.3.12 
	   (November 1994) Picture display extensions */

	/* derive number_of_frame_center_offsets */
	if (progressive_sequence)
	{
		if (repeat_first_field)
		{
			if (top_field_first)
				number_of_frame_center_offsets = 3;
			else
				number_of_frame_center_offsets = 2;
		}
		else
			number_of_frame_center_offsets = 1;
	}
	else
	{
		if (picture_structure!=FRAME_PICTURE)
			number_of_frame_center_offsets = 1;
		else
		{
			if (repeat_first_field)
				number_of_frame_center_offsets = 3;
			else
				number_of_frame_center_offsets = 2;
		}
	}

	/* now parse */
	for (i=0; i<number_of_frame_center_offsets; i++)
	{
		frame_center_horizontal_offset[i] = Get_Bits(16);
		Flush_Buffer(1);	// marker bit

		frame_center_vertical_offset[i] = Get_Bits(16);
		Flush_Buffer(1);	// marker bit
	}
}

/* decode picture coding extension */
static void picture_coding_extension()
{
	int chroma_420_type;
	int progressive_frame;
	int composite_display_flag;
	int v_axis;
	int field_sequence;
	int sub_carrier;
	int burst_amplitude;
	int sub_carrier_phase;

	f_code[0][0] = Get_Bits(4);
	f_code[0][1] = Get_Bits(4);
	f_code[1][0] = Get_Bits(4);
	f_code[1][1] = Get_Bits(4);

	intra_dc_precision			= Get_Bits(2);
	picture_structure			= Get_Bits(2);
	top_field_first				= Get_Bits(1);
	frame_pred_frame_dct		= Get_Bits(1);
	concealment_motion_vectors	= Get_Bits(1);
	q_scale_type				= Get_Bits(1);
	intra_vlc_format			= Get_Bits(1);
	alternate_scan				= Get_Bits(1);
	repeat_first_field			= Get_Bits(1);
	chroma_420_type				= Get_Bits(1);
	progressive_frame			= Get_Bits(1);
	composite_display_flag		= Get_Bits(1);

	d2v_current.pf = progressive_frame;
	d2v_current.trf = (top_field_first<<1) + repeat_first_field;

	if (composite_display_flag)
	{
		v_axis            = Get_Bits(1);
		field_sequence    = Get_Bits(3);
		sub_carrier       = Get_Bits(1);
		burst_amplitude   = Get_Bits(7);
		sub_carrier_phase = Get_Bits(8);
	}
}

/* decode extra bit information */
/* ISO/IEC 13818-2 section 6.2.3.4. */
static int extra_bit_information()
{
	int Byte_Count = 0;

	while (Get_Bits(1))
	{
		Flush_Buffer(8);
		Byte_Count++;
	}

	return Byte_Count;
}

/* Copyright extension */
/* ISO/IEC 13818-2 section 6.2.3.6. */
/* (header added in November, 1994 to the IS document) */
static void copyright_extension()
{
	int copyright_flag;
	int copyright_identifier;
	int original_or_copy;
	int copyright_number_1;
	int copyright_number_2;
	int copyright_number_3;

	int reserved_data;

	copyright_flag =       Get_Bits(1); 
	copyright_identifier = Get_Bits(8);
	original_or_copy =     Get_Bits(1);
  
	/* reserved */
	reserved_data = Get_Bits(7);

	Flush_Buffer(1); // marker bit
	copyright_number_1 =   Get_Bits(20);
	Flush_Buffer(1); // marker bit
	copyright_number_2 =   Get_Bits(22);
	Flush_Buffer(1); // marker bit
	copyright_number_3 =   Get_Bits(22);
}
