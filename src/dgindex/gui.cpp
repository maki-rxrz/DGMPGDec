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
#include <windows.h>
#include "resource.h"

#define GLOBAL
extern "C"
{
#include "global.h"
#include "AC3Dec\ac3.h"
#include "pat.h"
}

static char Version[] = "DGIndex 1.4.5";

#define TRACK_HEIGHT	30
#define INIT_WIDTH		480
#define INIT_HEIGHT		270
#define MIN_WIDTH		160
#define MIN_HEIGHT		16

#define MASKCOLOR		RGB(0, 6, 0)

#define	SAVE_D2V		1
#define SAVE_WAV		2
#define	OPEN_D2V		3
#define OPEN_VOB		4
#define OPEN_WAV		5
#define SAVE_BMP		6

#define PRIORITY_HIGH		1
#define PRIORITY_NORMAL		2
#define PRIORITY_LOW		3

bool PopFileDlg(PTSTR, HWND, int);
ATOM MyRegisterClass(HINSTANCE);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Info(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK VideoList(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClipResize(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Luminance(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Speed(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Normalization(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SetPids(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DetectPids(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static DWORD DDColorMatch(LPDIRECTDRAWSURFACE, COLORREF);
static void ShowInfo(bool);
static void ClearTrack(void);
static void CheckFlag(void);
static void Recovery(void);
static void RefreshWindow(bool);
static void SaveBMP(void);
static void OpenVideoFile(HWND);
static void OpenAudioFile(HWND);
DWORD WINAPI ProcessWAV(LPVOID n);

static void StartupEnables(void);
static void FileLoadedEnables(void);
static void RunningEnables(void);

static int WindowMode;
static int INIT_X, INIT_Y, Priority_Flag, Edge_Width, Edge_Height;

static FILE *INIFile;
static char szPath[_MAX_PATH], szTemp[_MAX_PATH], szWindowClass[_MAX_PATH];

static HINSTANCE hInst;
static HANDLE hProcess, hThread;
static HWND hLeftButton, hLeftArrow, hRightArrow, hRightButton;

static DWORD threadId;
static RECT wrect, crect;
static int SoundDelay[MAX_FILE_NUMBER];
static char Outfilename[MAX_FILE_NUMBER][_MAX_PATH];

char *ExitOnEnd;

PATParser pat_parser;
extern "C" int fix_d2v(HWND hWnd, char *path, int silent);
extern "C" int parse_d2v(HWND hWnd, char *path);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HACCEL hAccel;

	int i=0;
	char *ptr,*fptr,*ende;
	char aFName[_MAX_PATH];
	char suffix[_MAX_PATH];
	char *p, *q;
	int val;
	char ucCmdLine[4096];
	char delimiter1[2], delimiter2[2], save;
	char prog[1024];

	// Get the path to the DGIndex executable.
	GetModuleFileName(NULL, ExePath, 255);
	ptr = &ExePath[strlen(ExePath)];
	while (ptr > ExePath && *ptr != '\\') ptr--;
	if (*ptr == '\\') ptr++;
	*ptr = 0;

	// Load INI
	strcpy(prog, ExePath);
	strcat(prog, "DGIndex.ini");
	if ((INIFile = fopen(prog, "r")) == NULL)
	{
NEW_VERSION:
		INIT_X = INIT_Y = 100;

		iDCT_Flag = IDCT_SKAL;
		Scale_Flag = true;
		setRGBValues();
		FO_Flag = FO_NONE;
		Method_Flag = AUDIO_DEMUXALL;
		Track_Flag = 1 << TRACK_1;
		DRC_Flag = DRC_NORMAL;
		DSDown_Flag = false;
		SRC_Flag = SRC_NONE;
		Norm_Ratio = 100;
		Priority_Flag = PRIORITY_NORMAL;
		MPEG2_Transport_VideoPID = 0x02;
		MPEG2_Transport_AudioPID = 0x02;
		PlaybackSpeed = SPEED_NORMAL;
		ForceOpenGops = 0;
	}
	else
	{
		char line[_MAX_PATH], *p;

		fgets(line, _MAX_PATH - 1, INIFile);
		line[strlen(line)-1] = 0;
		p = line;
		while (*p != '=' && *p != 0) p++;
		if (*p == 0 || strcmp(++p, Version))
		{
			fclose(INIFile);
			goto NEW_VERSION;
		}

		fscanf(INIFile, "Window_Position=%d,%d\n", &INIT_X, &INIT_Y);
		if (INIT_X < 0 || INIT_X + 100 > GetSystemMetrics(SM_CXSCREEN) || 
			INIT_Y < 0 || INIT_Y + 100 > GetSystemMetrics(SM_CYSCREEN))
			INIT_X = INIT_Y = 100;

		fscanf(INIFile, "iDCT_Algorithm=%d\n", &iDCT_Flag);
		fscanf(INIFile, "YUVRGB_Scale=%d\n", &Scale_Flag);
		setRGBValues();
		fscanf(INIFile, "Field_Operation=%d\n", &FO_Flag);
		fscanf(INIFile, "Output_Method=%d\n", &Method_Flag);
		fscanf(INIFile, "Track_Number=%d\n", &Track_Flag);
		fscanf(INIFile, "DR_Control=%d\n", &DRC_Flag);
		fscanf(INIFile, "DS_Downmix=%d\n", &DSDown_Flag);
		fscanf(INIFile, "SRC_Precision=%d\n", &SRC_Flag);
		fscanf(INIFile, "Norm_Ratio=%d\n", &Norm_Ratio);
		fscanf(INIFile, "Process_Priority=%d\n", &Priority_Flag);
		fscanf(INIFile, "Transport_PIDs=%x,%x\n", &MPEG2_Transport_VideoPID, &MPEG2_Transport_AudioPID);
		fscanf(INIFile, "Playback_Speed=%d\n", &PlaybackSpeed);
		fscanf(INIFile, "Force_Open_Gops=%d\n", &ForceOpenGops);
		fclose(INIFile);
	}

	// Perform application initialization
	hInst = hInstance;

	// Load accelerators
	hAccel = LoadAccelerators(hInst, (LPCTSTR)IDR_ACCELERATOR);

	// Initialize global strings
	LoadString(hInst, IDC_GUI, szWindowClass, _MAX_PATH);
	MyRegisterClass(hInst);

	hWnd = CreateWindow(szWindowClass, "DGIndex", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME|WS_MAXIMIZEBOX),
		CW_USEDEFAULT, 0, INIT_WIDTH, INIT_HEIGHT, NULL, NULL, hInst, NULL);

	// Test CPU
	__asm
	{
		mov			eax, 1
		cpuid
		test		edx, 0x00800000		// STD MMX
		jz			TEST_SSE
		mov			[cpu.mmx], 1
TEST_SSE:
		test		edx, 0x02000000		// STD SSE
		jz			TEST_SSE2
		mov			[cpu.ssemmx], 1
		mov			[cpu.ssefpu], 1
TEST_SSE2:
		test		edx, 0x04000000		// SSE2	
		jz			TEST_3DNOW
		mov			[cpu.sse2], 1
TEST_3DNOW:
		mov			eax, 0x80000001
		cpuid
		test		edx, 0x80000000		// 3D NOW
		jz			TEST_SSEMMX
		mov			[cpu._3dnow], 1
TEST_SSEMMX:
		test		edx, 0x00400000		// SSE MMX
		jz			TEST_END
		mov			[cpu.ssemmx], 1
TEST_END:
	}

	if (!cpu.sse2)
	{
		DeleteMenu(hMenu, IDM_IDCT_SSE2MMX, 0);
	}

	if (!cpu.ssemmx)
	{
		DeleteMenu(hMenu, IDM_IDCT_SSEMMX, 0);
		DeleteMenu(hMenu, IDM_IDCT_SKAL, 0);
	}

	if (cpu.mmx)
		CheckMenuItem(hMenu, IDM_MMX, MF_CHECKED);
	else
		DestroyWindow(hWnd);

	if (cpu.ssemmx)
		CheckMenuItem(hMenu, IDM_SSEMMX, MF_CHECKED);

	if (cpu.sse2)
		CheckMenuItem(hMenu, IDM_SSE2, MF_CHECKED);

	if (cpu._3dnow)
		CheckMenuItem(hMenu, IDM_3DNOW, MF_CHECKED);

	if (cpu.ssefpu)
		CheckMenuItem(hMenu, IDM_SSEFPU, MF_CHECKED);

	// Create control
	hTrack = CreateWindow(TRACKBAR_CLASS, NULL,
		WS_CHILD | WS_VISIBLE | WS_DISABLED | TBS_ENABLESELRANGE | TBS_NOTICKS | TBS_TOP,
		0, INIT_HEIGHT, INIT_WIDTH-4*TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_TRACKBAR, hInst, NULL);
	SendMessage(hTrack, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, TRACK_PITCH));

	hLeftButton = CreateWindow("BUTTON", "[",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-4*TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_LEFT_BUTTON, hInst, NULL);

	hLeftArrow = CreateWindow("BUTTON", "<",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-3*TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_LEFT_ARROW, hInst, NULL);

	hRightArrow = CreateWindow("BUTTON", ">",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-2*TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_RIGHT_ARROW, hInst, NULL);

	hRightButton = CreateWindow("BUTTON", "]",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_RIGHT_BUTTON, hInst, NULL);

	ResizeWindow(INIT_WIDTH, INIT_HEIGHT);
	MoveWindow(hWnd, INIT_X, INIT_Y, INIT_WIDTH+Edge_Width, INIT_HEIGHT+Edge_Height+TRACK_HEIGHT, true);

	// Command line init.
	strcpy(ucCmdLine, lpCmdLine);
	strupr(ucCmdLine);

	// Show window normal, minimized, or hidden as appropriate.
	if (*lpCmdLine == 0)
		WindowMode = SW_SHOW;
	else
	{
		if (strstr(ucCmdLine,"-MINIMIZE"))
			WindowMode = SW_MINIMIZE;
		else if (strstr(ucCmdLine,"-HIDE"))
			WindowMode = SW_HIDE;
		else
			WindowMode = SW_SHOW;
	}
	ShowWindow(hWnd, WindowMode);

	StartupEnables();
	CheckFlag();
	CLIActive = 0;

	// If command line parameters are present, enter command line interface (CLI) mode.
	if(*lpCmdLine != 0x00)
	{
		int tmp;
		char cwd[1024];

		NumLoadedFiles = 0;
		SystemStream_Flag = ELEMENTARY_STREAM;
		delimiter1[0] = '[';
		delimiter2[0] = ']';
		delimiter1[1] = delimiter2[1] = 0;
		if ((ptr = strstr(ucCmdLine,"-SET-DELIMITER")) || (ptr = strstr(ucCmdLine,"-SD")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr += 4;
			delimiter1[0] = delimiter2[0] = *ptr;
		}
		if ((ptr = strstr(ucCmdLine,"-AUTO-INPUT-FILES")) || (ptr = strstr(ucCmdLine,"-AIF")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			ende = strstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			strcpy(aFName, ptr);
			*ende = save;
			for (;;)
			{
				/* If the specified file does not include a path, use the
				   current directory. */
				if (!strstr(aFName, "\\"))
				{
					GetCurrentDirectory(sizeof(cwd) - 1, cwd);
					strcat(cwd, "\\");
					strcat(cwd, aFName);
				}
				else
				{
					strcpy(cwd, aFName);
				}
				if ((tmp = _open(cwd, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL)) == -1) break;
				strcpy(Infilename[NumLoadedFiles], cwd);
				Infile[NumLoadedFiles] = tmp;
				NumLoadedFiles++;

				// First scan back from the end of the name for an _ character.
				p = aFName+strlen(aFName);
				while (*p != '_' && p >= aFName) p--;
				if (*p != '_') break;
				// Now pick up the number value and increment it.
				p++;
				if (*p < '0' || *p > '9') break;
				sscanf(p, "%d", &val);
				val++;
				// Save the suffix after the number.
				q = p;
				while (*p >= '0' && *p <= '9') p++;
				strcpy(suffix, p);
				// Write the new incremented number.
				sprintf(q, "%d", val);
				// Append the saved suffix.
				strcat(aFName, suffix);
			}
		}
		else if ((ptr = strstr(ucCmdLine,"-INPUT-FILES")) || (ptr = strstr(ucCmdLine,"-IF")))
		{
		  ptr = lpCmdLine + (ptr - ucCmdLine);
		  ptr  = strstr(ptr, delimiter1) + 1;
		  ende = strstr(ptr + 1, delimiter2);
  
		  do
		  {
			i = 0;
			if ((fptr = strstr(ptr, ",")) || (fptr = strstr(ptr + 1, delimiter2)))
			{
			  while (ptr < fptr)
			  {
				aFName[i] = *ptr;
				ptr++;
				i++;
			  }
			  aFName[i] = 0x00;
			  ptr++;
			}
			else
			{
			  strcpy(aFName, ptr);
			  ptr = ende;
			}

			/* If the specified file does not include a path, use the
			   current directory. */
			if (!strstr(aFName, "\\"))
			{
				GetCurrentDirectory(sizeof(cwd) - 1, cwd);
				strcat(cwd, "\\");
				strcat(cwd, aFName);
			}
			else
			{
				strcpy(cwd, aFName);
			}
			if ((tmp = _open(cwd, _O_RDONLY | _O_BINARY)) != -1)
			{
				strcpy(Infilename[NumLoadedFiles], cwd);
				Infile[NumLoadedFiles] = tmp;
				NumLoadedFiles++;
			}
		  }
		  while (ptr < ende);
		}
		else if ((ptr = strstr(ucCmdLine,"-BATCH-FILES")) || (ptr = strstr(ucCmdLine,"-BF")))
		{
			FILE *bf;
			char line[1024];

			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			ende = strstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			strcpy(aFName, ptr);
			*ende = save;
			/* If the specified batch file does not include a path, use the
			   current directory. */
			if (!strstr(aFName, "\\"))
			{
				GetCurrentDirectory(sizeof(cwd) - 1, cwd);
				strcat(cwd, "\\");
				strcat(cwd, aFName);
			}
			else
			{
				strcpy(cwd, aFName);
			}
			bf = fopen(cwd, "r");
			if (bf != 0)
			{
				while (fgets(line, 1023, bf) != 0)
				{
					// Zap the newline.
					line[strlen(line)-1] = 0;
					/* If the specified batch file does not include a path, use the
					   current directory. */
					if (!strstr(line, "\\"))
					{
						GetCurrentDirectory(sizeof(cwd) - 1, cwd);
						strcat(cwd, "\\");
						strcat(cwd, line);
					}
					else
					{
						strcpy(cwd, line);
					}
					if ((tmp = _open(cwd, _O_RDONLY | _O_BINARY)) != -1)
					{
						strcpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;
					}
				}
			}
		}
		Recovery();

		// Transport PIDs
		if ((ptr = strstr(ucCmdLine,"-VIDEO-PID")) || (ptr = strstr(ucCmdLine,"-VP")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			sscanf(strstr(ptr,"=")+1, "%x", &MPEG2_Transport_VideoPID);
		}
		if ((ptr = strstr(ucCmdLine,"-AUDIO-PID")) || (ptr = strstr(ucCmdLine,"-AP")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			sscanf(strstr(ptr,"=")+1, "%x", &MPEG2_Transport_AudioPID);
		}

		//iDCT Algorithm
		if ((ptr = strstr(ucCmdLine,"-IDCT-ALGORITHM")) || (ptr = strstr(ucCmdLine,"-IA")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
 			CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
   
			switch (*(strstr(ptr,"=")+1))
			{
			case '1':
			  iDCT_Flag = IDCT_MMX;
			  break;
			case '2':
			  iDCT_Flag = IDCT_SSEMMX;
			  break;
			default:
			case '3':
			  iDCT_Flag = IDCT_SSE2MMX;
			  break;
			case '4':
			  iDCT_Flag = IDCT_FPU;
			  break;
			case '5':
			  iDCT_Flag = IDCT_REF;
			  break;
			case '6':
			  iDCT_Flag = IDCT_SKAL;
			  break;
			case '7':
			  iDCT_Flag = IDCT_SIMPLE;
			  break;
			}
			CheckFlag();
		}

		//Field-Operation
		if ((ptr = strstr(ucCmdLine,"-FIELD-OPERATION")) || (ptr = strstr(ucCmdLine,"-FO")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			SetDlgItemText(hDlg, IDC_INFO, "");

			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
			  FO_Flag = FO_NONE;
			  CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
			  break;
			case '1':
			  FO_Flag = FO_FILM;
			  CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
			  break;
			case '2':
			  FO_Flag = FO_RAW;
			  CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
			  break;
			}
		}

		//YUV->RGB
		if ((ptr = strstr(ucCmdLine,"-YUV-RGB")) || (ptr = strstr(ucCmdLine,"-YR")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);    
			CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
    
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '1':
			  Scale_Flag = true;
			  setRGBValues();    
			  CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
			  break;
      
			case '2':
			  Scale_Flag = false;
			  setRGBValues();
			  CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
			  break;
			}
		}

		// Luminance filter and clipping not implemented

		//Track number
		if ((ptr = strstr(ucCmdLine,"-TRACK-NUMBER")) || (ptr = strstr(ucCmdLine,"-TN")))
		{
			int tmp;
			Track_Flag = 0;
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr = strstr(ptr,"=") + 1;
			for (;;)
			{
				// Parse TN=x,y,z
				sscanf(ptr, "%d", &tmp);
				if (tmp >= 1 && tmp <= 8)
				{
					Track_Flag |= (1 << (tmp - 1));
				}
				while (*ptr != ',' && *ptr != ' ')
					ptr++;
				if (*ptr == ' ')
					break;
				else
					ptr++;
			}
			ClearTrack();
			for (i = 0; i < 8; i++)
			{
				if (Track_Flag & (1 << i))
					CheckMenuItem(hMenu, IDM_TRACK_1 + i, MF_CHECKED);
			}
			Method_Flag = AUDIO_DEMUX;
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
		}

		// Output Method
		if ((ptr = strstr(ucCmdLine,"-OUTPUT-METHOD")) || (ptr = strstr(ucCmdLine,"-OM")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
				Method_Flag = AUDIO_NONE;
				CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
				break;
      
			case '1':
				Method_Flag = AUDIO_DEMUX;
				CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
				break;
     
			case '2':
				Method_Flag = AUDIO_DEMUXALL;
				CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
				break;
     
			case '3':
				Method_Flag = AUDIO_DECODE;
				CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
				break;
			}
		}

		// Dynamic-Range-Control
		if ((ptr = strstr(ucCmdLine,"-DYNAMIC-RANGE-CONTROL")) || (ptr = strstr(ucCmdLine,"-DRC")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
			  CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
			  DRC_Flag = DRC_NONE;
			  break;
			case '1':
			  CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
			  DRC_Flag = DRC_LIGHT;
			  break;
			case '2':
			  CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
			  DRC_Flag = DRC_NORMAL;
			  break;
			case '3':
			  CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
			  DRC_Flag = DRC_HEAVY;
			  break;
			}
		}

		// Dolby Surround Downmix
		if ((ptr = strstr(ucCmdLine,"-DOLBY-SURROUND-DOWNMIX")) || (ptr = strstr(ucCmdLine,"-DSD")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
			DSDown_Flag = *(strstr(ptr,"=")+1) - '0';
			if (DSDown_Flag)
			  CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);
		}

		// 48 -> 44 kHz
		if ((ptr = strstr(ucCmdLine,"-DOWNSAMPLE-AUDIO")) || (ptr = strstr(ucCmdLine,"-DSA")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
				SRC_Flag = SRC_NONE;
				CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
				break;
			case '1':
				SRC_Flag = SRC_LOW;
				CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
				break;
			case '2':
				SRC_Flag = SRC_MID;
				CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
				break;
			case '3':
				SRC_Flag = SRC_HIGH;
				CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
				break;
			case '4':
				SRC_Flag = SRC_UHIGH;
				CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
				break;
			}
		}

		// Normalization not implemented

		RefreshWindow(true);

		// Output D2V file
		if ((ptr = strstr(ucCmdLine,"-OUTPUT-FILE")) || (ptr = strstr(ucCmdLine,"-OF")) ||
			(ptr = strstr(ucCmdLine,"-OUTPUT-FILE-DEMUX")) || (ptr = strstr(ucCmdLine,"-OFD")))
		{
			// Set up demuxing if requested.
			if (strstr(ucCmdLine,"-OUTPUT-FILE-DEMUX") || strstr(ucCmdLine,"-OFD"))
			{
				MuxFile = (FILE *) 0;
			}

			// Don't pop up warning box for automatic invocation.
			gop_warned = true;
			CLIActive = 1;
			ExitOnEnd = strstr(ucCmdLine,"-EXIT");
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			ende = strstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			strcpy(szOutput, ptr);
			*ende = save;
		}

		if (NumLoadedFiles)
		{
			// Start a LOCATE_INIT thread. When it kills itself, it will start a
			// LOCATE_RIP thread by sending a WM_USER message to the main window.
			process.rightfile = NumLoadedFiles-1;
			process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/BUFFER_SIZE);
			process.end = Infiletotal - BUFFER_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE - 1)*BUFFER_SIZE;
			process.locate = LOCATE_INIT;
			if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
			  hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
		}
	}

	// Main message loop
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

HBITMAP splash = NULL;

// Processes messages for the main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD wmId, wmEvent;

	HDC hdc;
	PAINTSTRUCT ps;
	char prog[1024];
	char path[1024];
	char avsfile[1024];
	LPTSTR ptr;
	FILE *tplate, *avs;

	int i, j;
	char *ext;

	switch (message)
	{
		case SET_WINDOW_TEXT_MESSAGE:
			// This is used to write the completion percentage into the
			// minimized taskbar button.
			if (!IsIconic(hWnd))
			{
				sprintf(szBuffer, "DGIndex - ");
				ext = strrchr(Infilename[CurrentFile], '\\');
				strncat(szBuffer, ext+1, strlen(Infilename[0])-(int)(ext-Infilename[0]));
			}
			SetWindowText(hWnd, szBuffer);
			break;

		case CLI_RIP_MESSAGE:
			// The CLI-invoked LOCATE_INIT thread is finished.
			// Kick off a LOCATE_RIP thread.
			goto proceed;

		case D2V_DONE_MESSAGE:
			// Make an AVS file if it doesn't already exist and a template exists.
			strcpy(prog, ExePath);
			strcat(prog, "template.avs");
			strcpy(avsfile, D2VFilePath);
			ptr = strrchr(avsfile, '.');
			strcpy(++ptr, "avs");
			if (!fopen(avsfile, "r") && (tplate = fopen(prog, "r")))
			{
				avs = fopen(avsfile, "w");
				if (avs)
				{
					while (fgets(path, 1023, tplate))
					{
						if (ptr = strstr(path, "__vid__"))
						{
							ptr[0] = 0;
							strcpy(prog, path);
							strcat(prog, D2VFilePath);
							strcat(prog, ptr+7);
							fputs(prog, avs);
						}
						else if (ptr = strstr(path, "__aud__"))
						{
							ptr[0] = 0;
							strcpy(prog, path);
							strcat(prog, AudioFilePath);
							strcat(prog, ptr+7);
							fputs(prog, avs);
						}
						else
							fputs(path, avs);
					}
					fclose(tplate);
					fclose(avs);
				}
			}
			break;

		case WM_CREATE:
			PreScale_Ratio = 1.0;

			hDC = GetDC(hWnd);
			hMenu = GetMenu(hWnd);
			hProcess = GetCurrentProcess();

			// Load the splash screen from the file splash.bmp if it exists.
			strcpy(prog, ExePath);
			strcat(prog, "splash.bmp");
			splash = (HBITMAP) ::LoadImage (0, prog, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
//			if (splash == 0)
//			{
//				// No splash file. Use the built-in default splash screen.
//				splash = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH));
//			}

			for (i=0; i<MAX_FILE_NUMBER; i++)
				Infilename[i] = (char*)malloc(_MAX_PATH);

			for (i=0; i<8; i++)
				block[i] = (short *)_aligned_malloc(sizeof(short)*64, 64);

			Initialize_FPU_IDCT();

			// register VFAPI
			HKEY key; DWORD trash;

			if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\VFPlugin", 0, "",
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &trash) == ERROR_SUCCESS)
			{
				if (_getcwd(szBuffer, _MAX_PATH)!=NULL)
				{
					if (szBuffer[strlen(szBuffer)-1] != '\\')
						strcat(szBuffer, "\\");

					strcpy(szPath, szBuffer);

					struct _finddata_t vfpfile;
					if (_findfirst("DGVfapi.vfp", &vfpfile) != -1L)
					{
						strcat(szBuffer, "DGVfapi.vfp");

						RegSetValueEx(key, "DGIndex", 0, REG_SZ, (LPBYTE)szBuffer, strlen(szBuffer));
						CheckMenuItem(hMenu, IDM_VFAPI, MF_CHECKED);
					}

					RegCloseKey(key);
				}
			}

		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			// parse the menu selections
			switch (wmId)
			{
				case IDM_OPEN:
					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					DialogBox(hInst, (LPCTSTR)IDD_FILELIST, hWnd, (DLGPROC)VideoList);
					break;

				case IDM_PREVIEW:
				case IDM_PLAY:
					if (!Check_Flag)
					{
						MessageBox(hWnd, "No data. Check your PIDS.", "Preview/Play", MB_OK | MB_ICONWARNING);
					}
					else if (IsWindowEnabled(hTrack))
					{
						RunningEnables();
						Display_Flag = true;
						// Initialize for single stepping.
						RightArrowHit = false;
						ShowInfo(true);

						if (SystemStream_Flag == TRANSPORT_STREAM)
						{
							MPEG2_Transport_AudioType =
										pat_parser.GetAudioType(Infilename[0], MPEG2_Transport_AudioPID);
						}

						if (wmId == IDM_PREVIEW)
							process.locate = LOCATE_RIP;
						else
							process.locate = LOCATE_PLAY;

						if (WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case IDM_SAVE_D2V_AND_DEMUX:
					MuxFile = (FILE *) 0;
					goto proceed;

				case IDM_SAVE_D2V:
					// No video demux.
					MuxFile = (FILE *) 0xffffffff;
proceed:
					if (!Check_Flag)
					{
						MessageBox(hWnd, "No data. Check your PIDS.", "Save Project", MB_OK | MB_ICONWARNING);
						break;
					}
					if ((FO_Flag == FO_FILM) && ((mpeg_type == IS_MPEG1) || ((int) (frame_rate * 1000) != 29970)))
					{
						char buf[255];
						sprintf(buf, "It is almost always wrong to run Force Film on an MPEG1 stream,\n"
							"or a stream with a frame rate other than 29.970.\n"
							"Your stream is %s and has a frame rate of %.3f.\n"
							"Do you want to do it anyway?",
							mpeg_type == IS_MPEG1 ? "MPEG1" : "MPEG2", frame_rate); 
						if (MessageBox(hWnd, buf, "Force Film Warning", MB_YESNO | MB_ICONWARNING) != IDYES)
							break;
					}
					if (CLIActive || PopFileDlg(szOutput, hWnd, SAVE_D2V))
					{
						sprintf(szBuffer, "%s.d2v", szOutput);
						if (CLIActive)
						{
							if ((D2VFile = fopen(szBuffer, "w+")) == 0)
							{
								if (ExitOnEnd) exit (0);
								else CLIActive = 0;
							}
							strcpy(D2VFilePath, szBuffer);
						}
						else 
						{
							if (fopen(szBuffer, "r"))
							{
								char line[255];
								sprintf(line, "%s already exists.\nDo you want to replace it?", szBuffer);
								if (MessageBox(hWnd, line, "Save D2V",
									MB_YESNO | MB_ICONWARNING) != IDYES)
									break;
							}
							D2VFile = fopen(szBuffer, "w+");
							strcpy(D2VFilePath, szBuffer);
						}

						if (D2VFile != 0)
						{
							if (LogQuants_Flag)
							{
								// Initialize quant matric logging.
								// Generate the output file name.
								char *p;
								p = &szBuffer[strlen(szBuffer)];
								while (*p != '.') p--;
								p[1] = 0;
								strcat(p, "quants.txt");
								// Open the output file.
								Quants = fopen(szBuffer, "w");
								// Init the recorded quant matrices for change detection.
								memset(intra_quantizer_matrix_log, 0xffffffff, sizeof(intra_quantizer_matrix_log));
								memset(non_intra_quantizer_matrix_log, 0xffffffff, sizeof(non_intra_quantizer_matrix_log));
								memset(chroma_intra_quantizer_matrix_log, 0xffffffff, sizeof(chroma_intra_quantizer_matrix_log));
								memset(chroma_non_intra_quantizer_matrix_log, 0xffffffff, sizeof(chroma_non_intra_quantizer_matrix_log));
							}

							D2V_Flag = true;
							Display_Flag = false;
							RunningEnables();
							ShowInfo(true);
							// Get the audio type so that we parse correctly for transport streams.
							if (SystemStream_Flag == TRANSPORT_STREAM)
							{
								MPEG2_Transport_AudioType =
											pat_parser.GetAudioType(Infilename[0], MPEG2_Transport_AudioPID);
							}

							process.locate = LOCATE_RIP;
							if (CLIActive || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							{
								hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
							}
						}
						else
							MessageBox(hWnd, "Couldn't write D2V file. Is it read-only?", "Save D2V", MB_OK | MB_ICONERROR);
					}
					break;

				case IDM_LOAD_D2V:
					if (PopFileDlg(szInput, hWnd, OPEN_D2V))
					{
D2V_PROCESS:
						int m, n;
						char line[2048];
						int D2Vformat;

						D2VFile = fopen(szInput, "r");

						// Validate the D2V file.
						fgets(line, 2048, D2VFile);
						if (strncmp(line, "DGIndexProjectFile", 18) != 0)
						{
							MessageBox(hWnd, "The file is not a DGIndex project file!", NULL, MB_OK | MB_ICONERROR);
							fclose(D2VFile);
							break;
						}
						sscanf(line, "DGIndexProjectFile%d", &D2Vformat);
						if (D2Vformat != D2V_FILE_VERSION)
						{
							MessageBox(hWnd, "Obsolete D2V file.\nRecreate it with this version of DGIndex.", NULL, MB_OK | MB_ICONERROR);
							fclose(D2VFile);
							break;
						}

						// Close any opened files.
						while (NumLoadedFiles)
						{
							NumLoadedFiles--;
							_close(Infile[NumLoadedFiles]);
						}

						fscanf(D2VFile, "%d\n", &NumLoadedFiles);

						i = NumLoadedFiles;
						while (i)
						{
							fgets(Infilename[NumLoadedFiles-i], _MAX_PATH - 1, D2VFile);
							// Strip newline.
							Infilename[NumLoadedFiles-i][strlen(Infilename[NumLoadedFiles-i])-1] = 0;
							if ((Infile[NumLoadedFiles-i] = _open(Infilename[NumLoadedFiles-i], _O_RDONLY | _O_BINARY | _O_SEQUENTIAL))==-1)
							{
								while (i<NumLoadedFiles)
								{
									_close(Infile[NumLoadedFiles-i-1]);
									i++;
								}

								NumLoadedFiles = 0;
								break;
							}

							i--;
						}

						Recovery();

						fscanf(D2VFile, "\nStream_Type=%d\n", &SystemStream_Flag);
						if (SystemStream_Flag == TRANSPORT_STREAM)
							fscanf(D2VFile, "MPEG2_Transport_PID=%x,%x\n", &MPEG2_Transport_VideoPID, &MPEG2_Transport_AudioPID);
						fscanf(D2VFile, "MPEG_Type=%d\n", &mpeg_type);
						fscanf(D2VFile, "iDCT_Algorithm=%d (1:MMX 2:SSEMMX 3:FPU 4:REF 5:SSE2MMX)\n", &iDCT_Flag);
						fscanf(D2VFile, "YUVRGB_Scale=%d (0:TVScale 1:PCScale)\n", &Scale_Flag);
						setRGBValues();
						fscanf(D2VFile, "Luminance_Filter=%d,%d (Gamma, Offset)\n", &LumGamma, &LumOffset);

						if (LumGamma || LumOffset)
						{
							CheckMenuItem(hMenu, IDM_LUMINANCE, MF_CHECKED);
							Luminance_Flag = true;				
						}
						else
						{
							CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);
							Luminance_Flag = false;
						}

						fscanf(D2VFile, "Clipping=%d,%d,%d,%d (ClipLeft, ClipRight, ClipTop, ClipBottom)\n", 
								&Clip_Left, &Clip_Right, &Clip_Top, &Clip_Bottom);

						if (Clip_Top || Clip_Bottom || Clip_Left || Clip_Right)
						{
							CheckMenuItem(hMenu, IDM_CLIPRESIZE, MF_CHECKED);
							ClipResize_Flag = true;				
						}
						else
						{
							CheckMenuItem(hMenu, IDM_CLIPRESIZE, MF_UNCHECKED);
							ClipResize_Flag = false;
						}

						fscanf(D2VFile, "Aspect_Ratio=%d:%d\n", &m, &n);
						fscanf(D2VFile, "Picture_Size=%dx%d\n", &m, &n);
						fscanf(D2VFile, "Field_Operation=%d (0:None 1:ForcedFILM 2:RawFrames)\n", &FO_Flag);
						fscanf(D2VFile, "Frame_Rate=%d\n", &m);

						CheckFlag();

						if (NumLoadedFiles)
						{
							fscanf(D2VFile, "Location=%d,%X,%d,%X\n", &process.leftfile, 
								&process.leftlba, &process.rightfile, &process.rightlba);

							process.startfile = process.leftfile;
							process.startloc = process.leftlba * BUFFER_SIZE;
							process.endfile = process.rightfile;
							process.endloc = (process.rightlba - 1) * BUFFER_SIZE;

							process.run = 0;
							for (i=0; i<process.startfile; i++)
								process.run += Infilelength[i];
							process.start = process.run + process.startloc;

							process.end = 0;
							for (i=0; i<process.endfile; i++)
								process.end += Infilelength[i];
							process.end += process.endloc;

							process.trackleft = (process.start*TRACK_PITCH/Infiletotal);
							process.trackright = (process.end*TRACK_PITCH/Infiletotal);

							process.locate = LOCATE_INIT;

							if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
								hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
						}
					}
					break;

				case IDM_PARSE_D2V:
					if (PopFileDlg(szInput, hWnd, OPEN_D2V))
					{
						if (parse_d2v(hWnd, szInput))
						{
							ShellExecute(hDlg, "open", szInput, NULL, NULL, SW_SHOWNORMAL);
						}
					}
					break;

#if 0
				// This is disabled because it is done automatically.
				case IDM_FIX_D2V:
					if (PopFileDlg(szInput, hWnd, OPEN_D2V))
					{
						fix_d2v(hWnd, szInput, 0);
					}
					break;
#endif

				case IDM_LOG_QUANTS:
					if (LogQuants_Flag == 0)
					{
						// Enable quant matrix logging.
						LogQuants_Flag = 1;
						CheckMenuItem(hMenu, IDM_LOG_QUANTS, MF_CHECKED);
					}
					else
					{
						// Disable quant matrix logging.
						LogQuants_Flag = 0;
						CheckMenuItem(hMenu, IDM_LOG_QUANTS, MF_UNCHECKED);
					}
					break;

				case IDM_STOP:
					Stop_Flag = true;
					ExitOnEnd = 0;
					CLIActive = 0;
					FileLoadedEnables();

					if (Pause_Flag)
						ResumeThread(hThread);
					break;

				case IDM_AUDIO_NONE:
					Method_Flag = AUDIO_NONE;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
					break;

				case IDM_DEMUX:
					Method_Flag = AUDIO_DEMUX;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
					break;

				case IDM_DEMUXALL:
					Method_Flag = AUDIO_DEMUXALL;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
					break;

				case IDM_DECODE:
					Method_Flag = AUDIO_DECODE;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
					break;

				case IDM_TRACK_1:
				case IDM_TRACK_2:
				case IDM_TRACK_3:
				case IDM_TRACK_4:
				case IDM_TRACK_5:
				case IDM_TRACK_6:
				case IDM_TRACK_7:
				case IDM_TRACK_8:
					{
					int track;
					track = wmId - IDM_TRACK_1; 
//					ClearTrack();
					if (Track_Flag & (1 << track))
					{
						Track_Flag &= ~(1 << track);
						CheckMenuItem(hMenu, wmId, MF_UNCHECKED);
					}
					else
					{
						Track_Flag |= (1 << track);
						CheckMenuItem(hMenu, wmId, MF_CHECKED);
					}
					break;
					}

				case IDM_DRC_NONE:
					DRC_Flag = DRC_NONE;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					break;

				case IDM_DRC_LIGHT:
					DRC_Flag = DRC_LIGHT;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					break;

				case IDM_DRC_NORMAL:
					DRC_Flag = DRC_NORMAL;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					break;

				case IDM_DRC_HEAVY:
					DRC_Flag = DRC_HEAVY;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
					break;

				case IDM_DSDOWN:
					if (DSDown_Flag)
						CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
					else
						CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);

					DSDown_Flag = !DSDown_Flag;
					break;

				case IDM_PRESCALE:
					if (PreScale_Ratio==1.0 && Check_Flag && Method_Flag!=AUDIO_DEMUXALL && IsWindowEnabled(hTrack))
					{
						Decision_Flag = true;
						Display_Flag = false;
						RunningEnables();
						ShowInfo(true);

						process.locate = LOCATE_RIP;
						PreScale_Ratio = 1.0;

						if (WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					else
					{
						CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);
						PreScale_Ratio = 1.0;
					}
					break;

				case IDM_DETECT_PIDS_RAW:
					Pid_Detect_Method = PID_DETECT_RAW;
					DialogBox(hInst, (LPCTSTR)IDD_DETECT_PIDS, hWnd, (DLGPROC) DetectPids);
					break;

				case IDM_DETECT_PIDS:
					Pid_Detect_Method = PID_DETECT_PATPMT;
					DialogBox(hInst, (LPCTSTR)IDD_DETECT_PIDS, hWnd, (DLGPROC) DetectPids);
					break;

				case IDM_SET_PIDS:
					DialogBox(hInst, (LPCTSTR)IDD_SET_PIDS, hWnd, (DLGPROC) SetPids);
					break;

				case IDM_IDCT_MMX:
					iDCT_Flag = IDCT_MMX;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SSEMMX:
					iDCT_Flag = IDCT_SSEMMX;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SSE2MMX:
					iDCT_Flag = IDCT_SSE2MMX;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_FPU:
					iDCT_Flag = IDCT_FPU;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_REF:
					iDCT_Flag = IDCT_REF;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SKAL:
					iDCT_Flag = IDCT_SKAL;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SIMPLE:
					iDCT_Flag = IDCT_SIMPLE;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_CHECKED);
					break;

				case IDM_FO_NONE:
					FO_Flag = FO_NONE;
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					break;

				case IDM_FO_FILM:
					FO_Flag = FO_FILM;
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					break;

				case IDM_FO_RAW:
					FO_Flag = FO_RAW;
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					break;

				case IDM_TVSCALE:
					Scale_Flag = false;

					setRGBValues();

					CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);

					RefreshWindow(true);
					break;

				case IDM_PCSCALE:
					Scale_Flag = true;

					setRGBValues();

					CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);

					RefreshWindow(true);
					break;

				case IDM_CLIPRESIZE:
					DialogBox(hInst, (LPCTSTR)IDD_CLIPRESIZE, hWnd, (DLGPROC)ClipResize);
					break;

				case IDM_LUMINANCE:
					DialogBox(hInst, (LPCTSTR)IDD_LUMINANCE, hWnd, (DLGPROC)Luminance);
					break;

				case IDM_NORM:
					DialogBox(hInst, (LPCTSTR)IDD_NORM, hWnd, (DLGPROC)Normalization);
					break;

				case IDM_SPEED_SINGLE_STEP:
					PlaybackSpeed = SPEED_SINGLE_STEP;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, true);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_SUPER_SLOW:
					PlaybackSpeed = SPEED_SUPER_SLOW;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_SLOW:
					PlaybackSpeed = SPEED_SLOW;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_NORMAL:
					PlaybackSpeed = SPEED_NORMAL;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_FAST:
					PlaybackSpeed = SPEED_FAST;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_MAXIMUM:
					PlaybackSpeed = SPEED_MAXIMUM;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_CHECKED);
					break;

				case IDM_PP_HIGH:
					Priority_Flag = PRIORITY_HIGH;
					SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
					CheckMenuItem(hMenu, IDM_PP_HIGH, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_LOW, MF_UNCHECKED);
					break;

				case IDM_PP_NORMAL:
					Priority_Flag = PRIORITY_NORMAL;
					SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
					CheckMenuItem(hMenu, IDM_PP_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_PP_LOW, MF_UNCHECKED);
					break;

				case IDM_PP_LOW:
					Priority_Flag = PRIORITY_LOW;
					SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
					CheckMenuItem(hMenu, IDM_PP_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_LOW, MF_CHECKED);
					break;

				case IDM_FORCE_OPEN:
					ForceOpenGops ^= 1;
					if (ForceOpenGops)
						CheckMenuItem(hMenu, IDM_FORCE_OPEN, MF_CHECKED);
					else
						CheckMenuItem(hMenu, IDM_FORCE_OPEN, MF_UNCHECKED);
					break;

				case IDM_PAUSE:
					if (Pause_Flag)
						ResumeThread(hThread);
					else
						SuspendThread(hThread);

					Pause_Flag = !Pause_Flag;
					break;

				case IDM_BMP:
					SaveBMP();
					break;

				case IDM_SRC_NONE:
					SRC_Flag = SRC_NONE;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_LOW:
					SRC_Flag = SRC_LOW;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_MID:
					SRC_Flag = SRC_MID;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_HIGH:
					SRC_Flag = SRC_HIGH;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_UHIGH:
					SRC_Flag = SRC_UHIGH;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
					break;

				case IDM_ABOUT:
					DialogBox(hInst, (LPCTSTR)IDD_ABOUT, hWnd, (DLGPROC)About);
					break;

				case IDM_QUICK_START:
				case IDM_DGINDEX_MANUAL:
				case IDM_DGDECODE_MANUAL:
				{
					strcpy(prog, ExePath);
					if (wmId == IDM_QUICK_START)
						strcat(prog, "QuickStart.html");
					else if (wmId == IDM_DGINDEX_MANUAL)
						strcat(prog, "DGIndexManual.html");
					else
						strcat(prog, "DGDecodeManual.html");
					ShellExecute(NULL, "open", prog, NULL, NULL, SW_SHOWNORMAL);
					break;
				}

				case IDM_JACKEI:
					ShellExecute(NULL, "open", "http://arbor.ee.ntu.edu.tw/~jackeikuo/dvd2avi/", NULL, NULL, SW_SHOWNORMAL);
					break;

				case IDM_NEURON2:
					ShellExecute(NULL, "open", "http://neuron2.net/dgmpgdec/dgmpgdec.html", NULL, NULL, SW_SHOWNORMAL);
					break;

				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;

				case ID_LEFT_BUTTON:
					if (IsWindowEnabled(hTrack))
					{
						SetFocus(hWnd);
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
//						if ((process.file < process.rightfile) || (process.file==process.rightfile && process.lba<process.rightlba))
						{
							process.leftfile = process.file;
							process.leftlba = process.lba;

							process.run = 0;
							for (i=0; i<process.leftfile; i++)
								process.run += Infilelength[i];
							process.trackleft = ((process.run + process.leftlba * BUFFER_SIZE) * TRACK_PITCH / Infiletotal);

							SendMessage(hTrack, TBM_SETPOS, (WPARAM) true, (LONG) process.trackleft);
							SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
						}
					}
					break;

				case ID_LEFT_ARROW:
					SetFocus(hWnd);

					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
					{
						Display_Flag = true;

						process.locate = LOCATE_BACKWARD;
						hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case ID_RIGHT_ARROW:
					SetFocus(hWnd);
					if (process.locate == LOCATE_RIP)
					{
						// We are in play/preview mode. Signal the
						// display process to step forward one frame.
						if (PlaybackSpeed == SPEED_SINGLE_STEP)
							RightArrowHit = true;
						break;
					}

					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
					{
						Display_Flag = true;

						process.locate = LOCATE_FORWARD;
						hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case ID_RIGHT_BUTTON:
					if (IsWindowEnabled(hTrack))
					{
						SetFocus(hWnd);
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
//						if ((process.file>process.leftfile) || (process.file==process.leftfile && process.lba>process.leftlba))
						{
							process.rightfile = process.file;
							process.rightlba = process.lba;

							process.run = 0;
							for (i=0; i<process.rightfile; i++)
								process.run += Infilelength[i];
							process.trackright = ((process.run + (__int64)process.rightlba*BUFFER_SIZE)*TRACK_PITCH/Infiletotal);

							SendMessage(hTrack, TBM_SETPOS, (WPARAM) true, (LONG) process.trackright);
							SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
						}
					}
					break;

				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_HSCROLL:
			SetFocus(hWnd);

			if (Info_Flag)
			{
				DestroyWindow(hDlg);
				Info_Flag = false;
			}
			if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
			{
				int trackpos;

				Display_Flag = true;

				trackpos = SendMessage(hTrack, TBM_GETPOS, 0, 0);
				process.startloc = process.start = Infiletotal*trackpos/TRACK_PITCH;

				process.startfile = 0;
				process.run = 0;
				while (process.startloc > Infilelength[process.startfile])
				{
					process.startloc -= Infilelength[process.startfile];
					process.run += Infilelength[process.startfile];
					process.startfile++;
				}

				process.end = Infiletotal - BUFFER_SIZE;
				process.endfile = NumLoadedFiles - 1;
				process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE-1)*BUFFER_SIZE;

				process.locate = LOCATE_SCROLL;

				hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
			}
			break;

		case WM_KEYDOWN:
			switch (wParam)
			{
				case VK_HOME:
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_LEFT_BUTTON, 0), (LPARAM) 0);
					}
					break;

				case VK_END:
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_RIGHT_BUTTON, 0), (LPARAM) 0);
					}
					break;

				case VK_LEFT:
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
						{
							Display_Flag = true;

							process.locate = LOCATE_BACKWARD;
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
						}
					}
					break;

				case VK_RIGHT:
					if (process.locate == LOCATE_RIP)
					{
						// We are in play/preview mode. Signal the
						// display process to step forward one frame.
						if (PlaybackSpeed == SPEED_SINGLE_STEP)
							RightArrowHit = true;
						break;
					}
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
						{
							Display_Flag = true;

							process.locate = LOCATE_FORWARD;
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
						}
					}
					break;
			}
			break;

		case WM_SIZE:
			if (IsIconic(hWnd))
			{
				if (DDOverlay_Flag && Check_Flag)
					IDirectDrawSurface_UpdateOverlay(lpOverlay, NULL, lpPrimary, NULL, DDOVER_HIDE, NULL);
			}
			else
			{
				ShowInfo(false);
				RefreshWindow(true);
			}
			break;

		case WM_MOVE:
			if (IsIconic(hWnd))
			{
				if (DDOverlay_Flag && Check_Flag)
					IDirectDrawSurface_UpdateOverlay(lpOverlay, NULL, lpPrimary, NULL, DDOVER_HIDE, NULL);
			}
			else
			{
				ShowInfo(false);
				RefreshWindow(false);
			}
			break;

		case WM_PAINT:
			{
			BITMAP bm;
			HDC hdcMem;
			HBITMAP hbmOld;

			hdc = BeginPaint(hWnd, &ps);
			// Paint the splash screen if no files are loaded.
			if (splash && NumLoadedFiles == 0)
			{
				hdcMem = CreateCompatibleDC(hdc);
				hbmOld = (HBITMAP) SelectObject(hdcMem, splash);
				GetObject(splash, sizeof(bm), &bm);
				BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, hbmOld);
				DeleteDC(hdcMem);
			}
			EndPaint(hWnd, &ps);
			ReleaseDC(hWnd, hdc);
			RefreshWindow(false);
			break;
			}

		case WM_DROPFILES:
			char *ext, *tmp;
			int drop_count, drop_index;
			int n;

			if (Info_Flag)
			{
				DestroyWindow(hDlg);
				Info_Flag = false;
			}

			DragQueryFile((HDROP)wParam, 0, szInput, sizeof(szInput));
			SetForegroundWindow(hWnd);

			// Set the output directory for a Save D2V operation to the
			// same path as these input files.
			strcpy(path, szInput);
			tmp = path + strlen(path);
			while (*tmp != '\\' && tmp >= path) tmp--;
			tmp[1] = 0;
			strcpy(szSave, path);

			ext = strrchr(szInput, '.');
			if (ext!=NULL)
			{
				if (!_strnicmp(ext, ".d2v", 4))
				{
					DragFinish((HDROP)wParam);
					goto D2V_PROCESS;
				}
			}

			while (NumLoadedFiles)
			{
				NumLoadedFiles--;
				_close(Infile[NumLoadedFiles]);
			}

			drop_count = DragQueryFile((HDROP)wParam, 0xffffffff, szInput, sizeof(szInput));
			for (drop_index = 0; drop_index < drop_count; drop_index++)
			{
				DragQueryFile((HDROP)wParam, drop_index, szInput, sizeof(szInput));
				struct _finddata_t seqfile;
				if (_findfirst(szInput, &seqfile) != -1L)
				{
					strcpy(Infilename[NumLoadedFiles], szInput);
					NumLoadedFiles++;
					SystemStream_Flag = ELEMENTARY_STREAM;
				}
			}
			DragFinish((HDROP)wParam);
			// Sort the filenames.
			// This is a special sort designed to do things intelligently
			// for typical sequentially numbered filenames.
			// Sorry, just a bubble sort. No need for performance here. KISS.
			n = NumLoadedFiles;
			for (i = 0; i < n - 1; i++)
			{
				for (j = 0; j < n - 1 - i; j++)
				{
					if (strverscmp(Infilename[j+1], Infilename[j]) < 0)
					{
						tmp = Infilename[j];
						Infilename[j] = Infilename[j+1];
						Infilename[j+1] = tmp;
					}
				}
			}
			// Open the files.
			for (i = 0; i < NumLoadedFiles; i++)
			{
				Infile[i] = _open(Infilename[i], _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
			}
			DialogBox(hInst, (LPCTSTR)IDD_FILELIST, hWnd, (DLGPROC)VideoList);
			break;

		case WM_DESTROY:
			GetWindowRect(hWnd, &wrect);
			strcpy(prog, ExePath);
			strcat(prog, "DGIndex.ini");
			if ((INIFile = fopen(prog, "w")) != NULL)
			{
				fprintf(INIFile, "DVD2AVI_Version=%s\n", Version);
				fprintf(INIFile, "Window_Position=%d,%d\n", wrect.left, wrect.top);
				fprintf(INIFile, "iDCT_Algorithm=%d\n", iDCT_Flag);
				fprintf(INIFile, "YUVRGB_Scale=%d\n", Scale_Flag);
				fprintf(INIFile, "Field_Operation=%d\n", FO_Flag);
				fprintf(INIFile, "Output_Method=%d\n", Method_Flag);
				fprintf(INIFile, "Track_Number=%d\n", Track_Flag);
				fprintf(INIFile, "DR_Control=%d\n", DRC_Flag);
				fprintf(INIFile, "DS_Downmix=%d\n", DSDown_Flag);
				fprintf(INIFile, "SRC_Precision=%d\n", SRC_Flag);
				fprintf(INIFile, "Norm_Ratio=%d\n", 100 * Norm_Flag + Norm_Ratio);
				fprintf(INIFile, "Process_Priority=%d\n", Priority_Flag);
				fprintf(INIFile, "Transport_PIDs=%x,%x\n", MPEG2_Transport_VideoPID, MPEG2_Transport_AudioPID);
				fprintf(INIFile, "Playback_Speed=%d\n", PlaybackSpeed);
				fprintf(INIFile, "Force_Open_Gops=%d\n", ForceOpenGops);

				fclose(INIFile);
			}

			while (NumLoadedFiles)
			{
				NumLoadedFiles--;
				_close(Infile[NumLoadedFiles]);
			}

			Recovery();

			for (i=0; i<8; i++)
				_aligned_free(block[i]);

			for (i=0; i<MAX_FILE_NUMBER; i++)
				free(Infilename[i]);

	        DeleteObject(splash);
			ReleaseDC(hWnd, hDC);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return false;
}

LRESULT CALLBACK DetectPids(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	char msg[255];

	switch (message)
	{
		case WM_INITDIALOG:
			if (SystemStream_Flag != TRANSPORT_STREAM)
			{
				sprintf(msg, "Not a transport stream!");
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)msg);
			}
			else if (Pid_Detect_Method == PID_DETECT_RAW)
			{
				pat_parser.DumpRaw(hDialog, Infilename[0]);
			}
			else if (pat_parser.DumpPAT(hDialog, Infilename[0]) == 1)
			{
				sprintf(msg, "Could not find PAT/PMT tables!");
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)msg);
			}
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				int item;
				char text[80], *ptr;

				case IDC_SET_AUDIO:
				case IDC_SET_VIDEO:
					item = SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, (UINT) LB_GETCURSEL, 0, 0);
					if (item != LB_ERR)
					{
						SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, (UINT) LB_GETTEXT, item, (LPARAM) text);
						if ((ptr = strstr(text, "0x")) != NULL)
						{
							if (LOWORD(wParam) == IDC_SET_AUDIO)
								sscanf(ptr, "%x", &MPEG2_Transport_AudioPID);
							else
								sscanf(ptr, "%x", &MPEG2_Transport_VideoPID);
						}
					}
					if (LOWORD(wParam) == IDC_SET_AUDIO) break;
					Recovery();
					if (NumLoadedFiles)
					{
						FileLoadedEnables();
						process.rightfile = NumLoadedFiles-1;
						process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/BUFFER_SIZE);

						process.end = Infiletotal - BUFFER_SIZE;
						process.endfile = NumLoadedFiles - 1;
						process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE - 1)*BUFFER_SIZE;

						process.locate = LOCATE_INIT;

						if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case IDC_SET_DONE:
				case IDCANCEL:
					EndDialog(hDialog, 0);
			}
			return true;
	}
    return false;
}

LRESULT CALLBACK VideoList(HWND hVideoListDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i, j;
	char updown[_MAX_PATH];
	char *name;
	int handle;

	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETHORIZONTALEXTENT, (WPARAM) 1024, 0);  
			if (NumLoadedFiles)
				for (i=0; i<NumLoadedFiles; i++)
					SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)Infilename[i]);
			else
				OpenVideoFile(hVideoListDlg);

			if (NumLoadedFiles)
				SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, NumLoadedFiles-1, 0);

			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_ADD:
					OpenVideoFile(hVideoListDlg);

					if (NumLoadedFiles)
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, NumLoadedFiles-1, 0);
					break;

				case ID_UP:
					i = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
					if (i != 0)
					{
						name = Infilename[i];
						Infilename[i] = Infilename[i-1];
						Infilename[i-1] = name;
						handle = Infile[i];
						Infile[i] = Infile[i-1];
						Infile[i-1] = handle;
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETTEXT, i - 1, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i - 1, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_INSERTSTRING, i, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i - 1, 0);
					}
					break;

				case ID_DOWN:
					i = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
					j = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCOUNT, 0, 0);
					if (i < j - 1)
					{
						name = Infilename[i];
						Infilename[i] = Infilename[i+1];
						Infilename[i+1] = name;
						handle = Infile[i];
						Infile[i] = Infile[i+1];
						Infile[i+1] = handle;
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_INSERTSTRING, i + 1, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i + 1, 0);
					}
					break;

				case ID_DEL:
					if (NumLoadedFiles)
					{
						i= SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
						NumLoadedFiles--;
						_close(Infile[i]);
						for (j=i; j<NumLoadedFiles; j++)
						{
							Infile[j] = Infile[j+1];
							strcpy(Infilename[j], Infilename[j+1]);
						}
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i>=NumLoadedFiles ? NumLoadedFiles-1 : i, 0);
					}
					if (!NumLoadedFiles)
					{
						Recovery();
						SystemStream_Flag = ELEMENTARY_STREAM;
					}
					break;

				case ID_DELALL:
					while (NumLoadedFiles)
					{
						NumLoadedFiles--;
						i= SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
						_close(Infile[i]);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i>=NumLoadedFiles ? NumLoadedFiles-1 : i, 0);
					}
					Recovery();
					SystemStream_Flag = ELEMENTARY_STREAM;
					break;

				case IDOK:
				case IDCANCEL:
					EndDialog(hVideoListDlg, 0);
					Recovery();

					if (NumLoadedFiles)
					{
						FileLoadedEnables();
						process.rightfile = NumLoadedFiles-1;
						process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/BUFFER_SIZE);

						process.end = Infiletotal - BUFFER_SIZE;
						process.endfile = NumLoadedFiles - 1;
						process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE - 1)*BUFFER_SIZE;

						process.locate = LOCATE_INIT;

						if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					else
					{
						StartupEnables();
					}
					return true;
			}
			break;
	}
    return false;
}

static void OpenVideoFile(HWND hVideoListDlg)
{
	if (PopFileDlg(szInput, hVideoListDlg, OPEN_VOB))
	{
		char *p;
		char path[_MAX_PATH];
		char filename[_MAX_PATH];
		char curPath[_MAX_PATH];
		struct _finddata_t seqfile;
		int i, j, n;
		char *tmp;

		SystemStream_Flag = ELEMENTARY_STREAM;
		getcwd(curPath, _MAX_PATH);
		if (strlen(curPath) != strlen(szInput))
		{
			// Only one file specified.
			if (_findfirst(szInput, &seqfile) == -1L) return;
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) szInput);
			strcpy(Infilename[NumLoadedFiles], szInput);
			Infile[NumLoadedFiles] = _open(szInput, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
			NumLoadedFiles++;
			// Set the output directory for a Save D2V operation to the
			// same path as this input files.
			strcpy(path, szInput);
			p = path + strlen(path);
			while (*p != '\\' && p >= path) p--;
			p[1] = 0;
			strcpy(szSave, path);
			return;
		}
		// Multi-select handling.
		// First clear existing file list box.
		n = NumLoadedFiles;
		while (n)
		{
			n--;
			i = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i >= n ? n - 1 : i, 0);
		}
		// Save the path prefix (path without the filename).
		strcpy(path, szInput);
		// Also set that path as the default for a Save D2V operation.
		strcpy(szSave, szInput);
		// Add a trailing backslash if needed.
		p = szInput;
		while (*p != 0) p++;
		p--;
		if (*p != '\\')
			strcat(path, "\\");
		// Skip the path prefix.
		p = szInput;
		while (*p++ != 0);
		// Load the filenames.
		for (;;)
		{
			// Build full path plus filename.
			strcpy(filename, path);
			strcat(filename, p);
			if (_findfirst(filename, &seqfile) == -1L) break;
			strcpy(Infilename[NumLoadedFiles], filename);
			NumLoadedFiles++;
			// Skip to next filename.
			while (*p++ != 0);
			// A double zero is the end of the file list.
			if (*p == 0) break;
		}
		// Sort the filenames.
		// This is a special sort designed to do things intelligently
		// for typical sequentially numbered filenames.
		// Sorry, just a bubble sort. No need for performance here. KISS.
		n = NumLoadedFiles;
		for (i = 0; i < n - 1; i++)
		{
			for (j = 0; j < n - 1 - i; j++)
			{
				if (strverscmp(Infilename[j+1], Infilename[j]) < 0)
				{
					tmp = Infilename[j];
					Infilename[j] = Infilename[j+1];
					Infilename[j+1] = tmp;
				}
			}
		}
		// Load up the file open dialog list box and open the files.
		for (i = 0; i < NumLoadedFiles; i++)
		{
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) Infilename[i]);
			Infile[i] = _open(Infilename[i], _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
		}
	}
}

void ThreadKill()
{
	int i;
//	char d2vpath[2048];

	// CLose the quants log if necessary.
	if (Quants)
		fclose(Quants);

	for (i = 0; i < 8; i++)
	{
		if (D2V_Flag && (ac3[i].rip && Method_Flag==AUDIO_DECODE  ||  pcm[i].rip))
		{
			if (SRC_Flag)
			{
				EndSRC(pcm[i].file);
				pcm[i].size = ((int)(0.91875*pcm[i].size)>>2)<<2;
			}

			Normalize(NULL, 44, pcm[i].filename, pcm[i].file, 44, pcm[i].size);
			CloseWAV(pcm[i].file, pcm[i].size);
		}
	}

	if (process.locate==LOCATE_INIT || process.locate==LOCATE_RIP)
	{
		if (D2V_Flag)
		{
			// Revised by Donald Graft to support IBBPIBBP...
			WriteD2VLine(1);
			fprintf(D2VFile, "\nFINISHED");
			// Prevent divide by 0.
			if (FILM_Purity+VIDEO_Purity == 0) VIDEO_Purity = 1;
			fprintf(D2VFile, "  %.2f%% FILM\n", (FILM_Purity*100.0)/(FILM_Purity+VIDEO_Purity));
//			sprintf(d2vpath, "%s.d2v", szOutput);
//			fclose(D2VFile);
//			fix_d2v(hWnd, d2vpath, 1);
		}

		_fcloseall();

		if (Decision_Flag)
		{
			if (Sound_Max > 1)
			{
				PreScale_Ratio = 327.68 * Norm_Ratio / Sound_Max;

				if (PreScale_Ratio > 1.0 && PreScale_Ratio < 1.01)
					PreScale_Ratio = 1.0;

				sprintf(szBuffer, "%.2f", PreScale_Ratio);
				SetDlgItemText(hDlg, IDC_INFO, szBuffer);

				CheckMenuItem(hMenu, IDM_PRESCALE, MF_CHECKED);
				CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
				Norm_Flag = false;
			}
			else
			{
				SetDlgItemText(hDlg, IDC_INFO, "n/a");
				CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);
			}
		}

		FileLoadedEnables();
		SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
	}

	if (process.locate==LOCATE_RIP)
	{
		SetForegroundWindow(hWnd);

		MessageBeep(MB_OK);	
		SetDlgItemText(hDlg, IDC_REMAIN, "FINISH");
		if (D2V_Flag)
			SendMessage(hWnd, D2V_DONE_MESSAGE, 0, 0);
		if (ExitOnEnd) exit(0);
		else CLIActive= 0;
	}

	if (process.locate==LOCATE_INIT || process.locate==LOCATE_RIP)
	{
		D2V_Flag = false;
		Decision_Flag = false;
		Display_Flag = false;
	}
	// This restores the normal operation of the right arrow button
	// if we were in single-step playback.
   process.locate = LOCATE_INIT;

	if (CLIActive)
	{
		SendMessage(hWnd, CLI_RIP_MESSAGE, 0, 0);
	}
	ExitThread(0);
}

LRESULT CALLBACK Info(HWND hInfoDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			return true;

		case WM_COMMAND:
			if (LOWORD(wParam)==IDCANCEL)
			{
				DestroyWindow(hInfoDlg);
				Info_Flag = false;
				return true;
			}
	}
    return false;
}

LRESULT CALLBACK About(HWND hAboutDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(szBuffer, "%s", Version);
			SetDlgItemText(hAboutDlg, IDC_VERSION, szBuffer);
			return true;

		case WM_COMMAND:
			if (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL) 
			{
				EndDialog(hAboutDlg, 0);
				return true;
			}
	}
    return false;
}

LRESULT CALLBACK ClipResize(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDialog, IDC_LEFT_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 256));
			SendDlgItemMessage(hDialog, IDC_LEFT_SLIDER, TBM_SETPOS, 1, Clip_Left/2);
			sprintf(szTemp, "%d", Clip_Left);
			SetDlgItemText(hDialog, IDC_LEFT, szTemp);

			SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 256));
			SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_SETPOS, 1, Clip_Right/2);
			sprintf(szTemp, "%d", Clip_Right);
			SetDlgItemText(hDialog, IDC_RIGHT, szTemp);

			SendDlgItemMessage(hDialog, IDC_TOP_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 128));
			SendDlgItemMessage(hDialog, IDC_TOP_SLIDER, TBM_SETPOS, 1, Clip_Top/2);
			sprintf(szTemp, "%d", Clip_Top);
			SetDlgItemText(hDialog, IDC_TOP, szTemp);

			SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 128));
			SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_SETPOS, 1, Clip_Bottom/2);
			sprintf(szTemp, "%d", Clip_Bottom);
			SetDlgItemText(hDialog, IDC_BOTTOM, szTemp);

			ShowWindow(hDialog, SW_SHOW);

			if (ClipResize_Flag)
				SendDlgItemMessage(hDialog, IDC_CLIPRESIZE_CHECK, BM_SETCHECK, BST_CHECKED, 0);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_CLIPRESIZE_CHECK:
					if (SendDlgItemMessage(hDialog, IDC_CLIPRESIZE_CHECK, BM_GETCHECK, 1, 0)==BST_CHECKED)
					{
						CheckMenuItem(hMenu, IDM_CLIPRESIZE, MF_CHECKED);
						ClipResize_Flag = true;
					}
					else
					{
						CheckMenuItem(hMenu, IDM_CLIPRESIZE, MF_UNCHECKED);
						ClipResize_Flag = false;
					}

					RefreshWindow(true);
					ShowInfo(false);
					break;

				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;

		case WM_HSCROLL:
			switch (GetWindowLong((HWND)lParam, GWL_ID))
			{
				case IDC_LEFT_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_LEFT_SLIDER, TBM_GETPOS, 0, 0)*2;
					if (i+Clip_Right+MIN_WIDTH <= Coded_Picture_Width)
					{
						Clip_Left = i;
						sprintf(szTemp, "%d", Clip_Left);
						SetDlgItemText(hDialog, IDC_LEFT, szTemp);

						Clip_Right = 8 - Clip_Left%8;
						sprintf(szTemp, "%d", Clip_Right);
						SetDlgItemText(hDialog, IDC_RIGHT, szTemp);
						SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_SETPOS, 1, Clip_Right/2);
					}
					break;

				case IDC_RIGHT_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_GETPOS, 0, 0)*2;
					if (i+Clip_Left+MIN_WIDTH<=Coded_Picture_Width && (i+Clip_Left)%8==0)
					{
						Clip_Right = i;
						sprintf(szTemp, "%d", Clip_Right);
						SetDlgItemText(hDialog, IDC_RIGHT, szTemp);
					}
					break;

				case IDC_TOP_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_TOP_SLIDER, TBM_GETPOS, 0, 0)*2;
					if (i+Clip_Bottom+MIN_HEIGHT <= Coded_Picture_Height)
					{
						Clip_Top = i;
						sprintf(szTemp, "%d", Clip_Top);
						SetDlgItemText(hDialog, IDC_TOP, szTemp);

						Clip_Bottom = 8 - Clip_Top%8;
						sprintf(szTemp, "%d", Clip_Bottom);
						SetDlgItemText(hDialog, IDC_BOTTOM, szTemp);
						SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_SETPOS, 1, Clip_Bottom/2);
					}
					break;

				case IDC_BOTTOM_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_GETPOS, 0, 0)*2;
					if (i+Clip_Top+MIN_HEIGHT<=Coded_Picture_Height && (i+Clip_Top)%8==0)
					{
						Clip_Bottom = i;
						sprintf(szTemp, "%d", Clip_Bottom);
						SetDlgItemText(hDialog, IDC_BOTTOM, szTemp);
					}
					break;
			}

			RefreshWindow(true);
			ShowInfo(false);
			break;
	}
    return false;
}

LRESULT CALLBACK Luminance(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDialog, IDC_GAMMA_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 512));
			SendDlgItemMessage(hDialog, IDC_GAMMA_SLIDER, TBM_SETTICFREQ, 256, 0);
			SendDlgItemMessage(hDialog, IDC_GAMMA_SLIDER, TBM_SETPOS, 1, LumGamma+256);
			sprintf(szTemp, "%d", LumGamma);
			SetDlgItemText(hDialog, IDC_GAIN, szTemp);

			SendDlgItemMessage(hDialog, IDC_OFFSET_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 512));
			SendDlgItemMessage(hDialog, IDC_OFFSET_SLIDER, TBM_SETTICFREQ, 256, 0);
			SendDlgItemMessage(hDialog, IDC_OFFSET_SLIDER, TBM_SETPOS, 1, LumOffset+256);
			sprintf(szTemp, "%d", LumOffset);
			SetDlgItemText(hDialog, IDC_OFFSET, szTemp);

			ShowWindow(hDialog, SW_SHOW);

			if (Luminance_Flag)
				SendDlgItemMessage(hDialog, IDC_LUM_CHECK, BM_SETCHECK, BST_CHECKED, 0);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_LUM_CHECK:
					if (SendDlgItemMessage(hDialog, IDC_LUM_CHECK, BM_GETCHECK, 1, 0)==BST_CHECKED)
					{
						CheckMenuItem(hMenu, IDM_LUMINANCE, MF_CHECKED);
						Luminance_Flag = true;
					}
					else
					{
						CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);
						Luminance_Flag = false;
					}
					
					RefreshWindow(true);
					break;

				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;

		case WM_HSCROLL:
			switch (GetWindowLong((HWND)lParam, GWL_ID))
			{
				case IDC_GAMMA_SLIDER:
					LumGamma = SendDlgItemMessage(hDialog, IDC_GAMMA_SLIDER, TBM_GETPOS, 0, 0) - 256;
					sprintf(szTemp, "%d", LumGamma);
					SetDlgItemText(hDialog, IDC_GAIN, szTemp);	
					break;

				case IDC_OFFSET_SLIDER:
					LumOffset = SendDlgItemMessage(hDialog, IDC_OFFSET_SLIDER, TBM_GETPOS, 0, 0) - 256;
					sprintf(szTemp, "%d", LumOffset);
					SetDlgItemText(hDialog, IDC_OFFSET, szTemp);
					break;
			}

			RefreshWindow(true);
			break;
	}
    return false;
}

LRESULT CALLBACK SetPids(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[80];

	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(szTemp, "%x", MPEG2_Transport_VideoPID);
			SetDlgItemText(hDialog, IDC_VIDEO_PID, szTemp);
			sprintf(szTemp, "%x", MPEG2_Transport_AudioPID);
			SetDlgItemText(hDialog, IDC_AUDIO_PID, szTemp);
			ShowWindow(hDialog, SW_SHOW);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_PIDS_OK:
					GetDlgItemText(hDialog, IDC_VIDEO_PID, buf, 10);
					sscanf(buf, "%x", &MPEG2_Transport_VideoPID);
					GetDlgItemText(hDialog, IDC_AUDIO_PID, buf, 10);
					sscanf(buf, "%x", &MPEG2_Transport_AudioPID);
					EndDialog(hDialog, 0);
					Recovery();
					if (NumLoadedFiles)
					{
						FileLoadedEnables();
						process.rightfile = NumLoadedFiles-1;
						process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/BUFFER_SIZE);

						process.end = Infiletotal - BUFFER_SIZE;
						process.endfile = NumLoadedFiles - 1;
						process.endloc = (Infilelength[NumLoadedFiles-1]/BUFFER_SIZE - 1)*BUFFER_SIZE;

						process.locate = LOCATE_INIT;

						if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					return true;

				case IDCANCEL:
				case IDC_PIDS_CANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;
	}
    return false;
}

LRESULT CALLBACK Normalization(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_SETTICFREQ, 50, 0);
			SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_SETPOS, 1, Norm_Ratio);
			sprintf(szTemp, "%d", Norm_Ratio);
			SetDlgItemText(hDialog, IDC_NORM, szTemp);

			ShowWindow(hDialog, SW_SHOW);

			if (Norm_Flag)
				SendDlgItemMessage(hDialog, IDC_NORM_CHECK, BM_SETCHECK, BST_CHECKED, 0);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_NORM_CHECK:
					if (SendDlgItemMessage(hDialog, IDC_NORM_CHECK, BM_GETCHECK, 1, 0)==BST_CHECKED)
					{
						CheckMenuItem(hMenu, IDM_NORM, MF_CHECKED);
						Norm_Flag = true;
					}
					else
					{
						CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
						Norm_Flag = false;
					}
					break;

				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;

		case WM_HSCROLL:
			if (GetWindowLong((HWND)lParam, GWL_ID)==IDC_NORM_SLIDER)
			{
				Norm_Ratio = SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_GETPOS, 0, 0);
				sprintf(szTemp, "%d", Norm_Ratio);
				SetDlgItemText(hDialog, IDC_NORM, szTemp);
			}
			break;
	}
    return false;
}

/* register the window class */
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= false;
	wcex.cbWndExtra		= false;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MOVIE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= CreateSolidBrush(MASKCOLOR);

	wcex.lpszMenuName = (LPCSTR)IDC_GUI;
	wcex.lpszMenuName = (LPCSTR)IDC_GUI;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

bool PopFileDlg(PTSTR pstrFileName, HWND hOwner, int Status)
{
	static OPENFILENAME ofn;
	static char *szFilter, *ext;

	switch (Status)
	{
		case OPEN_VOB:
			ofn.nFilterIndex = 4;
			szFilter = \
				TEXT ("vob\0*.vob\0") \
				TEXT ("mpg, mpeg, m1v, m2v, mpv\0*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv\0") \
				TEXT ("tp, ts, trp, pva, vro\0*.tp;*.ts;*.trp;*.pva;*.vro\0") \
				TEXT ("All MPEG Files\0*.vob;*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv;*.tp;*.ts;*.trp;*.pva;*.vro\0") \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_D2V:
			szFilter = TEXT ("DGIndex Project File (*.d2v)\0*.d2v\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case SAVE_D2V:
			szFilter = TEXT ("DGIndex Project File (*.d2v)\0*.d2v\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case SAVE_BMP:
			szFilter = TEXT ("BMP File (*.bmp)\0*.bmp\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_WAV:
		case SAVE_WAV:
			szFilter = TEXT ("WAV File (*.wav)\0*.wav\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;
	}

	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = hOwner ;
	ofn.hInstance         = hInst ;
	ofn.lpstrFilter       = szFilter ;
	ofn.nMaxFile          = 10 * _MAX_PATH ;
	ofn.nMaxFileTitle     = _MAX_PATH ;
	ofn.lpstrFile         = pstrFileName ;
	ofn.lpstrInitialDir   = szSave;

	switch (Status)
	{
		case OPEN_VOB:
		case OPEN_D2V:
		case OPEN_WAV:
			gop_warned = false;
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			return GetOpenFileName(&ofn);

		case SAVE_BMP:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".bmp", 4))
					*ext = 0;
				return true;
			}
			break;

		case SAVE_WAV:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".wav", 4))
					*ext = 0;
				return true;
			}
			break;

		case SAVE_D2V:
			{
			char *p;

			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			// Load a default filename based on the name of the first input file.
			if (szOutput[0] == NULL)
			{
				strcpy(ofn.lpstrFile, Infilename[0]);
				p = &ofn.lpstrFile[strlen(ofn.lpstrFile)];
				while (*p != '.' && p >= ofn.lpstrFile) p--;
				if (p != ofn.lpstrFile)
				{
					*p = 0;
				}
			}
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".d2v", 4))
					*ext = 0;
				return true;
			}
			break;
			}
	}
	return false;
}

static void ShowInfo(bool update)
{
	if (update)
	{
		if (Info_Flag)
			DestroyWindow(hDlg);

		Info_Flag = true;
		hDlg = CreateDialog(hInst, (LPCTSTR)IDD_INFO, hWnd, (DLGPROC)Info);
	}

	if (Info_Flag)
	{
		GetWindowRect(hDlg, &crect);
		GetWindowRect(hWnd, &wrect);
		if (Coded_Picture_Width < 960 && Coded_Picture_Height < 720)
			MoveWindow(hDlg, wrect.right, wrect.top+Edge_Height-Edge_Width/2,
					   crect.right-crect.left, crect.bottom-crect.top, true);
		ShowWindow(hDlg, WindowMode);
	}
}

static void ClearTrack()
{
	CheckMenuItem(hMenu, IDM_TRACK_1, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_2, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_3, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_4, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_5, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_6, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_7, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TRACK_8, MF_UNCHECKED);
}

static void CheckFlag()
{
	int i;

	CheckMenuItem(hMenu, IDM_KEY_OFF, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_KEY_INPUT, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_KEY_OP, MF_UNCHECKED);

	CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);

	//Downgrade the iDCT if the processor does not support it.
	if (iDCT_Flag == IDCT_SSE2MMX && !cpu.sse2)
		iDCT_Flag = IDCT_SKAL;
	if (iDCT_Flag == IDCT_SKAL && !cpu.ssemmx)
		iDCT_Flag = IDCT_SSEMMX;
	if (iDCT_Flag == IDCT_SSEMMX && !cpu.ssemmx)
		iDCT_Flag = IDCT_MMX;

	switch (iDCT_Flag)
	{
		case IDCT_MMX:
			CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_CHECKED);
			break;

		case IDCT_SSEMMX:
			CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_CHECKED);
			break;

		case IDCT_SSE2MMX:
			CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_CHECKED);
			break;

		case IDCT_FPU:
			CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_CHECKED);
			break;

		case IDCT_REF:
			CheckMenuItem(hMenu, IDM_IDCT_REF, MF_CHECKED);
			break;

		case IDCT_SKAL:
			CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_CHECKED);
			break;

		case IDCT_SIMPLE:
			CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_CHECKED);
			break;
	}

	setRGBValues();
	if (Scale_Flag) CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
	else CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);

	switch (FO_Flag)
	{
		case FO_NONE:
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			break;

		case FO_FILM:
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			break;

		case FO_RAW:
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
			break;
	}

	for (i = 0; i < 8; i++)
	{
		if (Track_Flag & (1 << i))
			CheckMenuItem(hMenu, IDM_TRACK_1 + i, MF_CHECKED);
	}

	switch (Method_Flag)
	{
		case AUDIO_NONE:
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
			break;

		case AUDIO_DEMUX:
			CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			break;

		case AUDIO_DEMUXALL:
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
			break;

		case AUDIO_DECODE:
			CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
			break;
	}

	switch (DRC_Flag)
	{
		case DRC_NONE:
			CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
			break;

		case DRC_LIGHT:
			CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
			break;

		case DRC_NORMAL:
			CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
			break;

		case DRC_HEAVY:
			CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
			break;
	}

	if (DSDown_Flag)
		CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);

	switch (SRC_Flag)
	{
		case SRC_NONE:
			CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
			break;

		case SRC_LOW:
			CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
			break;

		case SRC_MID:
			CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
			break;

		case SRC_HIGH:
			CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
			break;

		case SRC_UHIGH:
			CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
			break;
	}

	if (Norm_Ratio > 100)
	{
		CheckMenuItem(hMenu, IDM_NORM, MF_CHECKED);
		Norm_Flag = true;
		Norm_Ratio -= 100;
	}

	switch (Priority_Flag)
	{
		case PRIORITY_HIGH:
			CheckMenuItem(hMenu, IDM_PP_HIGH, MF_CHECKED);
			SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
			break;

		case PRIORITY_NORMAL:
			CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_CHECKED);
			SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
			break;

		case PRIORITY_LOW:
			CheckMenuItem(hMenu, IDM_PP_LOW, MF_CHECKED);
			SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
			break;
	}

	switch (PlaybackSpeed)
	{
		case SPEED_SINGLE_STEP:
			CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_CHECKED);
			break;

		case SPEED_SUPER_SLOW:
			CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_CHECKED);
			break;

		case SPEED_SLOW:
			CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_CHECKED);
			break;

		case SPEED_NORMAL:
			CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_CHECKED);
			break;

		case SPEED_FAST:
			CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_CHECKED);
			break;

		case SPEED_MAXIMUM:
			CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_CHECKED);
			break;
	}

	if (ForceOpenGops)
			CheckMenuItem(hMenu, IDM_FORCE_OPEN, MF_CHECKED);
}

static void Recovery()
{
	int i;

	if (Check_Flag)
	{
		if (DDOverlay_Flag)
		{
			DDOverlay_Flag = false;
			CheckMenuItem(hMenu, IDM_DIRECTDRAW, MF_UNCHECKED);
			IDirectDrawSurface_UpdateOverlay(lpOverlay, NULL, lpPrimary, NULL, DDOVER_HIDE, NULL);
		}

		if (lpDD2)
			lpDD2->Release();
		if (lpDD)
			lpDD->Release();

		for (i=0; i<3; i++)
		{
			_aligned_free(backward_reference_frame[i]);
			_aligned_free(forward_reference_frame[i]);
			_aligned_free(auxframe[i]);
		}

		_aligned_free(u422);
		_aligned_free(v422);
		_aligned_free(u444);
		_aligned_free(v444);
		_aligned_free(rgb24);
		_aligned_free(rgb24small);
		_aligned_free(yuy2);
		_aligned_free(lum);
	}

	Check_Flag = false;
	MuxFile = (FILE *) 0xffffffff;

	SendMessage(hTrack, TBM_SETPOS, (WPARAM) true, 0);
	SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(0, 0));

	LumGamma = LumOffset = 0;
	Luminance_Flag = false;
	CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);

	Clip_Left = Clip_Right = Clip_Top = Clip_Bottom = 0;
	ClipResize_Flag = false;
	CheckMenuItem(hMenu, IDM_CLIPRESIZE, MF_UNCHECKED);

	PreScale_Ratio = 1.0;
	CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);

	SetWindowText(hWnd, "DGIndex");

	if (NumLoadedFiles)
	{
		ZeroMemory(&process, sizeof(PROCESS));
		process.trackright = TRACK_PITCH;

		Display_Flag = true;

		for (i=0, Infiletotal = 0; i<NumLoadedFiles; i++)
		{
			Infilelength[i] = _filelengthi64(Infile[i]);
			Infiletotal += Infilelength[i];
		}
	}

	ResizeWindow(INIT_WIDTH, INIT_HEIGHT);
	ResizeWindow(INIT_WIDTH, INIT_HEIGHT);	// 2-line menu antidote

	if (!CLIActive)
		szOutput[0] = 0;
	VOB_ID = CELL_ID = 0;
}

void CheckDirectDraw()
{
	if (DirectDrawCreate(NULL, &lpDD, NULL)==DD_OK)
	{
		if (lpDD->QueryInterface(IID_IDirectDraw2, (LPVOID*)&lpDD2)==DD_OK)
		{
			if (lpDD2->SetCooperativeLevel(hWnd, DDSCL_NORMAL)==DD_OK)
			{
				ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
				ddsd.dwSize = sizeof(DDSURFACEDESC);
				ddsd.dwFlags = DDSD_CAPS;
				ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;

				if (lpDD2->CreateSurface(&ddsd, &lpPrimary, NULL)==DD_OK)
				{
					ZeroMemory(&halcaps, sizeof(DDCAPS));
					halcaps.dwSize = sizeof(DDCAPS);

					if (lpDD2->GetCaps(&halcaps, NULL)==DD_OK)
					{
						if (halcaps.dwCaps & DDCAPS_OVERLAY)
						{
							DDPIXELFORMAT ddPixelFormat;
							ddPixelFormat.dwFlags = DDPF_FOURCC;
							ddPixelFormat.dwFourCC = mmioFOURCC('Y','U','Y','2');
							ddPixelFormat.dwYUVBitCount = 16;

							ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
							ddsd.dwSize = sizeof(DDSURFACEDESC);
							ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
							ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
							ddsd.dwWidth = Coded_Picture_Width;
							ddsd.dwHeight = Coded_Picture_Height;

							memcpy(&(ddsd.ddpfPixelFormat), &ddPixelFormat, sizeof(DDPIXELFORMAT));

							if (lpDD2->CreateSurface(&ddsd, &lpOverlay, NULL)==DD_OK)
							{
								DDOverlay_Flag = true;

								ZeroMemory(&ddofx, sizeof(DDOVERLAYFX));
								ddofx.dwSize = sizeof(DDOVERLAYFX);

								ddofx.dckDestColorkey.dwColorSpaceLowValue = DDColorMatch(lpPrimary, MASKCOLOR);
								ddofx.dckDestColorkey.dwColorSpaceHighValue = ddofx.dckDestColorkey.dwColorSpaceLowValue;

								if (IDirectDrawSurface_Lock(lpOverlay, NULL, &ddsd, 0, NULL)==DD_OK)
								{
									int i, j;
									unsigned char *dst = (unsigned char *)ddsd.lpSurface;

									for (i=0; i<Coded_Picture_Height; i++)
									{
										for (j=0; j<Coded_Picture_Width; j++)
										{
											dst[j*2] = 0;
											dst[j*2+1] = 128;
										}

										dst += ddsd.lPitch;
									}

									IDirectDrawSurface_Unlock(lpOverlay, NULL);
								}
							}
						}
					}
				}
			}
		}
	}

	if (DDOverlay_Flag)
	{
		CheckMenuItem(hMenu, IDM_DIRECTDRAW, MF_CHECKED);
	}
}

static DWORD DDColorMatch(LPDIRECTDRAWSURFACE pdds, COLORREF rgb)
{
	COLORREF	rgbT = 0;
	DWORD		dw = CLR_INVALID;
	HRESULT		hres;
	HDC			hdc;

	if (IDirectDrawSurface_GetDC(pdds, &hdc)==DD_OK)
	{
		rgbT = GetPixel(hdc, 0, 0);
		SetPixel(hdc, 0, 0, rgb);
		IDirectDrawSurface_ReleaseDC(pdds, hdc);
	}

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);

	while ((hres = IDirectDrawSurface_Lock(pdds, NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
		;

	if (hres==DD_OK)
	{
		dw = *(DWORD *) ddsd.lpSurface;
		if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
			dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount) - 1;
		IDirectDrawSurface_Unlock(pdds, NULL);
	}

	if (IDirectDrawSurface_GetDC(pdds, &hdc)==DD_OK)
	{
		SetPixel(hdc, 0, 0, rgbT);
		IDirectDrawSurface_ReleaseDC(pdds, hdc);
	}

	return dw;
}

void ResizeWindow(int width, int height)
{
	// Don't let window get too small so
	// that the trackbar does not become a joke.
	if (width < 320) width = 320;
	if (height < 240) height = 240;
	MoveWindow(hTrack, 0, height, width-4*TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hLeftButton, width-4*TRACK_HEIGHT, height, TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hLeftArrow, width-3*TRACK_HEIGHT, height, TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hRightArrow, width-2*TRACK_HEIGHT, height, TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hRightButton, width-TRACK_HEIGHT, height, TRACK_HEIGHT, TRACK_HEIGHT, true);

	GetWindowRect(hWnd, &wrect);
	GetClientRect(hWnd, &crect);
	Edge_Width = wrect.right - wrect.left - crect.right + crect.left;
	Edge_Height = wrect.bottom - wrect.top - crect.bottom + crect.top;

	MoveWindow(hWnd, wrect.left, wrect.top, width+Edge_Width, height+Edge_Height+TRACK_HEIGHT, true);
}

static void RefreshWindow(bool update)
{
	if (update)
	{
		if (Check_Flag && WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
		{
			Fault_Flag = 0;
			Display_Flag = true;
			Write_Frame(backward_reference_frame, d2v_backward, 0);
		}
	}
	else
		ShowFrame(true);
}

static void SaveBMP()
{
	FILE *BMPFile;

	if (!PopFileDlg(szTemp, hWnd, SAVE_BMP)) return;
	strcat(szTemp, ".bmp");
	if (fopen(szTemp, "r"))
	{
		char line[255];
		sprintf(line, "%s already exists.\nDo you want to replace it?", szTemp);
		if (MessageBox(hWnd, line, "Save BMP",
			MB_YESNO | MB_ICONWARNING) != IDYES)
		return;
	}
	if ((BMPFile = fopen(szTemp, "wb")) == NULL)
		return;

	int width = Coded_Picture_Width;
	int height = Coded_Picture_Height;

	if (ClipResize_Flag)
	{
		width -= Clip_Left+Clip_Right;
		height -= Clip_Top+Clip_Bottom;
	}

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	ZeroMemory(&bmfh, sizeof(BITMAPFILEHEADER));
	bmfh.bfType = 'M'*256 + 'B';
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmfh.bfSize = bmfh.bfOffBits + ((width*24+31)&~31)/8*height;

	ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = width * height * 3;

	fwrite(&bmfh, sizeof(BITMAPFILEHEADER), 1, BMPFile);
	fwrite(&bmih, sizeof(BITMAPINFOHEADER), 1, BMPFile);
	fwrite(rgb24, bmfh.bfSize - bmfh.bfOffBits, 1, BMPFile);

	fclose(BMPFile);
}

static void StartupEnables(void)
{
	// This sets enabled and disabled controls for the startup condition.

	// Main menu.
	EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_ENABLED);

	// File menu.
	EnableMenuItem(hMenu, IDM_OPEN, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_OPEN_AUTO, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_LOAD_D2V, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V_AND_DEMUX, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_BMP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PREVIEW, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_STOP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PAUSE, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_EXIT, MF_ENABLED);

	// Audio menu.
	if (Method_Flag == AUDIO_NONE)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
	}
	else if (Method_Flag == AUDIO_DEMUX)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
	}
	else if (Method_Flag == AUDIO_DEMUXALL)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
	}
	else if (Method_Flag == AUDIO_DECODE)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
	}

	// Other menus are all enabled in the resource file.

	// Drag and drop.
	DragAcceptFiles(hWnd, true);

	// Trackbar and buttons.
	EnableWindow(hTrack, false);
	EnableWindow(hLeftButton, false);
	EnableWindow(hLeftArrow, false);
	EnableWindow(hRightArrow, false);
	EnableWindow(hRightButton, false);

	// Refresh the menu bar.
	DrawMenuBar(hWnd);
}

static void FileLoadedEnables(void)
{
	// Main menu.
	EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_ENABLED);
	if (SystemStream_Flag == TRANSPORT_STREAM)
		EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_ENABLED);
	else
		EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_ENABLED);

	// File menu.
	EnableMenuItem(hMenu, IDM_OPEN, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_OPEN_AUTO, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_LOAD_D2V, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V_AND_DEMUX, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_BMP, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PLAY, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PREVIEW, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_STOP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PAUSE, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_EXIT, MF_ENABLED);

	// Drag and drop.
	DragAcceptFiles(hWnd, true);

	// Trackbar and buttons.
	EnableWindow(hTrack, true);
	EnableWindow(hLeftButton, true);
	EnableWindow(hLeftArrow, true);
	EnableWindow(hRightArrow, true);
	EnableWindow(hRightButton, true);

	// Refresh the menu bar.
	DrawMenuBar(hWnd);
}

static void RunningEnables(void)
{
	// Main menu.
	EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_ENABLED);

	// File menu.
	EnableMenuItem(hMenu, IDM_OPEN, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_OPEN_AUTO, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_LOAD_D2V, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V_AND_DEMUX, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_BMP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PREVIEW, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_STOP, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PAUSE, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_EXIT, MF_ENABLED);

	// Drag and drop.
	DragAcceptFiles(hWnd, false);

	// Trackbar and buttons.
	EnableWindow(hTrack, false);
	EnableWindow(hLeftButton, false);
	EnableWindow(hLeftArrow, false);
	if (PlaybackSpeed == SPEED_SINGLE_STEP)
		EnableWindow(hRightArrow, true);
	else
		EnableWindow(hRightArrow, false);
	EnableWindow(hRightButton, false);

	// Refresh the menu bar.
	DrawMenuBar(hWnd);
}
