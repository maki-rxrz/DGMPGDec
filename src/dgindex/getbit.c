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
#include "AC3Dec\ac3.h"

int _donread(int fd, void *buffer, unsigned int count)
{
	int bytes;
	bytes = _read(fd, buffer, count);
	return bytes;
}

int PTSDifference(unsigned int apts, unsigned int vpts, int *result)
{
	int diff;

	if (apts > vpts)
	{
		diff = (apts - vpts) / 90;
//		if (diff > 5000) return 1;
		*result = diff;
	}
	else
	{
		diff = (vpts - apts) / 90;
//		if (diff > 5000) return 1;
		*result = -diff;
	}
	return 0;
}

#define LOCATE												\
while (Rdptr >= (Rdbfr + BUFFER_SIZE))						\
{															\
	Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);	\
	if (Read < BUFFER_SIZE) Next_File();					\
	Rdptr -= BUFFER_SIZE;									\
}

#define DECODE_AC3																\
{																				\
	size = 0;																	\
	while (Packet_Length > 0)													\
	{																			\
		if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
		{																		\
			size = ac3_decode_data(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, size);		\
			Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);				\
			if (Read < BUFFER_SIZE) Next_File();								\
			Rdptr = Rdbfr;														\
		}																		\
		else																	\
		{																		\
			size = ac3_decode_data(Rdptr, Packet_Length, size);					\
			Rdptr += Packet_Length;												\
			Packet_Length = 0;													\
		}																		\
	}																			\
}

#define DEMUX_AC3															\
while (Packet_Length > 0)													\
{																			\
	if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
	{																		\
		fwrite(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, 1, ac3[This_Track].file);	\
		Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
		Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);				\
		if (Read < BUFFER_SIZE) Next_File();								\
		Rdptr = Rdbfr;														\
	}																		\
	else																	\
	{																		\
		fwrite(Rdptr, Packet_Length, 1, ac3[This_Track].file);				\
		Rdptr += Packet_Length;												\
		Packet_Length = 0;													\
	}																		\
}

#define DEMUX_PCM														\
{																		\
	unsigned char tmp;													\
	size = 0;															\
	while (Packet_Length > 0)											\
	{																	\
		if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)					\
		{																\
			memcpy(PCM_Buffer+size, Rdptr, BUFFER_SIZE+Rdbfr-Rdptr);	\
			size += BUFFER_SIZE+Rdbfr-Rdptr;							\
			Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;					\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);		\
			if (Read < BUFFER_SIZE) Next_File();						\
			Rdptr = Rdbfr;												\
		}																\
		else															\
		{																\
			memcpy(PCM_Buffer+size, Rdptr, Packet_Length);				\
			Rdptr += Packet_Length;										\
			size += Packet_Length;										\
			Packet_Length = 0;											\
		}																\
	}																	\
	for (i=0; i<size; i+=2)												\
	{																	\
		tmp = PCM_Buffer[i];											\
		PCM_Buffer[i] = PCM_Buffer[i+1];								\
		PCM_Buffer[i+1] = tmp;											\
	}																	\
}

#define DEMUX_MPA_AAC(fp)														\
while (Packet_Length > 0)													\
{																			\
	if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
	{																		\
		fwrite(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, 1, (fp));					\
		Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
		Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);				\
		if (Read < BUFFER_SIZE) Next_File();								\
		Rdptr = Rdbfr;														\
	}																		\
	else																	\
	{																		\
		fwrite(Rdptr, Packet_Length, 1, (fp));								\
		Rdptr += Packet_Length;												\
		Packet_Length = 0;													\
	}																		\
}

#define DEMUX_DTS															\
while (Packet_Length > 0)													\
{																			\
	if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
	{																		\
		fwrite(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, 1, dts[This_Track].file);	\
		Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
		Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);			\
		if (Read < BUFFER_SIZE) Next_File();								\
		Rdptr = Rdbfr;														\
	}																		\
	else																	\
	{																		\
		fwrite(Rdptr, Packet_Length, 1, dts[This_Track].file);				\
		Rdptr += Packet_Length;												\
		Packet_Length = 0;													\
	}																		\
}

static char *FTType[5] = {
	"48KHz", "44.1KHz", "44.1KHz", "44.1KHz", "44.1KHz"
};

static char *AC3ModeDash[8] = {
	"1+1", "1_0", "2_0", "3_0", "2_1", "3_1", "2_2", "3_2"
};

static char *AC3Mode[8] = {
	"1+1", "1/0", "2/0", "3/0", "2/1", "3/1", "2/2", "3/2"
};

static int AC3Rate[32] = {
	32, 40, 48, 56, 64, 80, 96, 112, 128, 160,
	192, 224, 256, 320, 384, 448, 512, 576, 640,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

unsigned int VideoPTS, AudioPTS;
static unsigned char PCM_Buffer[BUFFER_SIZE];
static short *ptrPCM_Buffer = (short*)PCM_Buffer;

unsigned char *buffer_invalid;

void Initialize_Buffer()
{
	Rdptr = Rdbfr + BUFFER_SIZE;
	Rdmax = Rdptr;
	buffer_invalid = (unsigned char *) 0xffffffff;

	if (SystemStream_Flag)
	{
		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr = *Rdptr++ << 24;

		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr += *Rdptr++ << 16;

		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr += *Rdptr++ << 8;

		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr += *Rdptr++;

		Fill_Next();
	}
	else
	{
		Fill_Buffer();

		CurrentBfr = (*Rdptr << 24) + (*(Rdptr+1) << 16) + (*(Rdptr+2) << 8) + *(Rdptr+3);
		Rdptr += 4;

		Fill_Next();
	}

	BitsLeft = 32;
}

// Skips ahead in transport stream by specified number of bytes.
#define SKIP_TRANSPORT_PACKET_BYTES(bytes_to_skip)					\
{																	\
	int temp = (bytes_to_skip);										\
	while (temp  > 0) 												\
	{ 																\
		if (temp + Rdptr > BUFFER_SIZE + Rdbfr)						\
		{ 															\
			temp  -= BUFFER_SIZE + Rdbfr - Rdptr;					\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);	\
			if (Read < BUFFER_SIZE) Next_File();					\
			Rdptr = Rdbfr;											\
		} 															\
		else														\
		{ 															\
			Rdptr += temp; 											\
			temp = 0;												\
		} 															\
	}																\
	Packet_Length -= (bytes_to_skip);								\
}

// Transport packet data structure.
typedef struct
{			
	// 1 byte
	unsigned char sync_byte;							// 8 bits

	// 2 bytes
	unsigned char transport_error_indicator;			// 1 bit
	unsigned char payload_unit_start_indicator;			// 1 bit
	unsigned char transport_priority;					// 1 bit
	unsigned short pid;									// 13 bits

	// 1 byte
	unsigned char transport_scrambling_control;			// 2 bits
	unsigned char adaptation_field_control;				// 2 bits
	unsigned char continuity_counter;					// 4 bits

	// 1 byte
	unsigned char adaptation_field_length;				// 8 bits 
	
	// 1 byte
	unsigned char discontinuity_indicator;				// 1 bit
	unsigned char random_access_indicator;				// 1 bit
	unsigned char elementary_stream_priority_indicator;	// 1 bit
	unsigned char PCR_flag;								// 1 bit
	unsigned char OPCR_flag;							// 1 bit
	unsigned char splicing_point_flag;					// 1 bit
	unsigned char transport_private_data_flag;			// 1 bit
	unsigned char adaptation_field_extension_flag;		// 1 bit
} transport_packet;

transport_packet tp_zeroed = { 0 };

FILE *mpafp = NULL;
FILE *mpvfp = NULL;

// ATSC transport stream parser.
// We ignore the 'continuity counter' because with some DTV
// broadcasters, this isnt a reliable indicator.
void Next_Transport_Packet()
{
	static int i, Packet_Length, Packet_Header_Length, size;
	static unsigned int code, flags, This_Track, VOBCELL_Count, AC3_Track, MPA_Track;
	__int64 PES_PTS, PES_DTS;
	unsigned int pts_stamp = 0, dts_stamp = 0;
	int PTSDiff;
	unsigned int bytes_left;
	transport_packet tp;
	unsigned int time, start;
	double picture_period;

	start = timeGetTime();
	for (;;)
	{
		// Don't loop forever. If we don't get data
		// in a reasonable time (2 secs) we exit.
		time = timeGetTime();
		if (time - start > 2000)
		{
			MessageBox(hWnd, "Cannot find audio or video data. Check your PIDs.",
					   NULL, MB_OK | MB_ICONERROR);
			ThreadKill();
		}

		PES_PTS = 0;
		bytes_left = 0;
		Packet_Length = 188; // total length of an MPEG-2 transport packet
		tp = tp_zeroed; // to avoid warnings

		// Search for a sync byte. Gives some protection against emulation.
		for(;;)
		{
			if (Get_Byte() != 0x47) continue;
			if (Rdptr + 187 >= Rdbfr + BUFFER_SIZE && Rdptr[-189] == 0x47)
				break;
			if (Rdptr + 187 < Rdbfr + BUFFER_SIZE && Rdptr[+187] == 0x47)
				break;
			else
				continue;
		}

		// Record the location of the start of the packet. This will be used
		// for indexing when an I frame is detected.
		if (D2V_Flag)
		{
			PackHeaderPosition = _telli64(Infile[CurrentFile])
								 - (__int64)BUFFER_SIZE + (__int64)Rdptr - (__int64)Rdbfr - 1;
		}
		--Packet_Length; // swallow the sync_byte

		code = Get_Short();
		Packet_Length = Packet_Length - 2; // swallow the two bytes we just got
		tp.pid = (unsigned short) (code & 0x1FFF);
		tp.transport_error_indicator = (unsigned char) ((code >> 15) & 0x01);
		tp.payload_unit_start_indicator = (unsigned char) ((code >> 14) & 0x01);
		tp.transport_priority = (unsigned char) ((code >> 13) & 0x01);

		if (tp.transport_error_indicator)
		{
			// Skip remaining bytes in current packet.
			SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
			// Try the next packet in the stream.
			continue;
		}

		code = Get_Byte();
		--Packet_Length; // decrement the 1 byte we just got;
		tp.transport_scrambling_control = (unsigned char) ((code >> 6) & 0x03);//		2	bslbf
		tp.adaptation_field_control = (unsigned char) ((code >> 4 ) & 0x03);//		2	bslbf
		tp.continuity_counter = (unsigned char) (code & 0x0F);//		4	uimsbf

		// we don't care about the continuity counter

		if (tp.adaptation_field_control == 0)
		{
			// no payload
			// skip remaining bytes in current packet
			SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
			continue;
		}

		// 3) check the Adaptation-header, only so we can determine
		//    the exact #bytes to skip
		if ( tp.adaptation_field_control == 2 || tp.adaptation_field_control == 3)
		{
			// adaptation field is present
			tp.adaptation_field_length = (unsigned char) Get_Byte(); // 8-bits
			--Packet_Length; // decrement the 1 byte we just got;

			if ( tp.adaptation_field_length != 0 ) // end of field already?
			{
				// if we made it this far, we no longer need to decrement
				// Packet_Length.  We took care of it up there!
				code = Get_Byte();
				--Packet_Length; // decrement the 1 byte we just got;
				tp.discontinuity_indicator = (unsigned char) ((code >> 7) & 0x01); //	1	bslbf
				tp.random_access_indicator = (unsigned char) ((code >> 6) & 0x01); //	1	bslbf
				tp.elementary_stream_priority_indicator = (unsigned char) ((code >> 5) & 0x01); //	1	bslbf
				tp.PCR_flag = (unsigned char) ((code >> 4) & 0x01); //	1	bslbf
				tp.OPCR_flag = (unsigned char) ((code >> 3) & 0x01); //	1	bslbf
				tp.splicing_point_flag = (unsigned char) ((code >> 2) & 0x01); //	1	bslbf
				tp.transport_private_data_flag = (unsigned char) ((code >> 1) & 0x01); //	1	bslbf
				tp.adaptation_field_extension_flag = (unsigned char) ((code >> 0) & 0x01); //	1	bslbf
				bytes_left = tp.adaptation_field_length - 1;
				// skip the remainder of the adaptation_field
				SKIP_TRANSPORT_PACKET_BYTES( bytes_left )
			} // if ( tp.adaptation_field_length != 0 )
		} // if ( tp.adaptation_field_control != 1 )

		// We've processed the MPEG-2 transport header. 
		// Any data left in the current transport packet?
		if (Packet_Length == 0) continue;

		if (tp.pid && tp.pid == MPEG2_Transport_VideoPID) 
		{
			LOCATE

			code = Get_Short();
			code = (code & 0xffff)<<16 | Get_Short();
			Packet_Length = Packet_Length - 4; // remove these two bytes

			// Packet start?
			if (code < 0x000001E0 || code > 0x000001EF ) 		
			{
				// No, move the buffer-pointer back.
				Rdptr -= 4; 
				Packet_Length = Packet_Length + 4; // restore these four bytes
			}
			else
			{
				Get_Short(); // MPEG2-PES total Packet_Length
				Get_Byte(); // skip a byte
				flags = Get_Byte();
				Packet_Header_Length = Get_Byte();
				Packet_Length = Packet_Length - 5; // compensate the bytes we extracted
	
				// Get timestamp, and skip rest of PES-header.
				if ((flags & 0x80) && (Packet_Header_Length > 4))
				{
					PES_PTS = (Get_Byte() & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					pts_stamp = (unsigned int) (PES_PTS & 0xffffffff);
					Packet_Length -= 5;
					// DTS is not used. The code is here for analysis and debugging.
					if ((flags & 0xc0) == 0xc0)
					{
						PES_DTS = (Get_Byte() & 0x0e) << 29;
						PES_DTS |= (Get_Short() & 0xfffe) << 14;
						PES_DTS |= (Get_Short()>>1) & 0x7fff;
						dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
						Packet_Length -= 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 10)
					}
					else
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 5)
				}
				else
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length);
				if (!Start_Flag)
				{
					// Start_Flag becomes true after the first I frame.
					// So VideoPTS will be left at the value corresponding to
					// the first I frame.
					VideoPTS = pts_stamp;
				}
			} // if ( code != 0x000001E0 )

			Rdmax = Rdptr + Packet_Length;

			Bitrate_Monitor += (Rdmax - Rdptr);
			return;
		}  // if ( (tp.pid == MPEG2_Transport_VideoPID ) ) 

		else if ((Method_Flag == AUDIO_DEMUXALL || Method_Flag == AUDIO_DEMUX) &&
				 (Start_Flag || MPEG2_Transport_VideoPID == 0) &&    // User sets video pid 0 for audio only.
				 tp.pid && (tp.pid == MPEG2_Transport_AudioPID) &&
				 (D2V_Flag || Decision_Flag) &&
				 (MPEG2_Transport_AudioType == 3 || MPEG2_Transport_AudioType == 4 || MPEG2_Transport_AudioType == 0xffffffff)) 
		{
			// Both MPEG and AAC audio come here. The sync word will be checked to
			// distinguish between them.
			if (mpafp)
			{
				if (tp.payload_unit_start_indicator)
				{
					Get_Short(); // start code and stream id
					Get_Short();
					Get_Short(); // packet length
					Get_Byte(); // flags
					code = Get_Byte(); // more flags
					Packet_Header_Length = Get_Byte();
					Packet_Length -= 9;
					if (code & 0x80)
					{
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						Packet_Length = Packet_Length - 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					}
				}
				DEMUX_MPA_AAC(mpafp);
			}
			else if (tp.payload_unit_start_indicator)
			{
				Get_Short(); // start code
				Get_Short(); // rest of start code and stream id
				Get_Short(); // packet length
				Get_Byte(); // flags
				code = Get_Byte(); // more flags
				Packet_Header_Length = Get_Byte();
				Packet_Length -= 9;
				if (code & 0x80)
				{
					code = Get_Byte();
					PES_PTS = (code & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					Packet_Length = Packet_Length - 5;
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);

					// Now we're at the start of the audio.
					// Find the audio header.
					code = Get_Byte();
					code = ((code & 0xff) << 8) | Get_Byte();
					Packet_Length -= 2;
					while ((code & 0xfff8) != 0xfff8 && Packet_Length > 0)
					{
						code = ((code & 0xff) << 8) | Get_Byte();
						Packet_Length--;
					}
					if ((code & 0xfff8) != 0xfff8)
					{
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
						continue;
					}
					// Found the audio header. Now check the layer field.
					// For MPEG, layer is 1, 2, or 3. For AAC, it is 0.
					// We demux the same for both; only the filename we create is different.
					if (((code & 6) >> 1) == 0x00)
					{
						// AAC audio.
						Channel[0] = FORMAT_AAC;

						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							if (picture_structure != FRAME_PICTURE)
								picture_period /= 2;
							VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
						}

						if (MPEG2_Transport_VideoPID == 0 || PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s AAC PID %03x.aac", szOutput, MPEG2_Transport_AudioPID);
						else
							sprintf(szBuffer, "%s AAC PID %03x DELAY %dms.aac", szOutput, MPEG2_Transport_AudioPID, PTSDiff);
					}
					else
					{
						// MPEG audio.
						Channel[0] = FORMAT_MPA;

						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							if (picture_structure != FRAME_PICTURE)
								picture_period /= 2;
							VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
						}

						if (MPEG2_Transport_VideoPID == 0 || PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s MPA PID %03x.mpa", szOutput, MPEG2_Transport_AudioPID);
						else
							sprintf(szBuffer, "%s MPA PID %03x DELAY %dms.mpa", szOutput, MPEG2_Transport_AudioPID, PTSDiff);
					}
					mpafp = fopen(szBuffer, "wb");
					if (mpafp == NULL)
					{
						// Cannot open the output file, Disable further audio processing.
						MPEG2_Transport_AudioType = 0xff;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
						continue;
					}
					Packet_Length += 2;
					Rdptr -= 2;
					DEMUX_MPA_AAC(mpafp);
				}
			}
		}

		else if (tp.pid && tp.pid == MPEG2_Transport_AudioPID && (MPEG2_Transport_AudioType == 0x81)) 
		{
			// We are demuxing AC3 audio.
			// search for an MPEG-PES packet header
			if (tp.random_access_indicator || tp.payload_unit_start_indicator )
			{
				LOCATE

				code = Get_Short();
				code = (code & 0xffff)<<16 | Get_Short();
				Packet_Length = Packet_Length - 4; // remove these two bytes

				// Check for MPEG2-PES packet header. This may contains a PTS.
				if (code != PRIVATE_STREAM_1)
				{
					// No, move the buffer-pointer back.
					Rdptr -= 4; 
					Packet_Length = Packet_Length + 4;
				}
				else
				{
					// YES, pull out PTS 
					//Packet_Length = Get_Short();
					Get_Short(); // MPEG2-PES total Packet_Length
					Get_Byte(); // skip a byte
	
					code = Get_Byte();
					Packet_Header_Length = Get_Byte();
					Packet_Length = Packet_Length - 5; // compensate the bytes we extracted
	
					// get PTS, and skip rest of PES-header
					if (code >= 0x80 && Packet_Header_Length > 4 )
					{
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						Packet_Length = Packet_Length - 5;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);	
						SKIP_TRANSPORT_PACKET_BYTES( Packet_Header_Length-5 )
					}
					else
						SKIP_TRANSPORT_PACKET_BYTES( Packet_Header_Length )
				}
			}

			// Done processing the MPEG-2 PES header, now for the *real* audio-data
			AC3_Track = 0;

			LOCATE
			// if this is AC3_Track's *first* observation, we will seek to the
			// first valid AC3-frame, then decode its header.  
			// We tried checking for tp.payload_unit_start_indicator, but this
			// indicator isn't reliable on a lot of DTV-stations!
			// Instead, we'll manually search for an AC3-sync word.
			if (!ac3[AC3_Track].rip &&
				(Start_Flag || MPEG2_Transport_VideoPID == 0) &&   // User sets video pid 0 for audio only.
				!Channel[AC3_Track] && 
				(tp.random_access_indicator || tp.payload_unit_start_indicator) 
				&& Packet_Length > 5 )
			{

				code = Get_Byte();
				code = (code & 0xff)<<8 | Get_Byte();
				Packet_Length = Packet_Length - 2; // remove these two bytes
				i = 0;

				// search for an AC3-sync word.  We wouldn't have to do this if
				// DTV-stations made proper use of tp.payload_unit_start_indicator;
				while (code!=0xb77 && Packet_Length > 0 )
				{
					code = (code & 0xff)<<8 | Get_Byte();
					--Packet_Length;
					++i;
				}

				if ( code !=0xb77 )  // did we find the sync-header?
				{
					// no, we searched the *ENTIRE* transport-packet and came up empty!
					SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
					continue;  
				}

				// First time that we detected this particular channel? yes
				Channel[AC3_Track] = FORMAT_AC3;

				//Packet_Length = Packet_Length - 5; // remove five bytes
				Get_Short(); 
				ac3[AC3_Track].rate = (Get_Byte()>>1) & 0x1f;
				Get_Byte();
				ac3[AC3_Track].mode = (Get_Byte()>>5) & 0x07;
				//Packet_Length = Packet_Length + 5; // restore these five bytes
				Rdptr -= 5; // restore these five bytes

				// ok, now move "backward" by two more bytes, so we're back at the
				// start of the AC3-sync header

				Packet_Length += 2;
				Rdptr -= 2; 

				if (D2V_Flag || Decision_Flag)
				{
					if (Decision_Flag && AC3_Track==Track_Flag)
					{
						InitialAC3();

						DECODE_AC3

						ac3[AC3_Track].rip = 1;
					}
					else if (Method_Flag==AUDIO_DECODE && AC3_Track==Track_Flag)
					{
						sprintf(szBuffer, "%s AC3 PID %03x T%02d %sch %dKbps %s.wav", szOutput, MPEG2_Transport_AudioPID, Track_Flag+1, 
							AC3ModeDash[ac3[AC3_Track].mode], AC3Rate[ac3[AC3_Track].rate], FTType[SRC_Flag]);

						strcpy(pcm.filename, szBuffer);
						pcm.file = fopen(szBuffer, "wb");

						StartWAV(pcm.file);

						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							if (picture_structure != FRAME_PICTURE)
								picture_period /= 2;
							VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							pcm.delay = 0;
						else
							pcm.delay = PTSDiff * 192;

						if (SRC_Flag)
						{
							DownWAV(pcm.file);
							InitialSRC();
						}

						if (pcm.delay > 0)
						{
							if (SRC_Flag)
								pcm.delay = ((int)(0.91875*pcm.delay)>>2)<<2;

							for (i=0; i<pcm.delay; i++)
								fputc(0, pcm.file);

							pcm.size += pcm.delay;
							pcm.delay = 0;
						}

						InitialAC3();

						DECODE_AC3

						if (-pcm.delay > size)
							pcm.delay += size;
						else
						{
							if (SRC_Flag)
								Wavefs44(pcm.file, size+pcm.delay, AC3Dec_Buffer-pcm.delay);
							else
								fwrite(AC3Dec_Buffer-pcm.delay, size+pcm.delay, 1, pcm.file);

							pcm.size += size+pcm.delay;
							pcm.delay = 0;
						}

						ac3[AC3_Track].rip = 1;
					}
					else if (Method_Flag == AUDIO_DEMUXALL || (Method_Flag==AUDIO_DEMUX && AC3_Track==Track_Flag))
					{
						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							if (picture_structure != FRAME_PICTURE)
								picture_period /= 2;
							VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
						}

						if (MPEG2_Transport_VideoPID == 0 || PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s AC3 PID %03x T%02d %sch %dKbps.ac3", szOutput, MPEG2_Transport_AudioPID, AC3_Track+1, 
								AC3ModeDash[ac3[AC3_Track].mode], AC3Rate[ac3[AC3_Track].rate]);
						else
							sprintf(szBuffer, "%s AC3 PID %03x T%02d %sch %dKbps DELAY %dms.ac3", szOutput, MPEG2_Transport_AudioPID, AC3_Track+1, 
								AC3ModeDash[ac3[AC3_Track].mode], AC3Rate[ac3[AC3_Track].rate], PTSDiff);

						ac3[AC3_Track].file = fopen(szBuffer, "wb");

						DEMUX_AC3

						ac3[AC3_Track].rip = 1;
					} // if (Decision_Flag && AC3_Track==Track_Flag)
				} // if ((Format_Flag==FORMAT_AC3 || !Format_Flag) && (AVI_Flag || D2V_Flag || Decision_Flag))

			} // if (!ac3[AC3_Track].rip && Rip_Flag && !CH[AC3_Track] && tp.)
			else if (ac3[AC3_Track].rip)
			{
				if (Decision_Flag)
					DECODE_AC3
				else if (Method_Flag==AUDIO_DECODE)
				{
					DECODE_AC3

					if (-pcm.delay > size)
						pcm.delay += size;
					else
					{
						if (SRC_Flag)
							Wavefs44(pcm.file, size+pcm.delay, AC3Dec_Buffer-pcm.delay);
						else
							fwrite(AC3Dec_Buffer-pcm.delay, size+pcm.delay, 1, pcm.file);

						pcm.size += size+pcm.delay;
						pcm.delay = 0;
					}
				}
				else
					DEMUX_AC3
			} // else if (ac3[AC3_Track].rip)
		} // if (tp.pid && tp.pid == MPEG2_Transport_AudioPID)

		// fallthrough case
		// skip remaining bytes in current packet
		SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
	} // for(;;)
} // Next_Transport_Packet()

// PVA packet data structure.
typedef struct
{			
	unsigned short sync_byte;
	unsigned char stream_id;
	unsigned char counter;
	unsigned char reserved;
	unsigned char flags;
	unsigned short length;
} pva_packet;

// PVA transport stream parser.
void Next_PVA_Packet()
{
	unsigned int Packet_Length;
	unsigned int time, start;
	pva_packet pva;
	unsigned int code, PTS, PES_PTS, Packet_Header_Length;
	int PTSDiff;
	double picture_period;

	start = timeGetTime();
	for (;;)
	{
		// Don't loop forever. If we don't get data
		// in a reasonable time (1 secs) we exit.
		time = timeGetTime();
		if (time - start > 2000)
		{
			MessageBox(hWnd, "Cannot find video data.", NULL, MB_OK | MB_ICONERROR);
			ThreadKill();
		}

		// Search for a good sync.
		while (true)
		{
			// Sync word is 0x4156.
			if (Get_Byte() != 0x41) continue;
			if (Get_Byte() != 0x56)
			{
				// This byte might be a 0x41, so back up by one.
				Rdptr--;
				continue;
			}
			// To protect against emulation of the sync word,
			// also check that the stream says audio or video.
			pva.stream_id = Get_Byte();
			if (pva.stream_id != 0x01 && pva.stream_id != 0x02)
			{
				// This byte might be a 0x41, so back up by one.
				Rdptr--;
				continue;
			}
			break;
		}

		// Record the location of the start of the packet. This will be used
		// for indexing when an I frame is detected.
		if (D2V_Flag)
		{
			PackHeaderPosition = _telli64(Infile[CurrentFile])
								 - (__int64)BUFFER_SIZE + (__int64)Rdptr - (__int64)Rdbfr - 3;
		}

		// Pick up the remaining packet header fields.
		pva.counter = Get_Byte();
		pva.reserved = Get_Byte();
		pva.flags = Get_Byte();
		pva.length = Get_Byte() << 8;
		pva.length |= Get_Byte();
		Packet_Length = pva.length;
		Rdmax = Rdptr + Packet_Length;

		// Any payload?
		if (Packet_Length == 0 || pva.reserved != 0x55)
			continue;  // No, try the next packet.

		// Check stream id for video.
		if (pva.stream_id == 1) 
		{
			// This is a video packet.
			// Extract the PTS if it exists.
			if (pva.flags & 0x10)
			{
				PTS = (int) ((Get_Byte() << 24) | (Get_Byte() << 16) | (Get_Byte() << 8) | Get_Byte());
				Packet_Length -= 4;
				if (pva.flags & 0x03)
				{
					// Suck up pre-bytes if any.
					int i;
					for (i = 0; i < (pva.flags & 0x03); i ++)
						Get_Byte();
					Packet_Length -= i;
				}
				if (!Start_Flag)
				{
					VideoPTS = PTS;
				}
			}

			// Deliver the video to the ES parsing layer. 
			Bitrate_Monitor += (Rdmax - Rdptr);
			return;
		}
		
		// Check stream id for audio.
		else if ((Method_Flag == AUDIO_DEMUXALL || Method_Flag == AUDIO_DEMUX) &&
				 Start_Flag &&
				 pva.stream_id == 2 &&
				 (D2V_Flag || Decision_Flag))
		{
			// This is an audio packet.
			if (mpafp)
			{
				// For audio, this flag bit means an embedded audio PES packet starts
				// immediately in this PVA packet.
				if (pva.flags & 0x10)
				{
					Get_Short(); // start code and stream id
					Get_Short();
					Get_Short(); // packet length
					Get_Byte(); // flags
					code = Get_Byte(); // more flags
					Packet_Header_Length = Get_Byte();
					Packet_Length -= 9;
					if (code & 0x80)
					{
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						Packet_Length = Packet_Length - 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					}
				}
				DEMUX_MPA_AAC(mpafp);
			}
			else if (pva.flags & 0x10)
			{
				Get_Short(); // start code
				Get_Short(); // rest of start code and stream id
				Get_Short(); // packet length
				Get_Byte(); // flags
				code = Get_Byte(); // more flags
				Packet_Header_Length = Get_Byte();
				Packet_Length -= 9;
				if (code & 0x80)
				{
					code = Get_Byte();
					PES_PTS = (code & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					Packet_Length = Packet_Length - 5;
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					// Now we're at the start of the audio.
					// Find the audio header.
					code = Get_Byte();
					code = ((code & 0xff) << 8) | Get_Byte();
					Packet_Length -= 2;
					while ((code & 0xfff8) != 0xfff8 && Packet_Length > 0)
					{
						code = ((code & 0xff) << 8) | Get_Byte();
						Packet_Length--;
					}
					if ((code & 0xfff8) != 0xfff8)
					{
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
						continue;
					}
					// Found the audio header. Now check the layer field.
					// For MPEG, layer is 1, 2, or 3. For AAC, it is 0.
					// We demux the same for both; only the file name we create is different.
					if (((code & 6) >> 1) == 0x00)
					{
						// AAC audio.
						Channel[0] = FORMAT_AAC;

						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							if (picture_structure != FRAME_PICTURE)
								picture_period /= 2;
							VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s AAC.aac", szOutput);
						else
							sprintf(szBuffer, "%s AAC DELAY %dms.aac", szOutput, PTSDiff);
					}
					else
					{
						// MPEG audio.
						Channel[0] = FORMAT_MPA;

						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							if (picture_structure != FRAME_PICTURE)
								picture_period /= 2;
							VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s MPA.mpa", szOutput);
						else
							sprintf(szBuffer, "%s MPA DELAY %dms.mpa", szOutput, PTSDiff);
					}
					mpafp = fopen(szBuffer, "wb");
					if (mpafp == NULL)
					{
						// Cannot open the output file.
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
						continue;
					}
					// Unread the audio header bytes.
					Packet_Length += 2;
					Rdptr -= 2;
					DEMUX_MPA_AAC(mpafp);
				}
			}	
		}

		// Not an video packet or an audio packet to be demultiplexed. Keep looking.
		SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
	}
}

// MPEG2 program stream parser.
void Next_Packet()
{
	static int i, Packet_Length, Packet_Header_Length, size;
	static unsigned int code, AUDIO_ID, VOBCELL_Count, This_Track, MPA_Track;
	int PTSDiff;
	double picture_period;

	if (SystemStream_Flag == 2)
	{
		Next_Transport_Packet();
		return;
	}
	else if (SystemStream_Flag == 3)
	{
		Next_PVA_Packet();
		return;
	}

	for (;;)
	{
		code = Get_Short();
		code = (code << 16) | Get_Short();

		while ((code & 0xffffff00) != 0x00000100)
			code = (code<<8) + Get_Byte();

		switch (code)
		{
			case PACK_START_CODE:
				if (D2V_Flag)
				{
					PackHeaderPosition = _telli64(Infile[CurrentFile]);
					PackHeaderPosition = PackHeaderPosition - (__int64)BUFFER_SIZE + (__int64)Rdptr - 4 - (__int64)Rdbfr;
				}
				Rdptr += 8;
				VOBCELL_Count = 0;
				break;

			case PRIVATE_STREAM_2:
				Packet_Length = Get_Short();

				if (++VOBCELL_Count==2)
				{
					Rdptr += 25;
					VOB_ID = Get_Short();
					Get_Byte();
					CELL_ID = Get_Byte();
					Rdptr += Packet_Length - 29;

					sprintf(szBuffer, "%d", VOB_ID);
					SetDlgItemText(hDlg, IDC_VOB_ID, szBuffer);

					sprintf(szBuffer, "%d", CELL_ID);
					SetDlgItemText(hDlg, IDC_CELL_ID, szBuffer);
				}
				else
					Rdptr += Packet_Length;
				break;

			case PRIVATE_STREAM_1:
				Packet_Length = Get_Short();

				Rdptr ++;	// +1
				code = Get_Byte();	// +1
				Packet_Header_Length = Get_Byte();	// +1

				if (code>=0x80)
				{
					__int64 PES_PTS;

					PES_PTS = (Get_Byte() & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
//					dprintf("DGIndex:   Audio PTS = %d\n", AudioPTS/90);

					Rdptr += Packet_Header_Length-5;
				}
				else
					Rdptr += Packet_Header_Length;

				AUDIO_ID = Get_Byte();	// +1
				Packet_Length -= Packet_Header_Length+4;

				if (AUDIO_ID>=SUB_AC3 && AUDIO_ID<SUB_AC3+CHANNEL)
				{
					Rdptr += 3; Packet_Length -= 3; This_Track = AUDIO_ID-SUB_AC3;

					LOCATE

					if (!ac3[This_Track].rip && Start_Flag && !Channel[This_Track])
					{
						Channel[This_Track] = FORMAT_AC3;

						code = Get_Byte();
						code = (code & 0xff)<<8 | Get_Byte();
						i = 0;

						while (code!=0xb77)
						{
							code = (code & 0xff)<<8 | Get_Byte();
							i++;
						}

						Get_Short();
						ac3[This_Track].rate = (Get_Byte()>>1) & 0x1f;
						Get_Byte();
						ac3[This_Track].mode = (Get_Byte()>>5) & 0x07;

						Rdptr -= 7; Packet_Length -= i;

						if (D2V_Flag || Decision_Flag)
						{
							if (Decision_Flag && This_Track==Track_Flag)
							{
								InitialAC3();

								DECODE_AC3

								ac3[This_Track].rip = true;
							}
							else if (Method_Flag==AUDIO_DECODE && This_Track==Track_Flag)
							{
								sprintf(szBuffer, "%s AC3 T%02d %sch %dKbps %s.wav", szOutput, This_Track+1, 
									AC3ModeDash[ac3[This_Track].mode], AC3Rate[ac3[This_Track].rate], FTType[SRC_Flag]);

								strcpy(pcm.filename, szBuffer);
								pcm.file = fopen(szBuffer, "wb");

								StartWAV(pcm.file);

								// Adjust the VideoPTS to account for frame reordering.
								if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
								{
									PTSAdjustDone = 1;
									picture_period = 1.0 / frame_rate;
									if (picture_structure != FRAME_PICTURE)
										picture_period /= 2;
									VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
								}

								if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
									pcm.delay = 0;
								else
									pcm.delay = PTSDiff * 192;

								if (SRC_Flag)
								{
									DownWAV(pcm.file);
									InitialSRC();
								}

								if (pcm.delay > 0)
								{
									if (SRC_Flag)
										pcm.delay = ((int)(0.91875*pcm.delay)>>2)<<2;

									for (i=0; i<pcm.delay; i++)
										fputc(0, pcm.file);

									pcm.size += pcm.delay;
									pcm.delay = 0;
								}

								InitialAC3();

								DECODE_AC3

								if (-pcm.delay > size)
									pcm.delay += size;
								else
								{
									if (SRC_Flag)
										Wavefs44(pcm.file, size+pcm.delay, AC3Dec_Buffer-pcm.delay);
									else
										fwrite(AC3Dec_Buffer-pcm.delay, size+pcm.delay, 1, pcm.file);

									pcm.size += size+pcm.delay;
 									pcm.delay = 0;
								}

								ac3[This_Track].rip = true;
							}
							else if (Method_Flag==AUDIO_DEMUXALL  ||  Method_Flag==AUDIO_DEMUX && This_Track==Track_Flag)
							{
								// Adjust the VideoPTS to account for frame reordering.
								if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
								{
									PTSAdjustDone = 1;
									picture_period = 1.0 / frame_rate;
									if (picture_structure != FRAME_PICTURE)
										picture_period /= 2;
									VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
								}

								if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
									sprintf(szBuffer, "%s AC3 T%02d %sch %dKbps.ac3", szOutput, This_Track+1, 
										AC3ModeDash[ac3[This_Track].mode], AC3Rate[ac3[This_Track].rate]);
								else
									sprintf(szBuffer, "%s AC3 T%02d %sch %dKbps DELAY %dms.ac3", szOutput, This_Track+1, 
										AC3ModeDash[ac3[This_Track].mode], AC3Rate[ac3[This_Track].rate], PTSDiff);

//								dprintf("DGIndex: Using Video PTS = %d, Audio PTS = %d [%d], reference = %d, rate = %f\n",
//										VideoPTS/90, AudioPTS/90, PTSDiff, StartTemporalReference, frame_rate);
								ac3[This_Track].file = fopen(szBuffer, "wb");

								DEMUX_AC3

								ac3[This_Track].rip = true;
							}
						}
					}
					else if (ac3[This_Track].rip)
					{
						if (Decision_Flag)
							DECODE_AC3
						else if (Method_Flag==AUDIO_DECODE)
						{
							DECODE_AC3

							if (-pcm.delay > size)
								pcm.delay += size;
							else
							{
								if (SRC_Flag)
									Wavefs44(pcm.file, size+pcm.delay, AC3Dec_Buffer-pcm.delay);
								else
									fwrite(AC3Dec_Buffer-pcm.delay, size+pcm.delay, 1, pcm.file);

								pcm.size += size+pcm.delay;
 								pcm.delay = 0;
							}
						}
						else
						{
							DEMUX_AC3
						}
					}
				}
				else if (AUDIO_ID>=SUB_PCM && AUDIO_ID<SUB_PCM+CHANNEL)
				{
					Rdptr += 6; Packet_Length -= 6; This_Track = AUDIO_ID-SUB_PCM;

					LOCATE

					if (!pcm.rip && Start_Flag && !Channel[This_Track])
					{
						Channel[This_Track] = FORMAT_LPCM;

						if (D2V_Flag)
						{
							if (Method_Flag==AUDIO_DECODE && This_Track==Track_Flag)
							{
								// Adjust the VideoPTS to account for frame reordering.
								if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
								{
									PTSAdjustDone = 1;
									picture_period = 1.0 / frame_rate;
									if (picture_structure != FRAME_PICTURE)
										picture_period /= 2;
									VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
								}

								if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
									pcm.delay = 0;
								else
									pcm.delay = PTSDiff * 192;

								sprintf(szBuffer, "%s LPCM T%02d %s.wav", szOutput, This_Track+1, FTType[SRC_Flag]);
								strcpy(pcm.filename, szBuffer);

								pcm.file = fopen(szBuffer, "wb");
								StartWAV(pcm.file);

								if (SRC_Flag)
								{
									DownWAV(pcm.file);
									InitialSRC();
								}

								if (pcm.delay > 0)
								{
									if (SRC_Flag)
										pcm.delay = ((int)(0.91875*pcm.delay)>>2)<<2;

									for (i=0; i<pcm.delay; i++)
										fputc(0, pcm.file);

									pcm.size += pcm.delay;
									pcm.delay = 0;
								}

								if (-pcm.delay > Packet_Length)
									pcm.delay += Packet_Length;
								else
								{
									DEMUX_PCM

									if (SRC_Flag)
										Wavefs44(pcm.file, size+pcm.delay, PCM_Buffer-pcm.delay);
									else
										fwrite(PCM_Buffer-pcm.delay, size+pcm.delay, 1, pcm.file);
	
									pcm.size += size+pcm.delay;
									pcm.delay = 0;
								}

								pcm.rip = true;
							}
						}
					}
					else if (pcm.rip)
					{
						if (-pcm.delay > Packet_Length)
							pcm.delay += Packet_Length;
						else
						{
							DEMUX_PCM

							if (SRC_Flag)
								Wavefs44(pcm.file, size+pcm.delay, PCM_Buffer-pcm.delay);
							else
							{
								fwrite(PCM_Buffer-pcm.delay, size+pcm.delay, 1, pcm.file);
								
								if (Norm_Flag)
									for (i=0; i<(size>>1); i++)
										if (Sound_Max < abs(ptrPCM_Buffer[i]))
											Sound_Max = abs(ptrPCM_Buffer[i]);
							}

							pcm.size += size+pcm.delay;
							pcm.delay = 0;
						}
					}
				}
				else if (AUDIO_ID>=SUB_DTS && AUDIO_ID<SUB_DTS+CHANNEL)
				{
					Rdptr += 3; Packet_Length -= 3; This_Track = AUDIO_ID-SUB_DTS;

					LOCATE

					if (!dts[This_Track].rip && Start_Flag && !Channel[This_Track])
					{
						Channel[This_Track] = FORMAT_DTS;

						if (D2V_Flag)
						{
							if (Method_Flag==AUDIO_DEMUXALL  ||  Method_Flag==AUDIO_DEMUX && This_Track==Track_Flag)
							{
								// Adjust the VideoPTS to account for frame reordering.
								if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
								{
									PTSAdjustDone = 1;
									picture_period = 1.0 / frame_rate;
									if (picture_structure != FRAME_PICTURE)
										picture_period /= 2;
									VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
								}

								if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
									sprintf(szBuffer, "%s DTS T%02d.dts", szOutput, This_Track+1);
								else
									sprintf(szBuffer, "%s DTS T%02d DELAY %dms.dts", szOutput, This_Track+1, PTSDiff);

								dts[This_Track].file = fopen(szBuffer, "wb");

								DEMUX_DTS

								dts[This_Track].rip = true;
							}
						}
					}
					else if (dts[This_Track].rip)
						DEMUX_DTS
				}
				Rdptr += Packet_Length;
				break;

			case AUDIO_ELEMENTARY_STREAM_7:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_6:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_5:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_4:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_3:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_2:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_1:
				MPA_Track++;
			case AUDIO_ELEMENTARY_STREAM_0:
				Packet_Length = Get_Short()-1;
				code = Get_Byte();

				if ((code & 0xc0)==0x80)
				{
					code = Get_Byte();	// +1
					Packet_Header_Length = Get_Byte();	// +1

					if (code>=0x80)
					{
						__int64 PES_PTS;

						PES_PTS = (Get_Byte() & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);

						Rdptr += Packet_Header_Length-5;
					}
					else
						Rdptr += Packet_Header_Length;

					Packet_Length -= Packet_Header_Length+2;

					LOCATE

					if (!mpa[MPA_Track].rip && Start_Flag && !Channel[MPA_Track])
					{
						Channel[MPA_Track] = FORMAT_MPA;

						code = Get_Byte();
						code = (code & 0xff)<<8 | Get_Byte();
						i = 0;

						while (code<0xfff0)
						{
							code = (code & 0xff)<<8 | Get_Byte();
							i++;
						}

						Rdptr -= 2; Packet_Length -= i;

						if (D2V_Flag)
						{
							if (Method_Flag==AUDIO_DEMUXALL  ||  Method_Flag==AUDIO_DEMUX && MPA_Track==Track_Flag)
							{
								// Adjust the VideoPTS to account for frame reordering.
								if (!PTSAdjustDone && StartTemporalReference != -1 && StartTemporalReference < 18)
								{
									PTSAdjustDone = 1;
									picture_period = 1.0 / frame_rate;
									if (picture_structure != FRAME_PICTURE)
										picture_period /= 2;
									VideoPTS -= (int) (StartTemporalReference * picture_period * 90000);
								}

								if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
									sprintf(szBuffer, "%s MPA T%02d.mpa", szOutput, MPA_Track+1);
								else
									sprintf(szBuffer, "%s MPA T%02d DELAY %dms.mpa", szOutput, MPA_Track+1, PTSDiff);
								mpa[MPA_Track].file = fopen(szBuffer, "wb");

								DEMUX_MPA_AAC(mpa[MPA_Track].file);

								mpa[MPA_Track].rip = true;
							}
						}
					}
					else if (mpa[MPA_Track].rip)
						DEMUX_MPA_AAC(mpa[MPA_Track].file);
				}

				Rdptr += Packet_Length;
				MPA_Track = 0;
				break;

			case VIDEO_ELEMENTARY_STREAM:
				Packet_Length = Get_Short();
				Rdmax = Rdptr + Packet_Length;

				code = Get_Byte();

				if ((code & 0xc0) == 0x80)
				{
					__int64 PES_PTS, PES_DTS;
					unsigned int pts_stamp, dts_stamp;

					code = Get_Byte();
					Packet_Header_Length = Get_Byte();

					if (code >= 0x80)
					{
						PES_PTS = (Get_Byte() & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						pts_stamp = (unsigned int) (PES_PTS & 0xffffffff);
						// DTS is not used. The code is here for analysis and debugging.
						if ((code & 0xc0) == 0xc0)
						{
							PES_DTS = (Get_Byte() & 0x0e) << 29;
							PES_DTS |= (Get_Short() & 0xfffe) << 14;
							PES_DTS |= (Get_Short()>>1) & 0x7fff;
							dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
//							dprintf("DGIndex: Video PTS = %d, Video DTS = %d, [diff=%d]\n",
//									pts_stamp/90, dts_stamp/90, (pts_stamp-dts_stamp)/90);
							Rdptr += Packet_Header_Length - 10;
						}
						else
							Rdptr += Packet_Header_Length - 5;

						if (!Start_Flag)
						{
							// Start_Flag becomes true after the first I frame.
							// So VideoPTS will be left at the value corresponding to
							// the first I frame.
							VideoPTS = pts_stamp;
						}
					}
					else
						Rdptr += Packet_Header_Length;

					Bitrate_Monitor += Rdmax - Rdptr;
					return;
				}
				else
				{
					Rdptr += Packet_Length-1;
				}

				break;

			default:
				if (code>=SYSTEM_START_CODE)
				{
					Packet_Length = Get_Short();
					Rdptr += Packet_Length;
				}
				break;
		}
	}
}

unsigned int Get_Bits_All(unsigned int N)
{
	N -= BitsLeft;
	Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - BitsLeft);

	if (N)
		Val = (Val << N) + (NextBfr >> (32 - N));

	CurrentBfr = NextBfr;
	BitsLeft = 32 - N;
	Fill_Next();

	return Val;
}

void Flush_Buffer_All(unsigned int N)
{
	CurrentBfr = NextBfr;
	BitsLeft = BitsLeft + 32 - N;
	Fill_Next();
}

void Fill_Buffer()
{
	Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);

	if (Read < BUFFER_SIZE)	Next_File();

	Rdptr = Rdbfr;

	if (SystemStream_Flag)
	{
		Rdmax -= BUFFER_SIZE;
	}
	else
		Bitrate_Monitor += Read;
}

void Next_File()
{
	int i, bytes;
	
	if (CurrentFile < NumLoadedFiles-1)
	{
		CurrentFile++;
		process.run = 0;
		for (i=0; i<CurrentFile; i++) process.run += Infilelength[i];
		_lseeki64(Infile[CurrentFile], 0, SEEK_SET);
		bytes = _donread(Infile[CurrentFile], Rdbfr + Read, BUFFER_SIZE - Read);
		if (Read + bytes == BUFFER_SIZE)
			// The whole buffer has valid data.
			buffer_invalid = (unsigned char *) 0xffffffff;
		else
			// Point to the first invalid buffer location.
			buffer_invalid = Rdbfr + Read + bytes;
	}
	else
	{
		buffer_invalid = Rdbfr + Read;
	}
}

void UpdateInfo()
{
	static int Old_NTSC_Purity;
	unsigned int pts;
	int profile, level;
	extern int Clip_Width, Clip_Height, profile_and_level_indication;

	sprintf(szBuffer, "%s", AspectRatio[aspect_ratio_information]);
	SetDlgItemText(hDlg, IDC_ASPECT_RATIO, szBuffer);

	sprintf(szBuffer, "%dx%d", Clip_Width, Clip_Height);
	SetDlgItemText(hDlg, IDC_FRAME_SIZE, szBuffer);

	profile = (profile_and_level_indication >> 4) & 0x7;
	level = profile_and_level_indication & 0xf;
	switch (profile)
	{
	case 1: // high
		strcpy(szBuffer, "high@");
		break;
	case 2: // spatial
		strcpy(szBuffer, "spatial@");
		break;
	case 3: // SNR
		strcpy(szBuffer, "snr@");
		break;
	case 4: // main
		strcpy(szBuffer, "main@");
		break;
	case 5: // simple
		strcpy(szBuffer, "simple@");
		break;
	default:
		sprintf(szBuffer, "esc (%d@%d)", profile, level);
		break;
	}
	switch (level)
	{
	case 4:
		strcat(szBuffer, "high");
		break;
	case 6:
		strcat(szBuffer, "high1440");
		break;
	case 8:
		strcat(szBuffer, "main");
		break;
	case 10:
		strcat(szBuffer, "low");
		break;
	default:
		break;
	}
	sprintf(szBuffer, "%s", szBuffer);
	SetDlgItemText(hDlg, IDC_PROFILE, szBuffer);

	sprintf(szBuffer, "%.3f fps", Frame_Rate);
	SetDlgItemText(hDlg, IDC_FRAME_RATE, szBuffer);

	switch (Channel[SystemStream_Flag == 2 ? 0 : Track_Flag])
	{
		case FORMAT_AC3:
			sprintf(szBuffer, "DD %s %d", AC3Mode[ac3[Track_Flag].mode], AC3Rate[ac3[Track_Flag].rate]);
			SetDlgItemText(hDlg, IDC_AUDIO_TYPE, szBuffer);
			break;

		case FORMAT_MPA:
			SetDlgItemText(hDlg, IDC_AUDIO_TYPE, "MPEG Audio");
			break;

		case FORMAT_AAC:
			SetDlgItemText(hDlg, IDC_AUDIO_TYPE, "AAC Audio");
			break;

		case FORMAT_LPCM:
			SetDlgItemText(hDlg, IDC_AUDIO_TYPE, "Linear PCM");
			break;

		case FORMAT_DTS:
			SetDlgItemText(hDlg, IDC_AUDIO_TYPE, "DTS");
			break;

		default:
			SetDlgItemText(hDlg, IDC_AUDIO_TYPE, "");
			break;
	}

	if (SystemStream_Flag && process.locate != LOCATE_INIT)
	{
		unsigned int hours, mins, secs;

		pts = AudioPTS/90000;
		hours = pts / 3600;
		mins = (pts % 3600) / 60;
		secs = pts % 60;
		sprintf(szBuffer, "%d:%02d:%02d", hours, mins, secs);
		SetDlgItemText(hDlg, IDC_TIMESTAMP, szBuffer);
	}
	else
		SetDlgItemText(hDlg, IDC_TIMESTAMP, "");

	if (process.locate==LOCATE_RIP)
	{
		if (NTSC_Purity || FILM_Purity)
		{
			if (!FILM_Purity)
			{
				if (frame_rate==25)
					sprintf(szBuffer, "PAL");
				else
					sprintf(szBuffer, "NTSC");
			}
			else if (!NTSC_Purity)
				sprintf(szBuffer, "FILM");
			else if (NTSC_Purity > Old_NTSC_Purity)
				sprintf(szBuffer, "NTSC %2d %%", 100 - (FILM_Purity*100)/(FILM_Purity+NTSC_Purity));
			else
				sprintf(szBuffer, "FILM %2d %%", (FILM_Purity*100)/(FILM_Purity+NTSC_Purity));

			Old_NTSC_Purity = NTSC_Purity;
			SetDlgItemText(hDlg, IDC_VIDEO_TYPE, szBuffer);
		}

		if (timing.op)
		{
			unsigned int elapsed, remain;
			float percent;
			timing.ed = timeGetTime();
			elapsed = (timing.ed-timing.op)/1000;
			percent = (float)(100.0*(process.run-process.start+_telli64(Infile[CurrentFile]))/(process.end-process.start));
			remain = (int)((timing.ed-timing.op)*(100.0-percent)/percent)/1000;

			sprintf(szBuffer, "%d:%02d:%02d", elapsed/3600, (elapsed%3600)/60, elapsed%60);
			SetDlgItemText(hDlg, IDC_ELAPSED, szBuffer);

			sprintf(szBuffer, "%d:%02d:%02d", remain/3600, (remain%3600)/60, remain%60);
			SetDlgItemText(hDlg, IDC_REMAIN, szBuffer);
		}
	}
	else if (GetDlgItemText(hDlg, IDC_ELAPSED, szBuffer, 9))
	{
		SetDlgItemText(hDlg, IDC_VIDEO_TYPE, "");
		SetDlgItemText(hDlg, IDC_FRAME_TYPE, "");
		SetDlgItemText(hDlg, IDC_CODED_NUMBER, "");
		SetDlgItemText(hDlg, IDC_PLAYBACK_NUMBER, "");
		SetDlgItemText(hDlg, IDC_BITRATE,"");
		SetDlgItemText(hDlg, IDC_FILE, "");
		SetDlgItemText(hDlg, IDC_FILE_SIZE, "");
		SetDlgItemText(hDlg, IDC_ELAPSED, "");
		SetDlgItemText(hDlg, IDC_REMAIN, "");
		SetDlgItemText(hDlg, IDC_FPS, "");
	}
}
