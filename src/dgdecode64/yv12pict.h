#ifndef YV12PICT_H
#define YV12PICT_H

#include <cstdint>
#include "avisynth.h"
#include "VapourSynth.h"


class YV12PICT {
    bool allocated;
public:
    uint8_t *y, *u, *v;
    int ypitch, uvpitch;
    int ywidth, uvwidth;
    int yheight, uvheight;
    int pf;

    YV12PICT(PVideoFrame& frame);
    YV12PICT(uint8_t* py, uint8_t* pu, uint8_t*pv, int yw, int cw, int h);
    YV12PICT(int height, int width, int chroma_format);
    YV12PICT::YV12PICT(const VSAPI* vsapi, VSFrameRef* Dst);
    ~YV12PICT();
};

#endif

