/*
 *  Avisynth 2.5 API for MPEG2Dec3
 *
 *	Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *	based of the intial MPEG2Dec Avisytnh API Copyright (C) Mathias Born - May 2001
 *
 *  This file is part of MPEG2Dec3, a free MPEG-2 decoder
 *
 *  MPEG2Dec3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  MPEG2Dec3 is distributed in the hope that it will be useful,
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
#include "avisynth2.h"
#include "text-overlay.h"

#define uc unsigned char

class MPEG2Source: public IClip {
protected:
  VideoInfo vi;
  int ovr_idct;
  int _PP_MODE;
  YV12PICT *out;
  unsigned char *bufY, *bufU, *bufV; // for 4:2:2 input support

public:
  MPEG2Source(const char* d2v);
  MPEG2Source(const char* d2v, int cpu, int idct, int iPP, int moderate_h, int moderate_v, bool showQ, bool fastMC, const char* _cpu2, bool _info, bool _upConv, IScriptEnvironment* env);
  ~MPEG2Source();

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void GetFrame(int n, unsigned char *buffer, int pitch);

  bool __stdcall GetParity(int n);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {};
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { } ;  // We do not pass cache requests upwards, only to the next filter.
  void override(int ovr_idct);
  void conv422toYUV422_2(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, 
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height);

  CMPEG2Decoder m_decoder;
};

class LumaFilter : public GenericVideoFilter {
	int lumgain,lumoff;
	__int64 LumOffsetMask,LumGainMask;
public:
	LumaFilter(AVSValue args, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

struct YUVRGBScale {
	__int64 RGB_Offset;
	__int64 RGB_Scale;
	__int64 RGB_CBU;
	__int64 RGB_CRV;
	__int64 RGB_CGX;
};

class YV12toRGB24 : public GenericVideoFilter {
	YUVRGBScale cscale;
	uc *u422,*v422,*u444,*v444;
	bool interlaced;
	bool TVscale;
public:
	YV12toRGB24(AVSValue args, IScriptEnvironment* env);
	~YV12toRGB24();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class YV12toYUY2 : public GenericVideoFilter {
	uc *u422,*v422;
	bool interlaced;
public:
	YV12toYUY2(AVSValue args, IScriptEnvironment* env);
	~YV12toYUY2();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class BlindPP : public GenericVideoFilter {
	int* QP;
	bool iPP;
	int PP_MODE;
	int moderate_h, moderate_v;
	YV12PICT *out;
public:
	BlindPP(AVSValue args, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	~BlindPP();
	void BlindPP::convYUV422to422(const unsigned char *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
       int pitch1, int pitch2y, int pitch2uv, int width, int height);
	void BlindPP::conv422toYUV422b(const unsigned char *py, unsigned char *pu, unsigned char *pv, unsigned char *dst, 
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height);
};

void conv420to422(const unsigned char *src, unsigned char *dst, int frame_type, int width, int height);