/* 
 *	Copyright (C) Chia-chen Kuo - April 2001
 *  Modifications (C) Donald A. Graft - October 2003
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

#include "windows.h"
#include "global.h"
#include "vfapi.h"
#include "avisynth2.h"

HRESULT __stdcall vfGetPluginInfo(LPVF_PluginInfo info);
HRESULT __stdcall vfGetPluginFunc(LPVF_PluginFunc func);

HRESULT __stdcall open_file(char *path, LPVF_FileHandle out);
HRESULT __stdcall close_file(VF_FileHandle p);
HRESULT __stdcall get_file_info(VF_FileHandle in, LPVF_FileInfo out);
HRESULT __stdcall get_stream_info(VF_FileHandle in, DWORD s, void *out);
HRESULT __stdcall read_data(VF_FileHandle in, DWORD s, void *out);

HRESULT __stdcall vfGetPluginInfo(LPVF_PluginInfo info)
{
//	OutputDebugString("DGMPGDec VFAPI: get info\n");
	if (info == NULL)
		return VF_ERROR;

	if (info->dwSize != sizeof(VF_PluginInfo))
		return VF_ERROR;

	info->dwAPIVersion = 1;
	info->dwVersion = 1;
	info->dwSupportStreamType = VF_STREAM_VIDEO;

	strcpy(info->cPluginInfo, "DGMPGDec 1.3.0 D2V Reader");
	strcpy(info->cFileType, "DGIndex Project File (*.d2v)|*.d2v");

	return VF_OK;
}

HRESULT __stdcall vfGetPluginFunc(LPVF_PluginFunc func)
{
//	OutputDebugString("DGMPGDec VFAPI: get funcs\n");
	if (func == NULL)
		return VF_ERROR;

	if (func->dwSize != sizeof(VF_PluginFunc))
		return VF_ERROR;

	func->OpenFile = open_file;
	func->CloseFile = close_file;
	func->GetFileInfo = get_file_info;
	func->GetStreamInfo = get_stream_info;
	func->ReadData = read_data;

	return VF_OK;
}

struct VideoInfo * (__cdecl *openMPEG2Source)(char*); 
unsigned char* (__cdecl *getRGBFrame)(int);
void (__cdecl *closeVideo)();
struct VideoInfo *vi;
D2VFAPI **d2v;
D2VFAPI vf;

HRESULT __stdcall open_file(char *path, LPVF_FileHandle out)
{
	HMODULE hDLL;
	HKEY  hKey;
	DWORD dwSize;
	DWORD dwDataType;
	unsigned char dwValue[1024], *p;

//	OutputDebugString("DGMPGDec VFAPI: open file\n");
//	OutputDebugString(path);
//	OutputDebugString("\n");

    // Make sure this is a D2V file open.
	if ((p = strrchr(path, '.')) == NULL) return VF_ERROR;
	if (toupper(p[1]) != 'D' || p[2] != '2' || toupper(p[3]) != 'V') return VF_ERROR;

	// We won't use the D2V structure, but the VFAPI codec wants it allocated.
	d2v = (D2VFAPI **) out;
	*d2v = &vf;

	// Find the DGDecode.dll decoder plugin. It should be in the same directory
	// as the DGVfapi.vfp plugin.
	if(RegOpenKeyEx(HKEY_CURRENT_USER,
					  "Software\\VFPlugin",
					  0,
					  KEY_QUERY_VALUE,
					  &hKey) == ERROR_SUCCESS)
	{
	  dwSize = sizeof(dwValue);
	  RegQueryValueEx(hKey, "DGIndex", 0, &dwDataType, (unsigned char *) &dwValue, &dwSize);
	  RegCloseKey(hKey);
	}
	p = dwValue + strlen(dwValue);
	while (*p != '\\') p--;
	*++p = 0;
	strcat(dwValue, "dgdecode.dll");
//	{
//		char buf[1024];
//		sprintf(buf, "VFAPI: REG: %s\n",dwValue);
//		OutputDebugString(buf);
//	}

	hDLL = LoadLibrary(dwValue);
	if (!hDLL)
	{
		OutputDebugString("DGMPGDec VFAPI: Could not find dgdecode.dll\n");
		return VF_ERROR;
	}
	
	openMPEG2Source = (struct VideoInfo*  (__cdecl *) (char*)) GetProcAddress(hDLL, "openMPEG2Source");
	getRGBFrame = (unsigned char*  (__cdecl *) (int)) GetProcAddress(hDLL, "getRGBFrame");
	closeVideo = (void  (__cdecl *) ()) GetProcAddress(hDLL, "closeVideo");

	if ( !openMPEG2Source || !getRGBFrame || !closeVideo )
	{
		OutputDebugString("DGMPGDec VFAPI: Could not find functions. Wrong dgdecode.dll?\n");
		return VF_ERROR;
	}

	vi = openMPEG2Source(path);
	if (!vi) return VF_ERROR;

	return VF_OK;
}

HRESULT __stdcall close_file(VF_FileHandle in)
{
//	OutputDebugString("DGMPGDec VFAPI: close file\n");
	if (!in && in == (VF_FileHandle) d2v)
	{
		closeVideo();
	}

	return VF_OK;
}

HRESULT __stdcall get_file_info(VF_FileHandle in, LPVF_FileInfo out)
{
//	OutputDebugString("DGMPGDec VFAPI: get file info\n");
	if (out == NULL)
		return VF_ERROR;

	if (out->dwSize != sizeof(VF_FileInfo))
		return VF_ERROR;

	out->dwHasStreams = VF_STREAM_VIDEO;

	return VF_OK;
}

HRESULT __stdcall get_stream_info(VF_FileHandle in, DWORD stream, void *out)
{
	D2VFAPI *d2v = (D2VFAPI *)in;
	LPVF_StreamInfo_Video info = (LPVF_StreamInfo_Video)out;

//	OutputDebugString("DGMPGDec VFAPI: get stream info\n");
	if (stream != VF_STREAM_VIDEO)
		return VF_ERROR;

	if (info == NULL)
		return VF_ERROR;
	
	if (info->dwSize != sizeof(VF_StreamInfo_Video))
		return VF_ERROR;

	info->dwLengthL = vi->num_frames;

	if (vi->fps_denominator)
		info->dwRate = (vi->fps_numerator * 1000) / vi->fps_denominator;
	else
		info->dwRate = 0;

	info->dwScale = 1000;
	info->dwWidth = vi->width;
	info->dwHeight = vi->height;
	info->dwBitCount = 24;

	return VF_OK;
}
	
HRESULT __stdcall read_data(VF_FileHandle in, DWORD stream, void *out)
{
	D2VFAPI *d2v = (D2VFAPI *)in;
	LPVF_ReadData_Video data = (LPVF_ReadData_Video)out;
	unsigned char *src, *dst = (unsigned char *)data->lpData;
	int y;

//	OutputDebugString("DGMPGDec VFAPI: read data\n");
	if (stream != VF_STREAM_VIDEO)
		return VF_ERROR;

	if (data == NULL)
		return VF_ERROR;
	
	if (data->dwSize != sizeof(VF_ReadData_Video))
		return VF_ERROR;

	src = getRGBFrame(data->dwFrameNumberL);
	for (y = 0; y < vi->height; y++)
		memcpy(dst+y*data->lPitch, src+y*vi->width*3, vi->width*3);

	return VF_OK;
}
