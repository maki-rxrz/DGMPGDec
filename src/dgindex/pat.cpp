/* 
 *	Copyright (C) 2004, Donald A Graft, All Rights Reserved
 *
 *  PAT/PMT table parser for PID detection.
 *	
 *  TABLE is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  TABLE is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with TABLE; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "windows.h"
#include "pat.h"
#include "resource.h"
#include "global.h"

PATParser::PATParser()
{
}

int PATParser::DumpRaw(HWND hDialog, char *filename)
{
	return AnalyzeRaw(hDialog, filename, 0, NULL);
}

int PATParser::AnalyzeRaw(HWND hDialog, char *filename, unsigned int audio_pid, unsigned int *audio_type)
{
	FILE *fin;
	unsigned char byte;
	unsigned int i, pid = 0;
	unsigned char stream_id;
	int afc, pkt_count;
	unsigned char buffer[188];
	int read, pes_offset;
	char listbox_line[255], description[80];
#define MAX_PIDS 500
#define MAX_PACKETS 100000
	struct
	{
		unsigned int pid;
		unsigned char stream_id;
	} Pids[MAX_PIDS];

	// Open the input file for reading.
	if ((fin = fopen(filename, "rb")) == NULL)
	{
		if (hDialog != NULL)
		{
			sprintf(listbox_line, "Cannot open the input file!");
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
		return 1;
	}

	// Find a sync byte.
	for (i = 0; i < LIMIT; i++)
	{
		if (fread(&byte, 1, 1, fin) == 0)
		{
			fclose(fin);
			return 1;
		}
		if (byte == TS_SYNC_BYTE)
		{
			fseek(fin, 187, SEEK_CUR);
			if (fread(&byte, 1, 1, fin) == 0)
			{
				fclose(fin);
				return 1;
			}
			if (byte == TS_SYNC_BYTE)
			{
				fseek(fin, -189, SEEK_CUR);
				break;
			}
			fseek(fin, -188, SEEK_CUR);
		}
	}
	if (i == LIMIT)
	{
		fclose(fin);
		return 1;
	}

	// Initialize the PID data table.
	for (i = 0; i < MAX_PIDS; i++)
	{
		Pids[i].pid = 0xffffffff;
		Pids[i].stream_id = 0;
	}
	// Process the transport packets looking for PIDs.
	pkt_count = 0;
	while ((pkt_count++ < MAX_PACKETS) && (read = fread(buffer, 1, 188, fin)) == 188)
	{
		// Pick up the PID.
		pid = ((buffer[1] & 0x1f) << 8) | buffer[2];
		stream_id = 0;
		// Check payload_unit_start_indicator.
		if (buffer[1] & 0x40)
		{
			// Look for a payload.
			afc = (buffer[3] & 0x30) >> 4;
			if (afc == 1 || afc == 3)
			{
				// Skip the adaptation field (if present).
				if (afc == 1)
				{
					pes_offset = 4;
				}
				else
				{
					pes_offset = 5 + buffer[4];
				}
				// Get the stream ID if there is a packet start code.
				if (buffer[pes_offset] == 0 && buffer[pes_offset+1] == 0 &&buffer[pes_offset+2] == 1)
					stream_id = buffer[pes_offset+3];
			}
			// Find an empty table entry and enter this PID if we haven't already.
			for (i = 0; Pids[i].pid != 0xffffffff && i < MAX_PIDS; i++)
			{
				if (Pids[i].pid == pid) break;
			}
			if (Pids[i].pid == 0xffffffff)
			{
				Pids[i].pid = pid;
				Pids[i].stream_id = stream_id;
			}
		}
	}

	// Display the detection results in the dialog box.
	for (i = 0; Pids[i].pid != 0xffffffff && i < MAX_PIDS; i++)
	{
		if (Pids[i].pid == 0)
			strcpy(description, "PAT");
		else if (Pids[i].pid == 1)
			strcpy(description, "CAT");
		else if (Pids[i].pid >= 2 && Pids[i].pid <= 0xf)
			strcpy(description, "Reserved");
		else if (Pids[i].pid == 0x1fff)
			strcpy(description, "Null");
		else if (Pids[i].stream_id == 0xbc)
			strcpy(description, "Program Stream Map");
		else if (Pids[i].stream_id == 0xbd)
		{
			if (audio_type != NULL && Pids[i].pid == audio_pid)
			{
				*audio_type = 0x81;
				fclose(fin);
				return 0;
			}
			strcpy(description, "Private Stream 1 (AC3 Audio)");
		}
		else if (Pids[i].stream_id == 0xbe)
			strcpy(description, "Padding Stream");
		else if (Pids[i].stream_id == 0xbf)
			strcpy(description, "Private Stream 2");
		else if (((Pids[i].stream_id & 0xe0) == 0xc0) ||
				 (Pids[i].stream_id == 0xfa))
		{
			if (audio_type != NULL && Pids[i].pid == audio_pid)
			{
				// The demuxing code is the same for MPEG and AAC.
				// The only difference will be the filename.
				// The demuxing code will look at the audio sync word to
				// decide between the two.
				*audio_type = 0x4;
				fclose(fin);
				return 0;
			}
			strcpy(description, "MPEG1/MPEG2/AAC Audio");
		}
		else if ((Pids[i].stream_id & 0xf0) == 0xe0)
			strcpy(description, "MPEG Video");
		else if (Pids[i].stream_id == 0xf0)
			strcpy(description, "ECM Stream");
		else if (Pids[i].stream_id == 0xf1)
			strcpy(description, "EMM Stream");
		else if (Pids[i].stream_id == 0xf9)
			strcpy(description, "Ancillary Stream");
		else if (Pids[i].stream_id == 0xff)
			strcpy(description, "Program Stream Directory");
		else
			strcpy(description, "Other");
		if (hDialog != NULL)
		{
			sprintf(listbox_line, "0x%x: %s", Pids[i].pid, description);
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
	}

	fclose(fin);
	return 0;
}

int PATParser::DumpPAT(HWND hDialog, char *filename)
{
	return AnalyzePAT(hDialog, filename, 0, NULL);
}

int PATParser::GetAudioType(char *filename, unsigned int audio_pid)
{
	unsigned int audio_type = 0xffffffff;

	if ((AnalyzePAT(NULL, filename, audio_pid, &audio_type) == 0) && (audio_type != 0xffffffff))
		return audio_type;
	else if ((AnalyzeRaw(NULL, filename, audio_pid, &audio_type) == 0) && (audio_type != 0xffffffff))
		return audio_type;
	else
		return -1;
}

int PATParser::AnalyzePAT(HWND hDialog, char *filename, unsigned int audio_pid, unsigned int *audio_type)
{
	FILE *fin;
	unsigned char byte;
	unsigned int i, pid, ndx, begin, length, number, last, program, pmtpid;
	unsigned int entry, descriptors_length, es_descriptors_length, pcrpid;
	unsigned char buffer[188], type;
	char *stream_type;
	int read;
	char listbox_line[255];
	int pkt_count;

	num_pat_entries = 0;
	first_pat = first_pmt = true;

	// Open the input file for reading.
	if ((fin = fopen(filename, "rb")) == NULL)
	{
		if (hDialog != NULL)
		{
			sprintf(listbox_line, "Cannot open the input file!");
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
		return 1;
	}

	// Find a sync byte.
	for (i = 0; i < LIMIT; i++)
	{
		if (fread(&byte, 1, 1, fin) == 0)
		{
			fclose(fin);
			return 1;
		}
		if (byte == TS_SYNC_BYTE)
		{
			fseek(fin, 187, SEEK_CUR);
			if (fread(&byte, 1, 1, fin) == 0)
			{
				fclose(fin);
				return 1;
			}
			if (byte == TS_SYNC_BYTE)
			{
				fseek(fin, -189, SEEK_CUR);
				break;
			}
			fseek(fin, -188, SEEK_CUR);
		}
	}
	if (i == LIMIT)
	{
		fclose(fin);
		return 1;
	}

	// Process the transport packets.
	pkt_count = 0;
	while ((pkt_count++ < MAX_PACKETS) && (read = fread(buffer, 1, 188, fin)) == 188)
	{
		pid = ((buffer[1] & 0x1f) << 8) | buffer[2];
		if (pid != 0) continue;

		// We have a PAT packet.
		// Check error indicator.
		if (buffer[1] & 0x80) continue;

		// Check for presence of a section start.
		if ((first_pat == true) && !(buffer[1] & 0x40)) continue;

		// Skip if there is no payload.
		byte = (unsigned char) ((buffer[3] & 0x30) >> 4);
		if (byte == 0 || byte == 2) continue;

		// Skip the adaptation field (if any).
		ndx = 4;
		if (byte == 3)
		{
			ndx += buffer[ndx];
		}

		// Skip to the start of the section.
		ndx += buffer[ndx] + 1;

		// Now pointing to the start of the section. Check the table id.
		if (buffer[ndx++] != 0) continue;

		// Check the section syntax indicator.
		if ((buffer[ndx] & 0xc0) != 0x80) continue;

		// Check and get section length.
		if ((buffer[ndx] & 0x0c) != 0) continue;
		length = ((buffer[ndx++] & 0x03) << 8);
		length |= buffer[ndx++];
		if (length > 0x3fd) continue;

		// Skip the transport stream id.
		ndx += 2;

		// We want only current tables.
		if (!(buffer[ndx++] & 0x01)) continue;

		// Get the number of this section.
		number = buffer[ndx++];

		// Get the number of the last section.
		last = buffer[ndx++];

		// Now we have the program/pid tuples.
		for (i = 0; i < length - 9;)
		{
			program = buffer[ndx+i++] << 8;
			program |= buffer[ndx+i++];
			pmtpid = (buffer[ndx+i++] & 0x1f) << 8;
			pmtpid |= buffer[ndx+i++];
			// Skip the Network Information Table (NIT).
			if (program != 0)
			{
				pat_entries[num_pat_entries].program = program;
				pat_entries[num_pat_entries++].pmtpid = pmtpid;
			}
		}

		first_pat = false;

		// If this is the last section number, we're done.
		if (number == last) break;
	}
	// Exit if we didn't find the PAT.
	if (first_pat == true)
	{
		fclose(fin);
		return 1;
	}

	// Now we have to get the PMT tables to retrieve the actual
	// program PIDs.
	for (entry = 0; entry < num_pat_entries; entry++)
	{
		// Start at the beginning of the file.
		fseek(fin, 0, SEEK_SET);
		// Find a sync byte.
		for (i = 0; i < LIMIT; i++)
		{
			if (fread(&byte, 1, 1, fin) == 0)
			{
				fclose(fin);
				return 1;
			}
			if (byte == TS_SYNC_BYTE)
			{
				fseek(fin, 187, SEEK_CUR);
				if (fread(&byte, 1, 1, fin) == 0)
				{
					fclose(fin);
					return 1;
				}
				if (byte == TS_SYNC_BYTE)
				{
					fseek(fin, -189, SEEK_CUR);
					break;
				}
				fseek(fin, -188, SEEK_CUR);
			}
		}

		// Process the transport packets.
		while ((read = fread(buffer, 1, 188, fin)) == 188)
		{
			pid = ((buffer[1] & 0x1f) << 8) | buffer[2];
			if (pid != pat_entries[entry].pmtpid) continue;

			// We have a PMT packet.
			// Check error indicator.
			if (buffer[1] & 0x80) continue;

			// Check for presence of a section start.
			if ((first_pmt == true) && !(buffer[1] & 0x40)) continue;

			// Skip if there is no payload.
			byte = (unsigned char) ((buffer[3] & 0x30) >> 4);
			if (byte == 0 || byte == 2) continue;

			// Skip the adaptation field (if any).
			ndx = 4;
			if (byte == 3)
			{
				ndx += buffer[ndx] + 1;
			}

			// Skip to the start of the section.
			ndx += buffer[ndx] + 1;

			// Now pointing to the start of the section. Check the table id.
			if (buffer[ndx++] != 2) continue;

			// Check the section syntax indicator.
			if ((buffer[ndx] & 0xc0) != 0x80) continue;

			// Check and get section length.
			if ((buffer[ndx] & 0x0c) != 0) continue;
			length = ((buffer[ndx++] & 0x03) << 8);
			length |= buffer[ndx++];
			if (length > 0x3fd) continue;
			begin = ndx;

			// Skip the program number
			if (hDialog != NULL)
			{
				sprintf(listbox_line, "Program %d", pat_entries[entry].program);
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
			}
			ndx += 2;

			// We want only current tables.
			if (!(buffer[ndx++] & 0x01)) continue;

			// Get the number of this section.
			number = buffer[ndx++];

			// Get the number of the last section.
			last = buffer[ndx++];

			// Get the PCRPID.
			pcrpid = (buffer[ndx++] & 0x1f) << 8;
			pcrpid |= buffer[ndx++];
			if (hDialog != NULL)
			{
				sprintf(listbox_line, "    PCR on PID 0x%x", pcrpid); 
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
			}

			// Skip the descriptors (if any).
			descriptors_length = (buffer[ndx++] & 0x0f) << 8;
			descriptors_length |= buffer[ndx++];
			ndx += descriptors_length;

			// Now we have the actual program data.
			while (ndx < begin + length - 4)
			{
				switch (type = buffer[ndx++])
				{
				case 0x01:
					stream_type = "MPEG1 Video";
					break;
				case 0x02:
					stream_type = "MPEG2 Video";
					break;
				case 0x03:
					stream_type = "MPEG1 Audio";
					break;
				case 0x04:
					stream_type = "MPEG2 Audio";
					break;
				case 0x05:
					stream_type = "Private Sections";
					break;
				case 0x06:
				case 0x07:
					stream_type = "Teletext/Subtitling";
					break;
				case 0x0f:
				case 0x11:
					// The demuxing code is the same for MPEG and AAC.
					// The only difference will be the filename.
					// The demuxing code will look at the audio sync word to
					// decide between the two.
					type = 0x04;
					stream_type = "AAC Audio";
					break;
				case 0x81:
					stream_type = "AC3 Audio";
					break;
				default:
					stream_type = "Other";
					break;
				}
				pid = (buffer[ndx++] & 0x1f) << 8;
				pid |= buffer[ndx++];
				if (hDialog != NULL)
				{
					sprintf(listbox_line, "    %s on PID 0x%x", stream_type, pid);
					SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
				}
				if (audio_type != NULL && pid == audio_pid)
				{
					*audio_type = type;
					fclose(fin);
					return 0;
				}

				// Skip the ES descriptors.
				es_descriptors_length = (buffer[ndx++] & 0x0f) << 8;
				es_descriptors_length |= buffer[ndx++];
				ndx += es_descriptors_length;				
			}

			first_pmt = false;

			// If this is the last section number, we're done.
			if (number == last) break;
		}
		if (read != 188)
		{
			fclose(fin);
			return 1;
		}
	}

	fclose(fin);
	return 0;
}
