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

#include <windows.h>
#include <stdio.h>
#include "avisynth.h"

#define	VF_STREAM_VIDEO		0x00000001
#define	VF_STREAM_AUDIO		0x00000002
#define	VF_OK				0x00000000
#define	VF_ERROR			0x80004005

typedef	DWORD VF_FileHandle, *LPVF_FileHandle;

typedef	struct {
	DWORD	dwSize;
	DWORD	dwAPIVersion;
	DWORD	dwVersion;
	DWORD	dwSupportStreamType;
	char	cPluginInfo[256];
	char	cFileType[256];
}	VF_PluginInfo, *LPVF_PluginInfo;

typedef	struct {
	DWORD	dwSize;
	DWORD	dwHasStreams;
}	VF_FileInfo, *LPVF_FileInfo;

typedef	struct {
	DWORD	dwSize;
	DWORD	dwLengthL;
	DWORD	dwLengthH;
	DWORD	dwRate;
	DWORD	dwScale;
	DWORD	dwWidth;
	DWORD	dwHeight;
	DWORD	dwBitCount;
}	VF_StreamInfo_Video, *LPVF_StreamInfo_Video;

typedef	struct {
	DWORD	dwSize;
	DWORD	dwLengthL;
	DWORD	dwLengthH;
	DWORD	dwRate;
	DWORD	dwScale;
	DWORD	dwChannels;
	DWORD	dwBitsPerSample;
	DWORD	dwBlockAlign;
}	VF_StreamInfo_Audio, *LPVF_StreamInfo_Audio;

typedef	struct {
	DWORD	dwSize;
	DWORD	dwFrameNumberL;
	DWORD	dwFrameNumberH;
	void	*lpData;
	int		lPitch;
}	VF_ReadData_Video, *LPVF_ReadData_Video;

typedef	struct {
	DWORD	dwSize;
	DWORD	dwSamplePosL;
	DWORD	dwSamplePosH;
	DWORD	dwSampleCount;
	DWORD	dwReadedSampleCount;
	DWORD	dwBufSize;
	void	*lpBuf;
}	VF_ReadData_Audio, *LPVF_ReadData_Audio;

typedef	struct {
	DWORD	dwSize;
	HRESULT (__stdcall *OpenFile)(char *lpFileName, LPVF_FileHandle lpFileHandle);
	HRESULT (__stdcall *CloseFile)(VF_FileHandle hFileHandle);
	HRESULT (__stdcall *GetFileInfo)(VF_FileHandle hFileHandle,LPVF_FileInfo lpFileInfo);
	HRESULT (__stdcall *GetStreamInfo)(VF_FileHandle hFileHandle,DWORD dwStream, void *lpStreamInfo);
	HRESULT (__stdcall *ReadData)(VF_FileHandle hFileHandle, DWORD dwStream, void *lpData); 
}	VF_PluginFunc, *LPVF_PluginFunc;

class vfMI
{
public:
	VideoInfo * (__cdecl *openMPEG2SourceMI)(char*, int); 
	unsigned char* (__cdecl *getRGBFrameMI)(int,int);
	void (__cdecl *closeVideoMI)(int);
	IScriptEnvironment* (__stdcall *CreateScriptEnvironment)(int);
	IScriptEnvironment *avsEnv;
	VideoInfo *vi;
	HMODULE hDLL;
	PClip *clip;
	int type, ident;
	static int instance;
	vfMI *prv, *nxt;
	vfMI::vfMI() : vi(NULL), avsEnv(NULL), clip(NULL), hDLL(NULL), type(-1)
	{
		openMPEG2SourceMI = NULL;
		getRGBFrameMI = NULL;
		closeVideoMI = NULL;
		CreateScriptEnvironment = NULL;
		ident = instance;
		++instance;
		prv = nxt = NULL;
	}
	vfMI::~vfMI()
	{
		if (vi && closeVideoMI) closeVideoMI(ident);
		if (hDLL) FreeLibrary(hDLL);
	}
};

class vfMILinkedList
{
public:
	vfMI *LLB, *LLE;
	vfMILinkedList::vfMILinkedList() : LLB(NULL), LLE(NULL) {};
	vfMILinkedList::~vfMILinkedList()
	{
		for (vfMI *i=LLB; i;)
		{
			vfMI *j = i->nxt;
			if (i->avsEnv)
			{
				if (i->clip) delete i->clip; 
				for (vfMI* k = i->nxt; k; k = k->nxt)
				{
					if (k != i && k->avsEnv == i->avsEnv) 
						goto noavsdelete;
				}
				delete i->avsEnv;
			}
noavsdelete:
			delete i;
			i = j;
		}
		LLE = LLB = NULL;
	}
	void vfMILinkedList::Add(vfMI *i)
	{
		if (!i) return;
		if (!LLB) { LLB = LLE = i; return; }
		i->prv = LLE;
		LLE->nxt = i;
		LLE = i;
	}
	void vfMILinkedList::Remove(vfMI *i)
	{
		if (!i) return;
		if (i->prv)
		{
			if (i->nxt) i->prv->nxt = i->nxt;
			else i->prv->nxt = NULL;
		}
		else if (i == LLB) LLB = i->nxt;
		if (i->nxt)
		{
			if (i->prv) i->nxt->prv = i->prv;
			else i->nxt->prv = NULL;
		}
		else if (i == LLE) LLE = i->prv;
	}
};