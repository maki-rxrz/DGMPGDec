// ParseD2V by Donald A. Graft
// Released under the Gnu Public Licence (GPL)

#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	FILE *fp;
	char line[2048], *p;
	int i, fill, val, prev_val = -1, ndx = 0, count = 0, fdom = -1;
	int D2Vformat = 0;
	int vob, cell;
	char render[128], temp[20];

 	printf("DGParse 1.3.0 by Donald A. Graft\n\n");
	if (argc != 2)
	{
		printf("Usage: dgparse file\n");
		return 1;
	}
	fp = fopen(argv[1], "r");
	if (fp == 0)
	{
		printf("Cannot open %s\n", argv[1]);
		return 1;
	}
	// Pick up the D2V format number
	fgets(line, 2048, fp);
	if (strncmp(line, "DGIndexProjectFile", 18) != 0)
	{
		printf("%s is not a DGIndex project file\n", argv[1]);
		fclose(fp);
		return 1;
	}
	sscanf(line, "DGIndexProjectFile%d", &D2Vformat);
	// Skip past the D2V header info to the GOP lines.
	while (fgets(line, 2048, fp) != 0)
	{
		if (strncmp(line, "Location", 8) == 0) break;
	}
	while (fgets(line, 2048, fp) != 0)
	{
		if (line[0] != '\n') break;
	}
	printf("Encoded Frame: Display Frames....Flags Byte (* means in 3:2 pattern)\n");
	printf("--------------------------------------------------------------------\n");
	// Process the GOP lines.
	do
	{
		// Skip to the TFF/RFF flags entries.
		p = line;
		while (*p++ != ' ');
		while (*p++ != ' ');
		while (*p++ != ' ');
		while (*p++ != ' ');
		sscanf(p, "%d", &vob);
		while (*p++ != ' ');
		sscanf(p, "%d", &cell);
		while (*p++ != ' ');
		printf("[GOP START]\n");
		// Process the flags entries.
		while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f'))
		{
			sscanf(p, "%x", &val);
			if (val == 0xff)
			{
				printf("[EOF]\n");
				break;
			}
			// Isolate the TFF/RFF bits.
			val &= 0x3;
			// Determine field dominance.
			if (fdom == -1)
			{
				fdom = (val >> 1) & 1;
				printf("[CLIP IS %s]\n", fdom ? "TFF" : "BFF");
			}

			// Show encoded-to-display mapping.
			if ((count % 2) && (val == 1 || val == 3))
			{
				sprintf(render, "%d: %d,%d,%d", ndx, ndx + count/2, ndx + count/2 + 1, ndx + count/2 + 1);
			}
			else if ((count % 2) && !(val == 1 || val == 3))
			{
				sprintf(render, "%d: %d,%d", ndx, ndx + count/2, ndx + count/2 + 1);
			}
			else if (!(count % 2) && (val == 1 || val == 3))
			{
				sprintf(render, "%d: %d,%d,%d", ndx, ndx + count/2, ndx + count/2, ndx + count/2 + 1);
			}
			else if (!(count % 2) && !(val == 1 || val == 3))
			{
				sprintf(render, "%d: %d,%d", ndx, ndx + count/2, ndx + count/2);
			}
			fill = 32 - strlen(render);
			for (i = 0; i < fill; i++) strcat(render, ".");
			sprintf(temp, "%x", val);
			printf("%s%s", render, temp);

			// Show vob cell id.
//			printf(" [vob/cell=%d/%d]", vob, cell);

			// Print warnings for 3:2 breaks and illegal transitions.
			if ((prev_val >= 0) && ((val == 0 && prev_val == 3) || (val != 0 && val == prev_val + 1)))
			{
					printf(" *");
			}

			if (prev_val >= 0)
			{
				if ((val == 2 && prev_val == 0) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 3 && prev_val == 0) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 0 && prev_val == 1) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 1 && prev_val == 1) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 0 && prev_val == 2) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 1 && prev_val == 2) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 2 && prev_val == 3) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
				else if ((val == 3 && prev_val == 3) || (val == 2 && prev_val == 0))
					printf(" [ILLEGAL TRANSITION!]");
			}

			printf("\n");

			// Count the encoded frames.
			ndx++;
			// Count the number of pulled-down fields.
			if (val == 1 || val == 3) count++;
			// Move to the next flags entry.
			while (*p != ' ' && *p != '\n') p++;
			p++;
			prev_val = val;
		}
	} while ((fgets(line, 2048, fp) != 0) &&
			 ((line[0] >= '0' && line[0] <= '9') || (line[0] >= 'a' && line[0] <= 'f')));

	return 0;
}
