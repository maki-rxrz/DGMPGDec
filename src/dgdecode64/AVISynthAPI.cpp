/*
 *  Avisynth 2.5 API for MPEG2Dec3
 *
 *  Changes to fix frame dropping and random frame access are
 *  Copyright (C) 2003-2008 Donald A. Graft
 *
 *  Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *  based of the intial MPEG2Dec Avisytnh API Copyright (C) Mathias Born - May 2001
 *
 *  This file is part of DGMPGDec, a free MPEG-2 decoder
 *
 *  DGMPGDec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  DGMPGDec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <cstring>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <malloc.h>
#include <sstream>

#include "AVISynthAPI.h"
#include "color_convert.h"
#include "misc.h"

bool PutHintingData(uint8_t *video, uint32_t hint)
{
    constexpr uint32_t MAGIC_NUMBER = 0xdeadbeef;

    for (int i = 0; i < 32; ++i) {
        *video &= ~1;
        *video++ |= ((MAGIC_NUMBER & (1 << i)) >> i);
    }
    for (int i = 0; i < 32; i++) {
        *video &= ~1;
        *video++ |= ((hint & (1 << i)) >> i);
    }
    return false;
}


void luminance_filter(uint8_t* luma, const int width, const int height,
    const int pitch, const uint8_t* table)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            luma[x] = table[luma[x]];
        }
        luma += pitch;
    }
}


static void show_info(int n, CMPEG2Decoder& d, PVideoFrame& frame,
    const VideoInfo& vi, IScriptEnvironment* env)
{
    uint32_t raw = std::max(d.FrameList[n].bottom, d.FrameList[n].top);
    if (raw < d.BadStartingFrames)
        raw = d.BadStartingFrames;

    uint32_t gop = 0;
    do {
        if (raw >= d.GOPList[gop].number)
            if (raw < d.GOPList[static_cast<int64_t>(gop)+1].number)
                break;
    } while (++gop < d.GOPList.size() - 1);

    const auto& rgop = d.GOPList[gop];
    const auto& fraw = d.FrameList[raw];
    const auto& fn = d.FrameList[n];

    int pct = fraw.pct == I_TYPE ? 'I' : fraw.pct == B_TYPE ? 'B' : 'P';

    const char* matrix[8] = {
        "Forbidden",
        "ITU-R BT.709",
        "Unspecified Video",
        "Reserved",
        "FCC",
        "ITU-R BT.470-2 System B, G",
        "SMPTE 170M",
        "SMPTE 240M (1987)",
    };

    if (d.info == 1)
    {
        char msg1[1024];
        sprintf_s(msg1, "%s (c) 2005-2021 Donald A. Graft (et al)", VERSION);
        d.DrawString(frame, 0, 0, 0, 0, msg1, 0);
        sprintf_s(msg1, "-----------------------------------------------------");
        d.DrawString(frame, 0, 1, 0, 0, msg1, 0);
        sprintf_s(msg1, "Source:        %s", d.Infilename[rgop.file].c_str());
        d.DrawString(frame, 0, 3, 0, 0, msg1, 0);
        sprintf_s(msg1, "Frame Rate:    %3.6f fps (%u/%u) %s",
            double(d.VF_FrameRate_Num) / double(d.VF_FrameRate_Den), d.VF_FrameRate_Num, d.VF_FrameRate_Den,
            (d.VF_FrameRate == 25000 || d.VF_FrameRate == 50000) ? "(PAL)" :
            d.VF_FrameRate == 29970 ? "(NTSC)" : "");
        d.DrawString(frame, 0, 4, 0, 0, msg1, 0);
        sprintf_s(msg1, "Field Order:   %s", d.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
        d.DrawString(frame, 0, 5, 0, 0, msg1, 0);
        sprintf_s(msg1, "Picture Size:  %d x %d", d.getLumaWidth(), d.getLumaHeight());
        d.DrawString(frame, 0, 6, 0, 0, msg1, 0);
        sprintf_s(msg1, "Aspect Ratio:  %s", d.Aspect_Ratio);
        d.DrawString(frame, 0, 7, 0, 0, msg1, 0);
        sprintf_s(msg1, "Progr Seq:     %s", rgop.progressive ? "True" : "False");
        d.DrawString(frame, 0, 8, 0, 0, msg1, 0);
        sprintf_s(msg1, "GOP Number:    %u (%u)  GOP Pos = %I64d", gop, rgop.number, rgop.position);
        d.DrawString(frame, 0, 9, 0, 0, msg1, 0);
        sprintf_s(msg1, "Closed GOP:    %s", rgop.closed ? "True" : "False");
        d.DrawString(frame, 0, 10, 0, 0, msg1, 0);
        sprintf_s(msg1, "Display Frame: %d", n);
        d.DrawString(frame, 0, 11, 0, 0, msg1, 0);
        sprintf_s(msg1, "Encoded Frame: %d (top) %d (bottom)", fn.top, fn.bottom);
        d.DrawString(frame, 0, 12, 0, 0, msg1, 0);
        sprintf_s(msg1, "Frame Type:    %c (%d)", pct, raw);
        d.DrawString(frame, 0, 13, 0, 0, msg1, 0);
        sprintf_s(msg1, "Progr Frame:   %s", fraw.pf ? "True" : "False");
        d.DrawString(frame, 0, 14, 0, 0, msg1, 0);
        sprintf_s(msg1, "Colorimetry:   %s (%d)", matrix[rgop.matrix], rgop.matrix);
        d.DrawString(frame, 0, 15, 0, 0, msg1, 0);
        sprintf_s(msg1, "Quants:        %d/%d/%d (avg/min/max)", d.avgquant, d.minquant, d.maxquant);
        d.DrawString(frame, 0, 16, 0, 0, msg1, 0);
    }
    else if (d.info == 2)
    {
        dprintf(const_cast<char*>("DGDecode: DGDecode %s (c) 2005-2021 Donald A. Graft (et al)\n"), VERSION);
        dprintf(const_cast<char*>("DGDecode: Source:            %s\n"), d.Infilename[rgop.file].c_str());
        dprintf(const_cast<char*>("DGDecode: Frame Rate:        %3.6f fps (%u/%u) %s\n"),
            double(d.VF_FrameRate_Num) / double(d.VF_FrameRate_Den),
            d.VF_FrameRate_Num, d.VF_FrameRate_Den,
            (d.VF_FrameRate == 25000 || d.VF_FrameRate == 50000) ? "(PAL)" : d.VF_FrameRate == 29970 ? "(NTSC)" : "");
        dprintf(const_cast<char*>("DGDecode: Field Order:       %s\n"), d.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
        dprintf(const_cast<char*>("DGDecode: Picture Size:      %d x %d\n"), d.getLumaWidth(), d.getLumaHeight());
        dprintf(const_cast<char*>("DGDecode: Aspect Ratio:      %s\n"), d.Aspect_Ratio);
        dprintf(const_cast<char*>("DGDecode: Progressive Seq:   %s\n"), rgop.progressive ? "True" : "False");
        dprintf(const_cast<char*>("DGDecode: GOP Number:        %d (%d)  GOP Pos = %I64d\n"), gop, rgop.number, rgop.position);
        dprintf(const_cast<char*>("DGDecode: Closed GOP:        %s\n"), rgop.closed ? "True" : "False");
        dprintf(const_cast<char*>("DGDecode: Display Frame:     %d\n"), n);
        dprintf(const_cast<char*>("DGDecode: Encoded Frame:     %d (top) %d (bottom)\n"), fn.top, fn.bottom);
        dprintf(const_cast<char*>("DGDecode: Frame Type:        %c (%d)\n"), pct, raw);
        dprintf(const_cast<char*>("DGDecode: Progressive Frame: %s\n"), fraw.pf ? "True" : "False");
        dprintf(const_cast<char*>("DGDecode: Colorimetry:       %s (%d)\n"), matrix[rgop.matrix], rgop.matrix);
        dprintf(const_cast<char*>("DGDecode: Quants:            %d/%d/%d (avg/min/max)\n"), d.avgquant, d.minquant, d.maxquant);
    } else if (d.info == 3) {
        constexpr uint32_t PROGRESSIVE = 0x00000001;
        constexpr int COLORIMETRY_SHIFT = 2;

        uint32_t hint = 0;
        if (fraw.pf == 1) hint |= PROGRESSIVE;
        hint |= ((rgop.matrix & 7) << COLORIMETRY_SHIFT);
        PutHintingData(frame->GetWritePtr(PLANAR_Y), hint);
    }
}


MPEG2Source::MPEG2Source(const char* d2v, int idct, bool showQ, int _info, IScriptEnvironment* env) :
    bufY(nullptr), bufU(nullptr), bufV(nullptr), decoder(nullptr)
{
    if (idct > 7)
        env->ThrowError("MPEG2Source: iDCT invalid (1:MMX,2:SSEMMX,3:SSE2,4:FPU,5:REF,6:Skal's,7:Simple)");

    FILE* f;
#ifdef _WIN32
    fopen_s(&f, d2v, "r");
#else
    f = fopen(d2v, "r");
#endif
    if (f == nullptr)
        env->ThrowError("MPEG2Source: unable to load D2V file \"%s\" ", d2v);

    try {
        decoder = new CMPEG2Decoder(f, d2v, idct, _info, showQ);
    }
    catch (std::runtime_error& e) {
        if (f) fclose(f);
        env->ThrowError("MPEG2Source: %s", e.what());
    }

    env->AtExit([](void* p, IScriptEnvironment*) { delete reinterpret_cast<CMPEG2Decoder*>(p); p = nullptr; }, decoder);
    auto& d = *decoder;

    int chroma_format = d.getChromaFormat();

    memset(&vi, 0, sizeof(vi));
    vi.width = d.Clip_Width;
    vi.height = d.Clip_Height;
    if (chroma_format == 2) vi.pixel_type = VideoInfo::CS_YV16;
    else vi.pixel_type = VideoInfo::CS_YV12;

    vi.SetFPS(d.VF_FrameRate_Num, d.VF_FrameRate_Den);
    vi.num_frames = static_cast<int>(d.FrameList.size());
    vi.SetFieldBased(false);

    luminanceFlag = (d.lumGamma != 0 || d.lumOffset != 0);
    if (luminanceFlag) {
        int lg = d.lumGamma;
        int lo = d.lumOffset;
        for (int i = 0; i < 256; ++i) {
            double value = 255.0 * pow(i / 255.0, pow(2.0, -lg / 128.0)) + lo + 0.5;

            if (value < 0)
                luminanceTable[i] = 0;
            else if (value > 255.0)
                luminanceTable[i] = 255;
            else
                luminanceTable[i] = static_cast<uint8_t>(value);
        }
    }

    has_at_least_v8 = true;
    try { env->CheckVersion(8); }
    catch (const AvisynthError&) { has_at_least_v8 = false; }

    std::string ar = d.Aspect_Ratio;
    std::vector<int> sar;
    sar.reserve(2);
    std::stringstream str(ar);
    int n;
    char ch;
    while (str >> n)
    {
        if (str >> ch)
            sar.push_back(n);
        else
            sar.push_back(n);
    }
    int num = sar[0];
    int den = sar[1];
    env->SetVar(env->Sprintf("%s", "FFSAR_NUM"), num);
    env->SetVar(env->Sprintf("%s", "FFSAR_DEN"), den);
    if (num > 0 && den > 0)
        env->SetVar(env->Sprintf("%s", "FFSAR"), num / static_cast<double>(den));
}


bool __stdcall MPEG2Source::GetParity(int)
{
    return decoder->Field_Order == 1;
}


PVideoFrame __stdcall MPEG2Source::GetFrame(int n, IScriptEnvironment* env)
{
    auto& d = *decoder;
    PVideoFrame frame = env->NewVideoFrame(vi, 32);
    YV12PICT out = YV12PICT(frame);

    d.Decode(n, out);

    if (luminanceFlag )
        luminance_filter(out.y, out.ywidth, out.yheight, out.ypitch, luminanceTable);

    if (d.info != 0)
        show_info(n, d, frame, vi, env);

    if (has_at_least_v8)
    {
        AVSMap* props = env->getFramePropsRW(frame);

        /* Sample duration */
        env->propSetInt(props, "_DurationNum", d.VF_FrameRate_Num, 0);
        env->propSetInt(props, "_DurationDen", d.VF_FrameRate_Den, 0);
        /* BFF or TFF */
        uint32_t raw = std::max(d.FrameList[n].bottom, d.FrameList[n].top);
        if (raw < d.BadStartingFrames)
            raw = d.BadStartingFrames;

        uint32_t gop = 0;
        do {
            if (raw >= d.GOPList[gop].number)
                if (raw < d.GOPList[static_cast<int64_t>(gop) + 1].number)
                    break;
        } while (++gop < d.GOPList.size() - 1);

        const auto& rgop = d.GOPList[gop];

        int field_based = 0;
        if (!rgop.progressive)
            field_based = d.Field_Order == 1 ? 2 : 1;
        env->propSetInt(props, "_FieldBased", field_based, 0);
        /* AR */
        env->propSetData(props, "_AspectRatio", d.Aspect_Ratio, strlen(d.Aspect_Ratio), 0);
        /* GOP */
        int64_t gop_number[2] = { gop, rgop.number };
        env->propSetIntArray(props, "_GOPNumber", gop_number, 2);
        env->propSetInt(props, "_GOPPosition", rgop.position, 0);

        int closed_gop = rgop.closed ? 1 : 0;
        env->propSetInt(props, "_GOPClosed", closed_gop, 0);
        /* Encoded frame */
        const auto& fn = d.FrameList[n];

        env->propSetInt(props, "_EncodedFrameTop", fn.top, 0);
        env->propSetInt(props, "_EncodedFrameBottom", fn.bottom, 0);
        /* Picture type */
        const auto& fraw = d.FrameList[raw];
        const char* pct = fraw.pct == I_TYPE ? "I" : fraw.pct == B_TYPE ? "B" : "P";

        env->propSetData(props, "_PictType", pct, 1, 0);
        /* Matrix */
        env->propSetInt(props, "_Matrix", rgop.matrix, 0);
        /* Quants */
        env->propSetInt(props, "_QuantsAverage", d.avgquant, 0);
        env->propSetInt(props, "_QuantsMin", d.minquant, 0);
        env->propSetInt(props, "_QuantsMax", d.maxquant, 0);
    }

    return frame;
}


static void set_user_default(FILE* def, char* d2v, int& idct, bool& showq,
                             int& info)
{
    char buf[512];
    auto load_str = [&buf](char* var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            std::vector<char> tmp(512, 0);
            sscanf_s(buf, name, tmp.data());
            tmp.erase(std::remove(tmp.begin(), tmp.end(), '"'), tmp.end());
            strncpy_s(var, tmp.size() + 1, tmp.data(), tmp.size());
        }
    };
    auto load_int = [&buf](int& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            sscanf_s(buf, name, &var);
        }
    };
    auto load_bool = [&buf](bool& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            char tmp[16];
            sscanf_s(buf, name, &tmp);
            var = tmp[0] == 't';
        }
    };
    auto load_bint = [&buf](int& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            char tmp[16];
            sscanf_s(buf, name, &tmp);
            var = tmp[0] == 't' ? 1 : 0;
        }
    };

    while(fgets(buf, 511, def) != 0) {
        load_str(d2v, "d2v=%s", 4);
        load_int(idct, "idct=%d", 5);
        load_bool(showq, "showQ=%s", 6);
        load_int(info, "info=%d", 5);
    }
}


AVSValue __cdecl MPEG2Source::create(AVSValue args, void*, IScriptEnvironment* env)
{
    char d2v[512];
    int idct = -1;

    bool showQ = false;
    int info = 0;

    FILE* def;
#ifdef _WIN32
    fopen_s(&def, "DGDecode.def", "r");
#else
    def = fopen("DGDecode.def", "r");
#endif
    if (def != nullptr) {
        set_user_default(def, d2v, idct, showQ, info);
        fclose(def);
    }

    // check for uninitialised strings
    if (strlen(d2v) >= _MAX_PATH) d2v[0] = 0;

    MPEG2Source *dec = new MPEG2Source( args[0].AsString(d2v),
                                        args[1].AsInt(idct),
                                        args[2].AsBool(showQ),
                                        args[3].AsInt(info),
                                        env );
    // Only bother invoking crop if we have to.
    auto& d = *dec->decoder;
    if (d.Clip_Top    || d.Clip_Bottom || d.Clip_Left   || d.Clip_Right ||
        // This is cheap but it works. The intent is to allow the
        // display size to be different from the encoded size, while
        // not requiring massive revisions to the code. So we detect the
        // difference and crop it off.
        d.vertical_size != d.Clip_Height || d.horizontal_size != d.Clip_Width ||
        d.vertical_size == 1088)
    {
        int vertical;
        // Special case for 1088 to 1080 as directed by DGIndex.
        if (d.vertical_size == 1088 && d.D2V_Height == 1080)
            vertical = 1080;
        else
            vertical = d.vertical_size;
        AVSValue CropArgs[] = {
            dec,
            d.Clip_Left,
            d.Clip_Top,
            -(d.Clip_Right + (d.Clip_Width - d.horizontal_size)),
            -(d.Clip_Bottom + (d.Clip_Height - vertical)),
            true
        };

        return env->Invoke("crop", AVSValue(CropArgs, 6));
    }

    return dec;
}

const AVS_Linkage* AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    const char* msargs =
        "[d2v]s"
        "[idct]i"
        "[showQ]b"
        "[info]i";

    env->AddFunction("MPEG2Source", msargs, MPEG2Source::create, nullptr);

    return VERSION;
}
