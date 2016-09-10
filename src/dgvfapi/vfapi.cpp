/*
 *  Copyright (C) Chia-chen Kuo - April 2001
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

#include "vfapi.h"
const AVS_Linkage* AVS_linkage = NULL;      // Unused in DGVFapi.

extern "C" HRESULT __stdcall vfGetPluginInfo(LPVF_PluginInfo info);
extern "C" HRESULT __stdcall vfGetPluginFunc(LPVF_PluginFunc func);
extern "C" HRESULT __stdcall open_file(char *path, LPVF_FileHandle out);
extern "C" HRESULT __stdcall close_file(VF_FileHandle p);
extern "C" HRESULT __stdcall get_file_info(VF_FileHandle in, LPVF_FileInfo out);
extern "C" HRESULT __stdcall get_stream_info(VF_FileHandle in, DWORD s, void *out);
extern "C" HRESULT __stdcall read_data(VF_FileHandle in, DWORD s, void *out);
IScriptEnvironment* findOpenAVSEnv();
void closeAVS(vfMI *j);

int vfMI::instance = 0;

vfMILinkedList g_LL; // used to make sure everything gets free'd/released

extern "C" HRESULT __stdcall vfGetPluginInfo(LPVF_PluginInfo info)
{
    if (info == NULL)
        return VF_ERROR;

    if (info->dwSize != sizeof(VF_PluginInfo))
        return VF_ERROR;

    info->dwAPIVersion = 1;
    info->dwVersion = 1;
    info->dwSupportStreamType = VF_STREAM_VIDEO;
    info->dwSupportStreamType |= VF_STREAM_AUDIO;

    strcpy(info->cPluginInfo, "DGMPGDec 1.5.8 D2V/AVS Reader");
    strcpy(info->cFileType, "DGIndex or AviSynth File (*.d2v;*.avs)|*.d2v;*.avs");

    return VF_OK;
}

extern "C" HRESULT __stdcall vfGetPluginFunc(LPVF_PluginFunc func)
{
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

IScriptEnvironment* findOpenAVSEnv()
{
    for (vfMI* i = g_LL.LLB; i; i = i->nxt)
    {
        if (i->avsEnv != NULL)
            return i->avsEnv;
    }
    return NULL;
}

void closeAVS(vfMI *j)
{
    if (!j || !j->avsEnv) return;
    if (j->clip)
    {
        delete j->clip;
        j->clip = NULL;
    }
    for (vfMI* i = g_LL.LLB; i; i = i->nxt)
    {
        if (i != j && i->avsEnv == j->avsEnv)
        {
            j->avsEnv = NULL;
            return;
        }
    }
    delete j->avsEnv;
    j->avsEnv = NULL;
}

#define D2V_TYPE 0
#define AVS_TYPE 1

extern "C" HRESULT __stdcall open_file(char *path, LPVF_FileHandle out)
{
    HKEY  hKey;
    DWORD dwSize, dwDataType;
    unsigned char dwValue[1024], *p, ttype;

    char buf[512];
    sprintf(buf,"DGVfapi: attempt to open file: %s.\n", path);
    OutputDebugString(buf);

    if ((p = (unsigned char *)strrchr(path, '.')) == NULL) return VF_ERROR;

    if (toupper(p[1]) != 'D' || p[2] != '2' || toupper(p[3]) != 'V')
    {
        if (toupper(p[1]) != 'A' || toupper(p[2]) != 'V' || toupper(p[3]) != 'S')
            return VF_ERROR;
        else ttype = AVS_TYPE;
    }
    else ttype = D2V_TYPE;

    vfMI **i = (vfMI**)out;

    *i = new vfMI();

    if (!(*i)) return VF_ERROR;

    g_LL.Add(*i);

    vfMI *j = *i;

    j->type = ttype;

    if (j->type == D2V_TYPE) // D2V File
    {
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
        p = dwValue + strlen((const char *)dwValue);
        while (*p != '\\') p--;
        *++p = 0;
        strcat((char *)dwValue, "dgdecode.dll");

        j->hDLL = LoadLibrary((LPCSTR)dwValue);
        if (!(j->hDLL))
        {
            OutputDebugString("DGVfapi: Could not find DGDecode.dll!\n");
            return VF_ERROR;
        }

        j->openMPEG2SourceMI = (VideoInfo*  (__cdecl *) (char*,int)) GetProcAddress(j->hDLL, "openMPEG2SourceMI");
        j->getRGBFrameMI = (unsigned char*  (__cdecl *) (int,int)) GetProcAddress(j->hDLL, "getRGBFrameMI");
        j->closeVideoMI = (void  (__cdecl *) (int)) GetProcAddress(j->hDLL, "closeVideoMI");

        if (!j->openMPEG2SourceMI || !j->getRGBFrameMI || !j->closeVideoMI)
        {
            OutputDebugString("DGVfapi: Could not find functions! Wrong DGDecode.dll?\n");
            return VF_ERROR;
        }

        j->vi = j->openMPEG2SourceMI(path,j->ident);
        if (!j->vi) return VF_ERROR;
    }
    else // AVS File
    {
        j->hDLL = LoadLibrary(TEXT("avisynth.dll"));
        if (!(j->hDLL))
        {
            OutputDebugString("DGVfapi: Could not find avisynth.dll!\n");
            return VF_ERROR;
        }

        j->CreateScriptEnvironment = (IScriptEnvironment *(__stdcall *)(int))GetProcAddress(j->hDLL, TEXT("CreateScriptEnvironment"));

        if (!j->CreateScriptEnvironment)
        {
            OutputDebugString("DGVfapi: Could not find functions! Wrong avisynth.dll?\n");
            return VF_ERROR;
        }

        // look for a currently open IScriptEnvironment first
        j->avsEnv = findOpenAVSEnv();

        // else create a new one
        if (!j->avsEnv)
            j->avsEnv = j->CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION);

        if (!j->avsEnv)
        {
            OutputDebugString("DGVfapi: Could not create script environment!\n");
            return VF_ERROR;
        }

        try
        {
            j->clip = new PClip();
            if (!j->clip)
            {
                closeAVS(j);
                return VF_ERROR;
            }

            AVSValue args1[1] = { path };
            *(j->clip) = j->avsEnv->Invoke("Import", AVSValue(args1, 1)).AsClip();

            AVSValue args2[1] = { *(j->clip) };
            *(j->clip) = j->avsEnv->Invoke("FlipVertical", AVSValue(args2, 1)).AsClip();

            // Convert to RGB24 if the script isn't already outputting RGB24
            if (!(*j->clip)->GetVideoInfo().IsRGB24())
            {
                AVSValue args3[1] = { *(j->clip) };
                *(j->clip) = j->avsEnv->Invoke("ConvertToRGB24", AVSValue(args3, 1)).AsClip();
            }

            // Read the first frame to make sure that everything works
            PVideoFrame temp = (*j->clip)->GetFrame(0, j->avsEnv);
            if ((*j->clip)->GetVideoInfo().HasAudio() && (*j->clip)->GetVideoInfo().SampleType() != 2)
            {
                AVSValue args4[1] = { *(j->clip) };
                *(j->clip) = j->avsEnv->Invoke("ConvertAudioTo16bit", AVSValue(args4, 1)).AsClip();
            }
        }
        catch (AvisynthError e)
        {
            OutputDebugString("DGVfapi: error (Avisynth Error) loading avisynth script!\n");
            sprintf(buf,"DGVfapi: %s.\n", e.msg);
            OutputDebugString(buf);
            closeAVS(j);
            return VF_ERROR;
        }
        catch (...)
        {
            OutputDebugString("DGVfapi: error (Unknown) loading avisynth script!\n");
            closeAVS(j);
            return VF_ERROR;
        }
    }

    OutputDebugString("DGVfapi: open file succeeded!\n");

    return VF_OK;
}

extern "C" HRESULT __stdcall close_file(VF_FileHandle in)
{
    vfMI *i = (vfMI *)in;

    g_LL.Remove(i);

    closeAVS(i);

    delete i;

    return VF_OK;
}

extern "C" HRESULT __stdcall get_file_info(VF_FileHandle in, LPVF_FileInfo out)
{
    if (out == NULL)
        return VF_ERROR;

    if (out->dwSize != sizeof(VF_FileInfo))
        return VF_ERROR;

    vfMI *i = (vfMI*)in;

    if (i->type == D2V_TYPE)
        out->dwHasStreams = VF_STREAM_VIDEO;
    else
    {
        out->dwHasStreams = 0;
        if ((*i->clip)->GetVideoInfo().HasVideo()) out->dwHasStreams |= VF_STREAM_VIDEO;
        if ((*i->clip)->GetVideoInfo().HasAudio()) out->dwHasStreams |= VF_STREAM_AUDIO;
    }

    return VF_OK;
}

extern "C" HRESULT __stdcall get_stream_info(VF_FileHandle in, DWORD stream, void *out)
{
    if (stream == VF_STREAM_VIDEO)
    {
        LPVF_StreamInfo_Video info = (LPVF_StreamInfo_Video)out;

        if (info == NULL)
            return VF_ERROR;

        if (info->dwSize != sizeof(VF_StreamInfo_Video))
            return VF_ERROR;

        vfMI *i = (vfMI*)in;

        if (i->type == D2V_TYPE)
        {
            info->dwLengthL = i->vi->num_frames;
            if (i->vi->fps_denominator) info->dwRate = i->vi->fps_numerator;
            else info->dwRate = 0;
            info->dwScale = i->vi->fps_denominator;
            info->dwWidth = i->vi->width;
            info->dwHeight = i->vi->height;
        }
        else
        {
            const VideoInfo vit = (*i->clip)->GetVideoInfo();
            info->dwLengthL = vit.num_frames;
            if (vit.fps_denominator) info->dwRate = vit.fps_numerator;
            else info->dwRate = 0;
            info->dwScale = vit.fps_denominator;
            info->dwWidth = vit.width;
            info->dwHeight = vit.height;
        }
        info->dwBitCount = 24;
    }
    else if (stream == VF_STREAM_AUDIO)
    {
        LPVF_StreamInfo_Audio info = (LPVF_StreamInfo_Audio)out;

        if (info == NULL)
            return VF_ERROR;

        if (info->dwSize != sizeof(VF_StreamInfo_Audio))
            return VF_ERROR;

        vfMI *i = (vfMI*)in;

        if (i->type == D2V_TYPE)
            return VF_ERROR;
        else
        {
            const VideoInfo vit = (*i->clip)->GetVideoInfo();
            if (!vit.HasAudio()) return VF_ERROR;
            info->dwLengthL = (unsigned long)vit.num_audio_samples;
            info->dwChannels = vit.nchannels;
            info->dwRate = vit.audio_samples_per_second * vit.BytesPerAudioSample();
            info->dwScale = vit.BytesPerAudioSample();
            info->dwBitsPerSample = vit.BytesPerChannelSample()*8;
            info->dwBlockAlign = vit.BytesPerAudioSample();
        }
    }
    else return VF_ERROR;

    return VF_OK;
}

extern "C" HRESULT __stdcall read_data(VF_FileHandle in, DWORD stream, void *out)
{
    if (stream == VF_STREAM_VIDEO)
    {
        LPVF_ReadData_Video data = (LPVF_ReadData_Video)out;
        unsigned char *dst = (unsigned char *)data->lpData;

        if (data == NULL)
            return VF_ERROR;

        if (data->dwSize != sizeof(VF_ReadData_Video))
            return VF_ERROR;

        vfMI *i = (vfMI*)in;

        if (i->type == D2V_TYPE)
        {
            unsigned char *src = i->getRGBFrameMI(data->dwFrameNumberL, i->ident);
            int y, height = i->vi->height;
            for (y = 0; y < height; y++)
                memcpy(dst+y*data->lPitch, src+y*i->vi->width*3, i->vi->width*3);
        }
        else
        {
            PVideoFrame src = (*i->clip)->GetFrame(data->dwFrameNumberL, i->avsEnv);
            i->avsEnv->BitBlt(dst, data->lPitch, src->GetReadPtr(), src->GetPitch(),
                src->GetRowSize(), src->GetHeight());
        }
    }
    else if (stream == VF_STREAM_AUDIO)
    {
        LPVF_ReadData_Audio data = (LPVF_ReadData_Audio)out;

        if (data == NULL)
            return VF_ERROR;

        if (data->dwSize != sizeof(VF_ReadData_Audio))
            return VF_ERROR;

        vfMI *i = (vfMI*)in;

        if (i->type == D2V_TYPE)
            return VF_ERROR;
        else
        {
            if (!(*i->clip)->GetVideoInfo().HasAudio()) return VF_ERROR;
            (*i->clip)->GetAudio(data->lpBuf, data->dwSamplePosL, data->dwSampleCount, i->avsEnv);
            data->dwReadedSampleCount = (unsigned long) min(data->dwSampleCount,
                max((*i->clip)->GetVideoInfo().num_audio_samples-data->dwSamplePosL,0));
        }
    }
    else return VF_ERROR;

    return VF_OK;
}
