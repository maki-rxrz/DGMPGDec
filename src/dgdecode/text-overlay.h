// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

// from AviSynth v2.61 sources

#ifndef __Text_overlay_H__
#define __Text_overlay_H__

// commented out includes and pasted the needed code
// from internal.h and convert.h into this file

//#include "../internal.h"

#include <windows.h>
#include "avisynth.h"


// copy and pasted from internal.h

struct AVSFunction {
  const char* name;
  const char* param_types;
  IScriptEnvironment::ApplyFunc apply;
  void* user_data;
};


int RGB2YUV(int rgb);

PClip Create_MessageClip(const char* message, int width, int height,
  int pixel_type, bool shrink, int textcolor, int halocolor, int bgcolor,
  IScriptEnvironment* env);

PClip new_Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env);
PClip new_SeparateFields(PClip _child, IScriptEnvironment* env);
PClip new_AssumeFrameBased(PClip _child);

// void BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp,
//             int src_pitch, int row_size, int height);

// void asm_BitBlt_ISSE(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);
// void asm_BitBlt_MMX(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);

// long GetCPUFlags();


class _PixelClip {
  enum { buffer=320 };
  BYTE clip[256+buffer*2];
public:
  _PixelClip() {
    memset(clip, 0, buffer);
    for (int i=0; i<256; ++i) clip[i+buffer] = (BYTE)i;
    memset(clip+buffer+256, 255, buffer);
  }
  BYTE operator()(int i) { return clip[i+buffer]; }
};

//extern _PixelClip PixelClip;
static _PixelClip PixelClip;  // made static to fix linking


template<class ListNode>
static __inline void Relink(ListNode* newprev, ListNode* me, ListNode* newnext) {
  if (me == newprev || me == newnext) return;
  me->next->prev = me->prev;
  me->prev->next = me->next;
  me->prev = newprev;
  me->next = newnext;
  me->prev->next = me->next->prev = me;
}



/*** Inline helper methods ***/


static __inline BYTE ScaledPixelClip(int i) {
  return PixelClip((i+32768) >> 16);
}


static __inline bool IsClose(int a, int b, unsigned threshold)
  { return (unsigned(a-b+threshold) <= threshold*2); }


// copy and pasted from convert.h

inline void YUV2RGB(int y, int u, int v, BYTE* out)
{
  const int crv = int(1.596*65536+0.5);
  const int cgv = int(0.813*65536+0.5);
  const int cgu = int(0.391*65536+0.5);
  const int cbu = int(2.018*65536+0.5);

  int scaled_y = (y - 16) * int((255.0/219.0)*65536+0.5);

  out[0] = ScaledPixelClip(scaled_y + (u-128) * cbu);                 // blue
  out[1] = ScaledPixelClip(scaled_y - (u-128) * cgu - (v-128) * cgv); // green
  out[2] = ScaledPixelClip(scaled_y                 + (v-128) * crv); // red
}


inline void YUV2RGB2(int y, int u0, int u1, int v0, int v1, BYTE* out)
{
  const int crv = int(1.596*32768+0.5);
  const int cgv = int(0.813*32768+0.5);
  const int cgu = int(0.391*32768+0.5);
  const int cbu = int(2.018*32768+0.5);

  const int scaled_y = (y - 16) * int((255.0/219.0)*65536+0.5);

  out[0] = ScaledPixelClip(scaled_y + (u0+u1-256) * cbu);                     // blue
  out[1] = ScaledPixelClip(scaled_y - (u0+u1-256) * cgu - (v0+v1-256) * cgv); // green
  out[2] = ScaledPixelClip(scaled_y                     + (v0+v1-256) * crv); // red
}


// not used here, but useful to other filters
inline int RGB2YUV(int rgb)
{
  const int cyb = int(0.114*219/255*65536+0.5);
  const int cyg = int(0.587*219/255*65536+0.5);
  const int cyr = int(0.299*219/255*65536+0.5);

  // y can't overflow
  int y = (cyb*(rgb&255) + cyg*((rgb>>8)&255) + cyr*((rgb>>16)&255) + 0x108000) >> 16;
  int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
  int b_y = ((rgb&255) << 16) - scaled_y;
  int u = ScaledPixelClip((b_y >> 10) * int(1/2.018*1024+0.5) + 0x800000);
  int r_y = (rgb & 0xFF0000) - scaled_y;
  int v = ScaledPixelClip((r_y >> 10) * int(1/1.596*1024+0.5) + 0x800000);
  return ((y*256+u)*256+v) | (rgb & 0xff000000);
}



/********************************************************************
********************************************************************/

// from original text-overlay.h

class Antialiaser
/**
  * Helper class to anti-alias text
 **/
{
public:
  Antialiaser(int width, int height, const char fontname[], int size,
	  int textcolor, int halocolor, int font_width=0, int font_angle=0, bool _interlaced=false);
  virtual ~Antialiaser();
  HDC GetDC();
  void FreeDC();

  void Apply(const VideoInfo& vi, PVideoFrame* frame, int pitch);

private:
  void ApplyYV12(BYTE* buf, int pitch, int UVpitch,BYTE* bufV,BYTE* bufU);
  void ApplyPlanar(BYTE* buf, int pitch, int UVpitch,BYTE* bufV,BYTE* bufU, int shiftX, int shiftY);
  void ApplyYUY2(BYTE* buf, int pitch);
  void ApplyRGB24(BYTE* buf, int pitch);
  void ApplyRGB32(BYTE* buf, int pitch);

  const int w, h;
  HDC hdcAntialias;
  HBITMAP hbmAntialias;
  void* lpAntialiasBits;
  HFONT hfontDefault;
  HBITMAP hbmDefault;
  unsigned short* alpha_calcs;
  bool dirty, interlaced;
  const int textcolor, halocolor;
  int xl, yt, xr, yb; // sub-rectangle containing live text

  void GetAlphaRect();
};



class ShowFrameNumber : public GenericVideoFilter
/**
  * Class to display frame number on a video clip
 **/
{
public:
  ShowFrameNumber(PClip _child, bool _scroll, int _offset, int _x, int _y, const char _fontname[], int _size,
			int _textcolor, int _halocolor, int font_width, int font_angle, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  Antialiaser antialiaser;
  const bool scroll;
  const int offset;
  const int size, x, y;
};



class ShowSMPTE : public GenericVideoFilter
/**
  * Class to display SMPTE codes on a video clip
 **/
{
public:
  ShowSMPTE(PClip _child, double _rate, const char* _offset, int _offset_f, int _x, int _y, const char _fontname[], int _size,
			int _textcolor, int _halocolor, int font_width, int font_angle, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl CreateSMTPE(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateTime(AVSValue args, void*, IScriptEnvironment* env);

private:
  Antialiaser antialiaser;
  int rate;
  int offset_f;
  const int x, y;
  bool dropframe;
};




class Subtitle : public GenericVideoFilter
/**
  * Subtitle creation class
 **/
{
public:
  Subtitle( PClip _child, const char _text[], int _x, int _y, int _firstframe, int _lastframe,
            const char _fontname[], int _size, int _textcolor, int _halocolor, int _align,
            int _spc, bool _multiline, int _lsp, int _font_width, int _font_angle, bool _interlaced );
  virtual ~Subtitle(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  void InitAntialiaser(IScriptEnvironment* env);

  const int x, y, firstframe, lastframe, size, lsp, font_width, font_angle;
  const bool multiline, interlaced;
  const int textcolor, halocolor, align, spc;
  const char* const fontname;
  const char* const text;
  Antialiaser* antialiaser;
};


class FilterInfo : public GenericVideoFilter
/**
  * FilterInfo creation class
 **/
{
public:
  FilterInfo( PClip _child);
  virtual ~FilterInfo(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const VideoInfo& AdjustVi();

  const VideoInfo &vii;
  Antialiaser antialiaser;
};


class Compare : public GenericVideoFilter
/**
  * Compare two clips frame by frame and display fidelity measurements (with optionnal logging to file)
 **/
{
public:
  Compare(PClip _child1, PClip _child2, const char* channels, const char *fname, bool _show_graph, IScriptEnvironment* env);
  ~Compare();
  static AVSValue __cdecl Create(AVSValue args, void* , IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
private:
  Antialiaser antialiaser;
  PClip child2;
  DWORD mask;
  int masked_bytes;
  FILE* log;
  int* psnrs;
  bool show_graph;
  double PSNR_min, PSNR_tot, PSNR_max;
  double MAD_min, MAD_tot, MAD_max;
  double MD_min, MD_tot, MD_max;
  double bytecount_overall, SSD_overall;
  int framecount;
  int planar_plane;
  void Compare_ISSE(DWORD mask, int incr,
	                const BYTE * f1ptr, int pitch1,
				    const BYTE * f2ptr, int pitch2,
					int rowsize, int height,
				    int &SAD_sum, int &SD_sum, int &pos_D,  int &neg_D, double &SSD_sum);

};



/**** Helper functions ****/

void ApplyMessage( PVideoFrame* frame, const VideoInfo& vi, const char* message, int size,
                   int textcolor, int halocolor, int bgcolor, IScriptEnvironment* env );

bool GetTextBoundingBox( const char* text, const char* fontname, int size, bool bold,
                         bool italic, int align, int* width, int* height );




/**** Inline helper functions ****/

inline static HFONT LoadFont(const char name[], int size, bool bold, bool italic, int width=0, int angle=0)
{
  return CreateFont( size, width, angle, angle, bold ? FW_BOLD : FW_NORMAL,
                     italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, name );
}


#endif  // __Text_overlay_H__
