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
#include "filter.h"

#define MAX_WINDOW_WIDTH 1024
#define MAX_WINDOW_HEIGHT 768

__forceinline static void Store_RGB24(unsigned char *src[]);
__forceinline static void Store_YUY2(unsigned char *src[]);

static void FlushRGB24(void);
static void FlushYUY2(void);

static BITMAPINFOHEADER birgb, birgbsmall, biyuv;

static char VideoOut[_MAX_PATH];
static unsigned char *y444;
static int playback, Old_Playback;
static long frame_size;
static bool TFF, RFF, TFB, BFB, frame_type;

static char *FrameType[] = {
	"Interlaced", "Progressive"
};

void Write_Frame(unsigned char *src[], D2VData d2v, DWORD frame)
{
	int repeat;

	if (Fault_Flag)
	{
		if (Fault_Flag < CRITICAL_ERROR_LEVEL)
		{
			SetDlgItemText(hDlg, IDC_INFO, "V.E.!");
			Fault_Flag = 0;		// fault tolerance
		}
		else
		{
			ThreadKill();
		}
	}

	frame_type = d2v.pf;
	TFF = d2v.trf>>1;
	if (FO_Flag == FO_RAW)
		RFF = 0;
	else
		RFF = d2v.trf & 0x01;

	if (!frame)
	{
		char *ext, szTemp[_MAX_PATH];

		TFB = BFB = false;
		playback = Old_Playback = 0;
		frame_size = 0;

		Clip_Width = Coded_Picture_Width;
		Clip_Height = Coded_Picture_Height;
		CLIP_AREA = HALF_CLIP_AREA = CLIP_STEP = CLIP_HALF_STEP = 0;

		if (Luminance_Flag)
			InitializeFilter();

		if (ClipResize_Flag)
		{
			if (Clip_Top || Clip_Bottom || Clip_Left || Clip_Right)
			{
				Clip_Width -= Clip_Left+Clip_Right;
				Clip_Height -= Clip_Top+Clip_Bottom;

				CLIP_AREA = Coded_Picture_Width * Clip_Top;
				HALF_CLIP_AREA = (Coded_Picture_Width>>1) * Clip_Top;

				CLIP_STEP = Coded_Picture_Width * Clip_Top + Clip_Left;
				CLIP_HALF_STEP = (Coded_Picture_Width>>1) * Clip_Top + (Clip_Left>>1);
			}
		}

		NINE_CLIP_WIDTH = Clip_Width * 9;
		QUAD_CLIP_WIDTH = Clip_Width<<2;
		DOUBLE_CLIP_WIDTH = Clip_Width<<1;
		HALF_CLIP_WIDTH = Clip_Width>>1;

		LUM_AREA = Coded_Picture_Width * Clip_Height;
		DOUBLE_WIDTH = Coded_Picture_Width<<1;
		HALF_WIDTH = Coded_Picture_Width>>1;
		HALF_WIDTH_D8 = (Coded_Picture_Width>>1) - 8;
		PROGRESSIVE_HEIGHT = (Coded_Picture_Height>>1) - 2;
		INTERLACED_HEIGHT = (Coded_Picture_Height>>2) - 2;
		RGB_DOWN1 = Clip_Width * (Clip_Height - 1) * 3;
		RGB_DOWN2 = Clip_Width * (Clip_Height - 2) * 3;

		sprintf(szBuffer, "DGIndex - [%d / %d] ", File_Flag+1, File_Limit);
		ext = strrchr(Infilename[File_Flag], '\\');
		strncat(szBuffer, ext+1, strlen(Infilename[0])-(int)(ext-Infilename[0]));

		sprintf(szTemp, " %d x %d", Clip_Width, Clip_Height);
		strcat(szBuffer, szTemp);

		if (Clip_Width%32 == 0)
			strcat(szBuffer, " [32X]");
		else if (Clip_Width%16 == 0)
			strcat(szBuffer, " [16X]");
		else if (Clip_Width%8 == 0)
			strcat(szBuffer, " [8X]");
		else
			strcat(szBuffer, " [ ]");

		if (Clip_Height%32 == 0)
			strcat(szBuffer, "[32X]");
		else if (Clip_Height%16 == 0)
			strcat(szBuffer, "[16X]");
		else if (Clip_Height%8 == 0)
			strcat(szBuffer, "[8X]");
		else
			strcat(szBuffer, "[ ]");

		SetWindowText(hWnd, szBuffer);

		if (Clip_Width > MAX_WINDOW_WIDTH || Clip_Height > MAX_WINDOW_HEIGHT)
			ResizeWindow(Clip_Width/2, Clip_Height/2);
		else
			ResizeWindow(Clip_Width, Clip_Height);

		ZeroMemory(&birgb, sizeof(BITMAPINFOHEADER));
		birgb.biSize = sizeof(BITMAPINFOHEADER);
		birgb.biWidth = Clip_Width;
		birgb.biHeight = Clip_Height;
		birgb.biPlanes = 1;
		birgb.biBitCount = 24;
		birgb.biCompression = BI_RGB;
		birgb.biSizeImage = Clip_Width * Clip_Height * 3;

		ZeroMemory(&birgbsmall, sizeof(BITMAPINFOHEADER));
		birgbsmall.biSize = sizeof(BITMAPINFOHEADER);
		birgbsmall.biWidth = Clip_Width;
		birgbsmall.biHeight = Clip_Height;
		birgbsmall.biPlanes = 1;
		birgbsmall.biBitCount = 24;
		birgbsmall.biCompression = BI_RGB;
		birgbsmall.biSizeImage = Clip_Width * Clip_Height * 3;

		ZeroMemory(&biyuv, sizeof(BITMAPINFOHEADER));
		biyuv = birgb;
		biyuv.biBitCount = 16;
		biyuv.biCompression = mmioFOURCC('Y','U','Y','2');
		biyuv.biSizeImage = Clip_Width * Clip_Height * 2;

		if (FO_Flag!=FO_FILM)
		{
			if (TFF)
				SetDlgItemText(hDlg, IDC_INFO, "T");
			else
				SetDlgItemText(hDlg, IDC_INFO, "B");
		}
	}

	repeat = DetectVideoType(frame, d2v.trf);

	if (FO_Flag != FO_FILM || repeat)
	{
		Store_RGB24(src);
	}

	if (FO_Flag != FO_FILM && repeat==2)
	{
		Store_RGB24(src);
	}

	if (Info_Flag && process.locate==LOCATE_RIP)
	{
		sprintf(szBuffer, "%s", FrameType[frame_type]);
		SetDlgItemText(hDlg, IDC_FRAME_TYPE, szBuffer);

		sprintf(szBuffer, "%s", picture_structure == 3 ? "Frame" : "Field");
		SetDlgItemText(hDlg, IDC_FRAME_STRUCTURE, szBuffer);

		sprintf(szBuffer, "%d", frame+1);
		SetDlgItemText(hDlg, IDC_CODED_NUMBER, szBuffer);
		sprintf(szBuffer, "%d", playback);
		SetDlgItemText(hDlg, IDC_PLAYBACK_NUMBER, szBuffer);

		if ((frame & 63) == 63)
		{
			process.ed = timeGetTime();

			sprintf(szBuffer, "%.1f", 1000.0*(playback-Old_Playback)/(process.ed-process.mi+1));
			SetDlgItemText(hDlg, IDC_FPS, szBuffer);
			sprintf(szBuffer, "%u", (unsigned int) (Bitrate_Monitor * 8 * Frame_Rate) / (playback-Old_Playback));
			SetDlgItemText(hDlg, IDC_BITRATE, szBuffer);
			Bitrate_Monitor = 0;
			process.mi = process.ed;
			Old_Playback = playback;
		}
	}
}

static void Store_RGB24(unsigned char *src[])
{
	if (chroma_format==CHROMA420)
	{
		conv420to422(src[1], u422, frame_type);
		conv420to422(src[2], v422, frame_type);

		conv422to444(u422, u444);
		conv422to444(v422, v444);
	}
	else
	{
		conv422to444(src[1], u444);
		conv422to444(src[2], v444);
	}

	if (Luminance_Flag)
	{
		LuminanceFilter(src[0], lum);
		y444 = lum;
	}
	else
		y444 = src[0];

	if (BFB)
	{
		conv444toRGB24odd(y444, u444, v444, rgb24);
		TFB = true;
		FlushRGB24();

		conv444toRGB24even(y444, u444, v444, rgb24);
		BFB = true;
		FlushRGB24();
	}
	else
	{
		conv444toRGB24even(y444, u444, v444, rgb24);
		BFB = true;
		FlushRGB24();

		conv444toRGB24odd(y444, u444, v444, rgb24);
		TFB = true;
		FlushRGB24();
	}

	if (FO_Flag!=FO_FILM && FO_Flag!=FO_RAW && RFF)
		if (TFF)
		{
			TFB = true;
			FlushRGB24();
		}
		else
		{
			BFB = true;
			FlushRGB24();
		}
}

static void FlushRGB24()
{
	if (TFB & BFB)
	{
		ShowFrame(false);

		playback++;
		TFB = BFB = false;
	}
}

static void Store_YUY2(unsigned char *src[])
{
	if (chroma_format==CHROMA420)
	{
		conv420to422(src[1], u422, frame_type);
		conv420to422(src[2], v422, frame_type);
	}
	else
	{
		u422 = src[1];
		v422 = src[2];
	}

	if (Luminance_Flag)
	{
		LuminanceFilter(src[0], lum);
		y444 = lum;
	}
	else
		y444 = src[0];

	if (BFB)
	{
		conv422toyuy2odd(y444, u422, v422, yuy2);
		TFB = true;
		FlushYUY2();

		conv422toyuy2even(y444, u422, v422, yuy2);
		BFB = true;
		FlushYUY2();
	}
	else
	{
		conv422toyuy2even(y444, u422, v422, yuy2);
		BFB = true;
		FlushYUY2();

		conv422toyuy2odd(y444, u422, v422, yuy2);
		TFB = true;
		FlushYUY2();
	}

	if (FO_Flag!=FO_FILM && FO_Flag!=FO_RAW && RFF)
		if (TFF)
		{
			TFB = true;
			FlushYUY2();
		}
		else
		{
			BFB = true;
			FlushYUY2();
		}
}

static void FlushYUY2()
{
	if (TFB & BFB)
	{
		ShowFrame(false);

		playback++;
		TFB = BFB = false;
	}
}

int DetectVideoType(int frame, int trf)
{
	static int Old_TRF, Repeat_On, Repeat_Off;
	static bool Repeat_Init;

	if (frame)
	{
		if ((trf & 3) == ((Old_TRF+1) & 3))
			FILM_Purity++;
		else
			NTSC_Purity++;
	}
	else
	{
		FILM_Purity = NTSC_Purity = Repeat_On = Repeat_Off = 0;
		Repeat_Init = false;
	}

	Old_TRF = trf;

	if (trf & 1)
		Repeat_On ++;
	else
		Repeat_Off ++;

	if (Repeat_Init)
	{
		if (Repeat_Off-Repeat_On == 5)
		{
			Repeat_Off = Repeat_On = 0;
			return 0;
		}
		else if (Repeat_On-Repeat_Off == 5)
		{
			Repeat_Off = Repeat_On = 0;
			return 2;
		}
	}
	else
	{
		if (Repeat_Off-Repeat_On == 3)
		{
			Repeat_Off = Repeat_On = 0;
			Repeat_Init = true;
			return 0;
		}
		else if (Repeat_On-Repeat_Off == 3)
		{
			Repeat_Off = Repeat_On = 0;
			Repeat_Init = true;
			return 2;
		}
	}

	return 1;
}

void ShowFrame(bool move)
{
	unsigned int x, xx, y, yy, incy, incyy;
	unsigned char *yp, *yyp;
	unsigned int Clip_HeightOver2 = Clip_Height / 2;
	unsigned int Clip_WidthTimes3Over2 = Clip_Width * 3 / 2;
	

	if (!Check_Flag)
		return;

	if (rgb24 && rgb24small && (move || Display_Flag))
	{
		if (Clip_Width > MAX_WINDOW_WIDTH || Clip_Height > MAX_WINDOW_HEIGHT)
		{
			// Zoom out by two in both directions. Quick and dirty.
			yp = rgb24small + (Clip_Height - 1) * Clip_Width * 3;
			yyp = rgb24 + (Clip_Height - 1) * Clip_Width * 3;
			incy = Clip_Width * 3;
			incyy = 2 * Clip_Width * 3;
			for (y = yy = Clip_Height - 1; y >= Clip_HeightOver2; y--, yy-=2)
			{
				for (x = xx = 0; x < Clip_WidthTimes3Over2; x+=3, xx+=6)
				{
					yp[x] = yyp[xx];
					yp[x+1] = yyp[xx+1];
					yp[x+2] = yyp[xx+2];
				}
				yp -= incy;
				yyp -= incyy;
			}
			SetDIBitsToDevice(hDC, 0, 0, Clip_Width / 2, Clip_Height, 0, 0, Clip_Height/2, Clip_Height/2,
							  rgb24small + Clip_Height/2 * Clip_Width * 3, (LPBITMAPINFO) &birgbsmall, DIB_RGB_COLORS);
		}
		else
		{
			SetDIBitsToDevice(hDC, 0, 0, Clip_Width, Clip_Height, 0, 0, 0, Clip_Height,
							  rgb24, (LPBITMAPINFO)&birgb, DIB_RGB_COLORS);
		}
	}
}