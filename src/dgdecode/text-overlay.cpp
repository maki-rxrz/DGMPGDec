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


// from AviSynth v2.55 sources


#include "text-overlay.h"

#include<string>
#include<sstream>

using namespace std;



/******************************
 *******   Anti-alias    ******
 *****************************/

Antialiaser::Antialiaser(int width, int height, const char fontname[], int size)
 : w(width), h(height)
{
  struct {
    BITMAPINFOHEADER bih;
    RGBQUAD clr[2];
  } b;

  b.bih.biSize                    = sizeof(BITMAPINFOHEADER);
  b.bih.biWidth                   = width * 8 + 32;
  b.bih.biHeight                  = height * 8 + 32;
  b.bih.biBitCount                = 1;
  b.bih.biPlanes                  = 1;
  b.bih.biCompression             = BI_RGB;
  b.bih.biXPelsPerMeter   = 0;
  b.bih.biYPelsPerMeter   = 0;
  b.bih.biClrUsed                 = 2;
  b.bih.biClrImportant    = 2;
  b.clr[0].rgbBlue = b.clr[0].rgbGreen = b.clr[0].rgbRed = 0;
  b.clr[1].rgbBlue = b.clr[1].rgbGreen = b.clr[1].rgbRed = 255;

  hdcAntialias = CreateCompatibleDC(NULL);
  hbmAntialias = CreateDIBSection
    ( hdcAntialias,
      (BITMAPINFO *)&b,
      DIB_RGB_COLORS,
      &lpAntialiasBits,
      NULL,
      0 );
  hbmDefault = (HBITMAP)SelectObject(hdcAntialias, hbmAntialias);

  HFONT newfont = LoadFont(fontname, size, true, false);
  hfontDefault = (HFONT)SelectObject(hdcAntialias, newfont);

  SetMapMode(hdcAntialias, MM_TEXT);
  SetTextColor(hdcAntialias, 0xffffff);
  SetBkColor(hdcAntialias, 0);

  alpha_bits = new char[width*height*2];

  dirty = true;
}


Antialiaser::~Antialiaser() {
  DeleteObject(SelectObject(hdcAntialias, hbmDefault));
  DeleteObject(SelectObject(hdcAntialias, hfontDefault));
  DeleteDC(hdcAntialias);
  delete[] alpha_bits;
}


HDC Antialiaser::GetDC() {
  dirty = true;
  return hdcAntialias;
}


void Antialiaser::Apply( const VideoInfo& vi, PVideoFrame* frame, int pitch, int textcolor, 
                         int halocolor ) 
{
  if (vi.IsRGB32())
    ApplyRGB32((*frame)->GetWritePtr(), pitch, textcolor, halocolor);
  else if (vi.IsRGB24())
    ApplyRGB24((*frame)->GetWritePtr(), pitch, textcolor, halocolor);
  else if (vi.IsYUY2())
    ApplyYUY2((*frame)->GetWritePtr(), pitch, textcolor, halocolor);
  else if (vi.IsYV12())
    ApplyYV12((*frame)->GetWritePtr(), pitch, textcolor, halocolor, (*frame)->GetPitch(PLANAR_U),(*frame)->GetWritePtr(PLANAR_U),(*frame)->GetWritePtr(PLANAR_V));
}

void Antialiaser::ApplyYV12(BYTE* buf, int pitch, int textcolor, int halocolor, int pitchUV, BYTE* bufU, BYTE* bufV) {
  if (dirty) GetAlphaRect();
  int Ytext = ((textcolor>>16)&255), Utext = ((textcolor>>8)&255), Vtext = (textcolor&255);
  int Yhalo = ((halocolor>>16)&255), Uhalo = ((halocolor>>8)&255), Vhalo = (halocolor&255);
  int w2 = w*2;
  char* alpha = alpha_bits;
  int h_half = h/2;

  for (int y=0; y<h_half; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;  // Supersampled (alpha) x position.
      if (*(__int32*)&alpha[x2] || *(__int32*)&alpha[x2+w2]) {
        buf[x+0] = (buf[x+0] * (64-alpha[x2+0]-alpha[x2+1]) + Ytext * alpha[x2+0] + Yhalo * alpha[x2+1]) >> 6;
        buf[x+1] = (buf[x+1] * (64-alpha[x2+2]-alpha[x2+3]) + Ytext * alpha[x2+2] + Yhalo * alpha[x2+3]) >> 6;

        buf[x+0+pitch] = (buf[x+0+pitch] * (64-alpha[x2+0+((w2))]-alpha[x2+1+(w2)]) + Ytext * alpha[x2+0+w2] + Yhalo * alpha[x2+1+w2]) >> 6;
        buf[x+1+pitch] = (buf[x+1+pitch] * (64-alpha[x2+2+(w2)]-alpha[x2+3+(w2)]) + Ytext * alpha[x2+2+w2] + Yhalo * alpha[x2+3+w2]) >> 6;

        int auv1 = (alpha[x2] + alpha[x2+2] + alpha[x2+w2] + alpha[x2+2+w2]) >> 1;
        int auv2 = (alpha[x2+1] + alpha[x2+3] + alpha[x2+1+w2] + alpha[x2+3+w2]) >> 1;

        bufU[x>>1] = (bufU[x>>1] * (128-auv1-auv2) + Utext * auv1 + Uhalo * auv2) >> 7;
        bufV[x>>1] = (bufV[x>>1] * (128-auv1-auv2) + Vtext * auv1 + Vhalo * auv2) >> 7;
      }
    }
    buf += pitch*2;
    bufU += pitchUV;
    bufV += pitchUV;
    alpha += w*4;
  }
}


void Antialiaser::ApplyYUY2(BYTE* buf, int pitch, int textcolor, int halocolor) {
  if (dirty) GetAlphaRect();
  int Ytext = ((textcolor>>16)&255), Utext = ((textcolor>>8)&255), Vtext = (textcolor&255);
  int Yhalo = ((halocolor>>16)&255), Uhalo = ((halocolor>>8)&255), Vhalo = (halocolor&255);
  char* alpha = alpha_bits;
  for (int y=h; y>0; --y) {
    for (int x=0; x<w; x+=2) {
      if (*(__int32*)&alpha[x*2]) {
        buf[x*2+0] = (buf[x*2+0] * (64-alpha[x*2+0]-alpha[x*2+1]) + Ytext * alpha[x*2+0] + Yhalo * alpha[x*2+1]) >> 6;
        buf[x*2+2] = (buf[x*2+2] * (64-alpha[x*2+2]-alpha[x*2+3]) + Ytext * alpha[x*2+2] + Yhalo * alpha[x*2+3]) >> 6;
        int auv1 = alpha[x*2]+alpha[x*2+2];
        int auv2 = alpha[x*2+1]+alpha[x*2+3];
        buf[x*2+1] = (buf[x*2+1] * (128-auv1-auv2) + Utext * auv1 + Uhalo * auv2) >> 7;
        buf[x*2+3] = (buf[x*2+3] * (128-auv1-auv2) + Vtext * auv1 + Vhalo * auv2) >> 7;
      }
    }
    buf += pitch;
    alpha += w*2;
  }
}


void Antialiaser::ApplyRGB24(BYTE* buf, int pitch, int textcolor, int halocolor) {
  if (dirty) GetAlphaRect();
  int Rtext = ((textcolor>>16)&255), Gtext = ((textcolor>>8)&255), Btext = (textcolor&255);
  int Rhalo = ((halocolor>>16)&255), Ghalo = ((halocolor>>8)&255), Bhalo = (halocolor&255);
  char* alpha = alpha_bits + (h-1)*w*2;
  for (int y=h; y>0; --y) {
    for (int x=0; x<w; ++x) {
      int textalpha = alpha[x*2+0];
      int haloalpha = alpha[x*2+1];
      if (textalpha | haloalpha) {
        buf[x*3+0] = (buf[x*3+0] * (64-textalpha-haloalpha) + Btext * textalpha + Bhalo * haloalpha) >> 6;
        buf[x*3+1] = (buf[x*3+1] * (64-textalpha-haloalpha) + Gtext * textalpha + Ghalo * haloalpha) >> 6;
        buf[x*3+2] = (buf[x*3+2] * (64-textalpha-haloalpha) + Rtext * textalpha + Rhalo * haloalpha) >> 6;
      }
    }
    buf += pitch;
    alpha -= w*2;
  }
}


void Antialiaser::ApplyRGB32(BYTE* buf, int pitch, int textcolor, int halocolor) {
  if (dirty) GetAlphaRect();
  int Rtext = ((textcolor>>16)&255), Gtext = ((textcolor>>8)&255), Btext = (textcolor&255);
  int Rhalo = ((halocolor>>16)&255), Ghalo = ((halocolor>>8)&255), Bhalo = (halocolor&255);
  char* alpha = alpha_bits + (h-1)*w*2;
  for (int y=h; y>0; --y) {
    for (int x=0; x<w; ++x) {
      int textalpha = alpha[x*2+0];
      int haloalpha = alpha[x*2+1];
      if (textalpha | haloalpha) {
        buf[x*4+0] = (buf[x*4+0] * (64-textalpha-haloalpha) + Btext * textalpha + Bhalo * haloalpha) >> 6;
        buf[x*4+1] = (buf[x*4+1] * (64-textalpha-haloalpha) + Gtext * textalpha + Ghalo * haloalpha) >> 6;
        buf[x*4+2] = (buf[x*4+2] * (64-textalpha-haloalpha) + Rtext * textalpha + Rhalo * haloalpha) >> 6;
      }
    }
    buf += pitch;
    alpha -= w*2;
  }
}


void Antialiaser::GetAlphaRect() {

  dirty = false;

  static BYTE bitcnt[256],    // bit count
              bitexl[256],    // expand to left bit
              bitexr[256];    // expand to right bit
  static bool fInited = false;

  if (!fInited) {
    int i;

    for(i=0; i<256; i++) {
      BYTE b=0, l=0, r=0;

      if (i&  1) { b=1; l|=0x01; r|=0xFF; }
      if (i&  2) { ++b; l|=0x03; r|=0xFE; }
      if (i&  4) { ++b; l|=0x07; r|=0xFC; }
      if (i&  8) { ++b; l|=0x0F; r|=0xF8; }
      if (i& 16) { ++b; l|=0x1F; r|=0xF0; }
      if (i& 32) { ++b; l|=0x3F; r|=0xE0; }
      if (i& 64) { ++b; l|=0x7F; r|=0xC0; }
      if (i&128) { ++b; l|=0xFF; r|=0x80; }

      bitcnt[i] = b;
      bitexl[i] = l;
      bitexr[i] = r;
    }

    fInited = true;
  }

  int srcpitch = (w+4+3) & -4;

  char* dst = alpha_bits;
  for (int y=0; y<h; ++y) {
    BYTE* src = (BYTE*)lpAntialiasBits + ((h-y-1)*8 + 20) * srcpitch + 2;
    int wt = w;
    do {
      int alpha1, alpha2;
      int i;
      BYTE bmasks[8], tmasks[8];

      alpha1 = alpha2 = 0;

/*      BYTE tmp = 0;
      for (i=0; i<8; i++) {
        tmp |= src[srcpitch*i];
        tmp |= src[srcpitch*i-1];
        tmp |= src[srcpitch*i+1];
        tmp |= src[srcpitch*(-8+i)];
        tmp |= src[srcpitch*(-8+i)-1];
        tmp |= src[srcpitch*(-8+i)+1];
        tmp |= src[srcpitch*(8+i)];
        tmp |= src[srcpitch*(8+i)-1];
        tmp |= src[srcpitch*(8+i)+1];
      }
*/
      DWORD tmp;
      __asm {           // test if the whole area isn't just plain black
        mov edx, srcpitch
        mov esi, src
        mov ecx, edx
        dec esi
        shl ecx, 3
        sub esi, ecx  ; src - 8*pitch - 1

        mov eax, [esi]  ; repeat 24 times
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]
        add esi, edx
        or eax, [esi]

        and eax, 0x00ffffff
        mov tmp, eax
      }

      if (tmp != 0) {     // quick exit in a common case
        for (i=0; i<8; i++)
          alpha1 += bitcnt[src[srcpitch*i]];

        BYTE cenmask = 0, mask1, mask2;

        for(i=0; i<=8; i++) {
          cenmask |= (BYTE)(((long)-src[srcpitch*i  ])>>31);
          cenmask |= bitexl[src[srcpitch*i-1]];
          cenmask |= bitexr[src[srcpitch*i+1]];
        }

        mask1 = mask2 = cenmask;

        for(i=0; i<8; i++) {
          mask1 |= (BYTE)(((long)-src[srcpitch*(-i)])>>31);
          mask1 |= bitexl[src[srcpitch*(-8+i)-1]];
          mask1 |= bitexr[src[srcpitch*(-8+1)+1]];
          mask2 |= (BYTE)(((long)-src[srcpitch*(8+i)])>>31);
          mask2 |= bitexl[src[srcpitch*(8+i)-1]];
          mask2 |= bitexr[src[srcpitch*(8+i)+1]];

          tmasks[i] = mask1;
          bmasks[i] = mask2;
        }

        for(i=0; i<8; i++) {
          alpha2 += bitcnt[cenmask | tmasks[7-i] | bmasks[i]];
        }
      }

      dst[0] = alpha1;
      dst[1] = alpha2-alpha1;
      dst += 2;
      ++src;
    } while(--wt);
  }
}



/************************************
 *******   Helper Functions    ******
 ***********************************/


void ApplyMessage( PVideoFrame* frame, const VideoInfo& vi, const char* message, int size, 
                   int textcolor, int halocolor, int bgcolor, IScriptEnvironment* env ) 
{
  Antialiaser antialiaser(vi.width, vi.height, "Arial", size);
  HDC hdcAntialias = antialiaser.GetDC();
  RECT r = { 4*8, 4*8, (vi.width-4)*8, (vi.height-4)*8 };
  DrawText(hdcAntialias, message, lstrlen(message), &r, DT_NOPREFIX|DT_LEFT|DT_WORDBREAK);
  GdiFlush();
  if (vi.IsYUV()) {
    textcolor = RGB2YUV(textcolor);
    halocolor = RGB2YUV(halocolor);
  }
  antialiaser.Apply(vi, frame, (*frame)->GetPitch(), textcolor, halocolor);
}