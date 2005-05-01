// DGFix 1.0.0
// Fix a D2V file by correcting illegal transitions.
// Illegal transitions are requests for two consecutive
// top or bottom fields.
// Author: Donald Graft, neuron2@comcast.net
//
// Run from a command window.
// Usage: dgfix file.d2v
// The corrected file will be file.d2v.fixed
//
// This works with DGMPGDec 1.2.1.

#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	FILE *fp, *wfp;
	char line[2048], prev_line[2048], wfile[2048], *p, *q;
	int val, mval, prev_val, mprev_val, fix;
	bool first, found;
	int D2Vformat = 0;

	printf("DGFix 1.3.0 by Donald A. Graft\n\n");
	if (argc < 2)
	{
		printf("Usage: DGFix d2v_file\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == 0)
	{
		printf("Cannot open %s\n", argv[1]);
		return 1;
	}
	// Pick up the D2V format number
	fgets(line, 1024, fp);
	if (strncmp(line, "DGIndexProjectFile", 18) != 0)
	{
		printf("%s is not a DGIndex project file\n", argv[1]);
		fclose(fp);
		return 1;
	}
	sscanf(line, "DGIndexProjectFile%d", &D2Vformat);

	strcpy(wfile, argv[1]);
	strcat(wfile,".fixed");
	wfp = fopen(wfile, "w");
	if (wfp == 0)
	{
		printf("Cannot open %s\n", wfile);
		fclose(fp);
		return 1;
	}
	fputs(line, wfp);
	
	while (fgets(line, 1024, fp) != 0)
	{
		fputs(line, wfp);
		if (strncmp(line, "Location", 8) == 0) break;
	}
	fgets(line, 1024, fp);
	fputs(line, wfp);
	fgets(line, 1024, fp);
	prev_line[0] = 0;
	prev_val = -1;
	mprev_val = -1;
	found = false;
	do
	{
		p = line;
		while (*p++ != ' ');
		while (*p++ != ' ');
		while (*p++ != ' ');
		while (*p++ != ' ');
		while (*p++ != ' ');
		while (*p++ != ' ');
		first = true;
		while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f'))
		{
			fix = -1;
			sscanf(p, "%x", &val);
			if (val == 0xff) break;
			// Isolate the TFF/RFF bits.
			mval = val & 0x3;
			if (prev_val >= 0) mprev_val = prev_val & 0x3;
			// Detect illegal transitions.
			if (mval == 2 && mprev_val == 0)      fix = 1;
			else if (mval == 3 && mprev_val == 0) fix = 1;
			else if (mval == 0 && mprev_val == 1) fix = 0;
			else if (mval == 1 && mprev_val == 1) fix = 0;
			else if (mval == 0 && mprev_val == 2) fix = 3;
			else if (mval == 1 && mprev_val == 2) fix = 3;
			else if (mval == 2 && mprev_val == 3) fix = 2;
			else if (mval == 3 && mprev_val == 3) fix = 2;
			// Correct the illegal transition.
			if (fix >= 0)
			{
				found = true;
				printf("Illegal transition: %x -> %x\n", mprev_val, mval);
				printf(prev_line);
				printf(line);
				if (first == false)
				{
					q = p;
					while (*q-- != ' ');
				}
				else
				{
					q = prev_line;
					while (*q != '\n') q++;
					while (!((*q >= '0' && *q <= '9') || (*q >= 'a' && *q <= 'f'))) q--;
				}
				*q = fix + '0';
				printf("corrected...\n");
				printf(prev_line);
				printf(line);
				printf("\n");
			}
			while (*p != ' ' && *p != '\n') p++;
			p++;
			prev_val = val;
			first = false;
		}
		fputs(prev_line, wfp);
		strcpy(prev_line, line);
	} while ((fgets(line, 2048, fp) != 0) &&
			 ((line[0] >= '0' && line[0] <= '9') || (line[0] >= 'a' && line[0] <= 'f')));
	fputs(prev_line, wfp);
	fputs(line, wfp);
	while (fgets(line, 2048, fp) != 0) fputs(line, wfp);
	fclose(fp);
	fclose(wfp);
	if (found == false)
	{
		printf("No errors found.\n");
		unlink(wfile);
	}

	return 0;
}
