/* 
 *	Copyright (C) 2004, Donald A Graft, All Rights Reserved
 *
 *  Command line PAT/PMT table parser for PID detection.
 *  Usage: dgtable filename
 *	
 *  DGTABLE is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DGTABLE is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with DGTABLE; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "pat.h"

PATParser::PATParser()
{
	num_pat_entries = 0;
	first_pat = first_pmt = true;
}

int PATParser::DumpPAT(char *filename)
{
	FILE *fin;
	unsigned char byte;
	unsigned int i, pid, ndx, begin, length, number, last, program, pmtpid;
	unsigned int entry, descriptors_length, es_descriptors_length, pcrpid;
	unsigned char buffer[188];
	char *stream_type;
	int read;

	// Open the input file for reading.
	if ((fin = fopen(filename, "rb")) == NULL)
	{
		printf("Could not open input file!\n");
		return 1;
	}

	// Find a sync byte.
	for (i = 0; i < LIMIT; i++)
	{
		if (fread(&byte, 1, 1, fin) == 0)
		{
			fclose(fin);
			printf("Could not find packet sync!\n");
			return 1;
		}
		if (byte == TS_SYNC_BYTE)
		{
			fseek(fin, 187, SEEK_CUR);
			if (fread(&byte, 1, 1, fin) == 0)
			{
				printf("Could not find packet sync!\n");
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
		// The PAT/PMT are on PID 0.
		pid = ((buffer[1] & 0x1f) << 8) | buffer[2];
		if (pid != 0) continue;

		// We have a PAT/PMT packet. We'll start by looking for the PAT.
		// Check error indicator.
		if (buffer[1] & 0x80) continue;

		// Check for presence of a section start.
		if ((first_pat == true) && !(buffer[1] & 0x40)) continue;

		// Skip if there is no payload.
		byte = (buffer[3] & 0x30) >> 4;
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
		// The PAT is table id 0, so skip if it's not a PAT section.
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
			pat_entries[num_pat_entries].program = program;
			pat_entries[num_pat_entries++].pmtpid = pmtpid;
		}

		first_pat = false;

		// If this is the last section number, we're done.
		if (number == last) break;
	}
	if (read != 188)
	{
		fclose(fin);
		printf("Could not find packet sync!\n");
		return 1;
	}

	// Now we have to get the PMT tables to retrieve the actual
	// program PIDs.
	for (entry = 0; entry < num_pat_entries; entry++)
	{
		// Find a sync byte.
		for (i = 0; i < LIMIT; i++)
		{
			if (fread(&byte, 1, 1, fin) == 0)
			{
				fclose(fin);
				printf("Could not find packet sync!\n");
				return 1;
			}
			if (byte == TS_SYNC_BYTE)
			{
				fseek(fin, 187, SEEK_CUR);
				if (fread(&byte, 1, 1, fin) == 0)
				{
					fclose(fin);
					printf("Could not find packet sync!\n");
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
			byte = (buffer[3] & 0x30) >> 4;
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
			// PMT sections should have table id 2.
			if (buffer[ndx++] != 2) continue;

			// Check the section syntax indicator.
			if ((buffer[ndx] & 0xc0) != 0x80) continue;

			// Check and get section length.
			if ((buffer[ndx] & 0x0c) != 0) continue;
			length = ((buffer[ndx++] & 0x03) << 8);
			length |= buffer[ndx++];
			if (length > 0x3fd) continue;
			begin = ndx;

			// Skip the program number.
			printf("Program %d\n", pat_entries[entry].program);
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
			printf("    PCR on PID 0x%x\n", pcrpid); 

			// Skip the descriptors (if any).
			descriptors_length = (buffer[ndx++] & 0x0f) << 8;
			descriptors_length |= buffer[ndx++];
			ndx += descriptors_length;

			// Now we have the actual program data.
			while (ndx < begin + length - 4)
			{
				switch (buffer[ndx++])
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
				case 0x81:
					stream_type = "AC3 Audio";
					break;
				default:
					stream_type = "Other";
					break;
				}
				pid = (buffer[ndx++] & 0x1f) << 8;
				pid |= buffer[ndx++];
				printf("    %s on PID 0x%x\n", stream_type, pid);

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
			printf("Could not find packet sync!\n");
			return 1;
		}
	}

	fclose(fin);
	return 0;
}

int main(int argc, char *argv[])
{
	PATParser parser;

	printf("--------------------------------------------------\n");
	printf("DGTable 1.1.0 -- PAT/PMT Parser by Donald A. Graft\n");
	printf("--------------------------------------------------\n\n");
	if (argc < 2)
	{
		printf("Usage: dgtable filename\n");
		return 1;
	}
	parser.DumpPAT(argv[1]);
	return 0;
}

