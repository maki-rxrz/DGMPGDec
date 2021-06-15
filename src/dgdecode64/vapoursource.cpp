/*
*  DGDecodeNV -- frame accurate AVC/VC1/MPEG decoder/frame server
*  Copyright (C) 2007-2018 Donald A. Graft
*/

#include <string>
#include "global.h"
#include "vapoursource.h"
#include "VSHelper.h"
#include <stdint.h>
#include "MPEG2Decoder.h"
#include "yv12pict.h"
#include <sstream>
#include <algorithm>
#include "misc.h"

unsigned int gcd(unsigned int a, unsigned int b)
{
	if (b == 0)
		return a;
	return gcd(b, a % b);
}

static void VSshow_info(int n, CMPEG2Decoder *d, const VSAPI *vsapi, VSFrameRef *frame,
	const VSVideoInfo *vi, unsigned char *video)
{
	uint32_t raw = max(d->FrameList[n].bottom, d->FrameList[n].top);
	if (raw < d->BadStartingFrames)
		raw = d->BadStartingFrames;

	uint32_t gop = 0;
	do {
		if (raw >= d->GOPList[gop].number)
			if (raw < d->GOPList[static_cast<int64_t>(gop) + 1].number)
				break;
	} while (++gop < d->GOPList.size() - 1);

	const auto& rgop = d->GOPList[gop];
	const auto& fraw = d->FrameList[raw];
	const auto& fn = d->FrameList[n];

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

	if (d->info == 1)
	{
		char msg1[1024];
		sprintf_s(msg1, "%s (c) 2005-2021 Donald A. Graft (et al)", VERSION);
		d->VSDrawString(frame, vsapi, 0, 0, 0, 0, msg1, 0);
		sprintf_s(msg1, "-----------------------------------------------------");
		d->VSDrawString(frame, vsapi, 0, 1, 0, 0, msg1, 0);
		sprintf_s(msg1, "Source:        %s", d->Infilename[rgop.file].c_str());
		d->VSDrawString(frame, vsapi, 0, 3, 0, 0, msg1, 0);
		sprintf_s(msg1, "Frame Rate:    %3.6f fps (%u/%u) %s",
			double(d->VF_FrameRate_Num) / double(d->VF_FrameRate_Den), d->VF_FrameRate_Num, d->VF_FrameRate_Den,
			(d->VF_FrameRate == 25000 || d->VF_FrameRate == 50000) ? "(PAL)" :
			d->VF_FrameRate == 29970 ? "(NTSC)" : "");
		d->VSDrawString(frame, vsapi, 0, 4, 0, 0, msg1, 0);
		sprintf_s(msg1, "Field Order:   %s", d->Field_Order == 1 ? "Top Field First" : "Bottom Field First");
		d->VSDrawString(frame, vsapi, 0, 5, 0, 0, msg1, 0);
		sprintf_s(msg1, "Picture Size:  %d x %d", d->getLumaWidth(), d->getLumaHeight());
		d->VSDrawString(frame, vsapi, 0, 6, 0, 0, msg1, 0);
		sprintf_s(msg1, "Aspect Ratio:  %s", d->Aspect_Ratio);
		d->VSDrawString(frame, vsapi, 0, 7, 0, 0, msg1, 0);
		sprintf_s(msg1, "Progr Seq:     %s", rgop.progressive ? "True" : "False");
		d->VSDrawString(frame, vsapi, 0, 8, 0, 0, msg1, 0);
		sprintf_s(msg1, "GOP Number:    %u (%u)  GOP Pos = %I64d", gop, rgop.number, rgop.position);
		d->VSDrawString(frame, vsapi, 0, 9, 0, 0, msg1, 0);
		sprintf_s(msg1, "Closed GOP:    %s", rgop.closed ? "True" : "False");
		d->VSDrawString(frame, vsapi, 0, 10, 0, 0, msg1, 0);
		sprintf_s(msg1, "Display Frame: %d", n);
		d->VSDrawString(frame, vsapi, 0, 11, 0, 0, msg1, 0);
		sprintf_s(msg1, "Encoded Frame: %d (top) %d (bottom)", fn.top, fn.bottom);
		d->VSDrawString(frame, vsapi, 0, 12, 0, 0, msg1, 0);
		sprintf_s(msg1, "Frame Type:    %c (%d)", pct, raw);
		d->VSDrawString(frame, vsapi, 0, 13, 0, 0, msg1, 0);
		sprintf_s(msg1, "Progr Frame:   %s", fraw.pf ? "True" : "False");
		d->VSDrawString(frame, vsapi, 0, 14, 0, 0, msg1, 0);
		sprintf_s(msg1, "Colorimetry:   %s (%d)", matrix[rgop.matrix], rgop.matrix);
		d->VSDrawString(frame, vsapi, 0, 15, 0, 0, msg1, 0);
		sprintf_s(msg1, "Quants:        %d/%d/%d (avg/min/max)", d->avgquant, d->minquant, d->maxquant);
		d->VSDrawString(frame, vsapi, 0, 16, 0, 0, msg1, 0);
	}
	else if (d->info == 2)
	{
		dprintf(const_cast<char*>("DGDecode: DGDecode %s (c) 2005-2021 Donald A. Graft (et al)\n"), VERSION);
		dprintf(const_cast<char*>("DGDecode: Source:            %s\n"), d->Infilename[rgop.file].c_str());
		dprintf(const_cast<char*>("DGDecode: Frame Rate:        %3.6f fps (%u/%u) %s\n"),
			double(d->VF_FrameRate_Num) / double(d->VF_FrameRate_Den),
			d->VF_FrameRate_Num, d->VF_FrameRate_Den,
			(d->VF_FrameRate == 25000 || d->VF_FrameRate == 50000) ? "(PAL)" : d->VF_FrameRate == 29970 ? "(NTSC)" : "");
		dprintf(const_cast<char*>("DGDecode: Field Order:       %s\n"), d->Field_Order == 1 ? "Top Field First" : "Bottom Field First");
		dprintf(const_cast<char*>("DGDecode: Picture Size:      %d x %d\n"), d->getLumaWidth(), d->getLumaHeight());
		dprintf(const_cast<char*>("DGDecode: Aspect Ratio:      %s\n"), d->Aspect_Ratio);
		dprintf(const_cast<char*>("DGDecode: Progressive Seq:   %s\n"), rgop.progressive ? "True" : "False");
		dprintf(const_cast<char*>("DGDecode: GOP Number:        %d (%d)  GOP Pos = %I64d\n"), gop, rgop.number, rgop.position);
		dprintf(const_cast<char*>("DGDecode: Closed GOP:        %s\n"), rgop.closed ? "True" : "False");
		dprintf(const_cast<char*>("DGDecode: Display Frame:     %d\n"), n);
		dprintf(const_cast<char*>("DGDecode: Encoded Frame:     %d (top) %d (bottom)\n"), fn.top, fn.bottom);
		dprintf(const_cast<char*>("DGDecode: Frame Type:        %c (%d)\n"), pct, raw);
		dprintf(const_cast<char*>("DGDecode: Progressive Frame: %s\n"), fraw.pf ? "True" : "False");
		dprintf(const_cast<char*>("DGDecode: Colorimetry:       %s (%d)\n"), matrix[rgop.matrix], rgop.matrix);
		dprintf(const_cast<char*>("DGDecode: Quants:            %d/%d/%d (avg/min/max)\n"), d->avgquant, d->minquant, d->maxquant);
	}
	else if (d->info == 3)
	{
		extern bool PutHintingData(uint8_t * video, uint32_t hint);
		constexpr uint32_t PROGRESSIVE = 0x00000001;
		constexpr int COLORIMETRY_SHIFT = 2;

		uint32_t hint = 0;
		if (fraw.pf == 1) hint |= PROGRESSIVE;
		hint |= ((rgop.matrix & 7) << COLORIMETRY_SHIFT);
		PutHintingData(video, hint);
	}
}

VSVideoSource::VSVideoSource(const char *d2v, int64_t idct, int64_t showQ, int64_t info,
	const VSAPI* vsapi, VSCore* core)
{
	if (idct > 7)
		throw_error("MPEG2Source: iDCT invalid (1:MMX,2:SSEMMX,3:SSE2,4:FPU,5:REF,6:Skal's,7:Simple)");

	FILE* f;
	fopen_s(&f, d2v, "r");
	if (f == nullptr)
		throw_error("MPEG2Source: unable to load D2V file");

	try {
			m_decoder = new CMPEG2Decoder(f, d2v, idct, info, showQ);
	}
	catch (std::runtime_error& e) {
		if (f) fclose(f);
		throw_error("MPEG2Source: could not create the decoder");
	}

	VI[0].numFrames = m_decoder->FrameList.size();
	VI[0].width = m_decoder->Clip_Width;
	VI[0].height = m_decoder->Clip_Height;

	int chroma_format = m_decoder->getChromaFormat();

	if (chroma_format == 2) vsapi ? vsapi->registerFormat(cmYUV, stInteger, 8, 1, 0, core) : 0;
	else
		VI[0].format = vsapi ? vsapi->registerFormat(cmYUV, stInteger, 8, 1, 1, core) : 0;
	VI[0].fpsNum = m_decoder->VF_FrameRate_Num;
	VI[0].fpsDen = m_decoder->VF_FrameRate_Den;
	VI[0].flags = 0;
	muldivRational(&VI[0].fpsDen, &VI[0].fpsNum, 1, 1);

	luminanceFlag = (m_decoder->lumGamma != 0 || m_decoder->lumOffset != 0);
	if (luminanceFlag) {
		int lg = m_decoder->lumGamma;
		int lo = m_decoder->lumOffset;
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
}

VSVideoSource::~VSVideoSource()
{
}

const VSFrameRef *VS_CC VSVideoSource::GetFrame(int n, int activationReason, void **instanceData, void **, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
	int x, x2, y;
	unsigned int d, skip;
	VSVideoSource *vs = static_cast<VSVideoSource *>(*instanceData);
	int leading;
	unsigned char *p, *i, *j;
	char buf[80];
	int gop;
	unsigned char c = 0;
	char *t = NULL;
	extern void luminance_filter(uint8_t * luma, const int width, const int height,
		const int pitch, const uint8_t * table);

	if (activationReason != arInitial)
		return nullptr;

	VSFrameRef *Dst = vsapi->newVideoFrame(vs->VI[0].format, vs->VI[0].width, vs->VI[0].height, nullptr, core);
	VSMap* Props = vsapi->getFramePropsRW(Dst);

	YV12PICT out = YV12PICT(vsapi, Dst);

	vs->m_decoder->Decode(n, out);

	if (vs->luminanceFlag)
		luminance_filter(out.y, out.ywidth, out.yheight, out.ypitch, vs->luminanceTable);

	if (vs->m_decoder->info != 0)
		VSshow_info(n, vs->m_decoder, vsapi, Dst, &vs->VI[0], (unsigned char *)out.y);

	// Fill in Vapoursynth properties for ClipInfo().
	char ctype;
	switch (vs->m_decoder->FrameList[n].pct)
	{
	case P_TYPE:
		ctype = 'P';
		break;
	case B_TYPE:
		ctype = 'B';
		break;
	case I_TYPE:
		ctype = 'I';
		break;
	default:
		ctype = 'U';
		break;
	}
	vsapi->propSetData(Props, "_PictType", &ctype, 1, paReplace);
	// These are reversed to convert rate to duration.
	vsapi->propSetInt(Props, "_DurationNum", (int64_t)vs->m_decoder->VF_FrameRate_Den, paReplace);
	vsapi->propSetInt(Props, "_DurationDen", (int64_t)vs->m_decoder->VF_FrameRate_Num, paReplace);
	if (vs->m_decoder->Field_Order != -1)
	{
		int order;
		if (vs->m_decoder->Field_Order == 1)
			order = 2;
		else if (vs->m_decoder->Field_Order == 2)
			order = 1;
		else
			order = 0;
		vsapi->propSetInt(Props, "_FieldBased", (int64_t)order, paReplace);
	}
	vsapi->propSetInt(Props, "_Matrix", (int64_t)vs->m_decoder->matrix_coefficients, paReplace);
	vsapi->propSetInt(Props, "_Primaries", (int64_t)vs->m_decoder->colour_primaries, paReplace);
	vsapi->propSetInt(Props, "_Transfer", (int64_t)vs->m_decoder->transfer_characteristics, paReplace);
	
	unsigned int sar_num, sar_den;
	(void)sscanf(vs->m_decoder->Aspect_Ratio, "%d:%d", &sar_num, &sar_den);
	vsapi->propSetInt(Props, "_SARNum", (int64_t)sar_num, paReplace);
	vsapi->propSetInt(Props, "_SARDen", (int64_t)sar_den, paReplace);

	return Dst;
}

void VS_CC VSVideoSource::Init(VSMap *, VSMap *, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
	VSVideoSource *Source = static_cast<VSVideoSource *>(*instanceData);
	vsapi->setVideoInfo(Source->VI, 1, node);
}

void VS_CC VSVideoSource::Free(void *instanceData, VSCore *, const VSAPI *)
{
	delete static_cast<VSVideoSource *>(instanceData);
}

static void VS_CC CreateDGDecode(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* vsapi)
{
	int err;

	const char* d2v = vsapi->propGetData(in, "d2v", 0, nullptr);
	int64_t idct = vsapi->propGetInt(in, "idct", 0, &err);
	if (err)
		idct = 0;
	int64_t showQ = vsapi->propGetInt(in, "showQ", 0, &err);
	if (err)
		showQ = 0;
	int64_t info = vsapi->propGetInt(in, "info", 0, &err);
	if (err)
		info = 0;

	VSVideoSource* vs;
	try {
		vs = new VSVideoSource(d2v, idct, showQ, info, vsapi, core);
	}
	catch (std::exception const& e) {
		return vsapi->setError(out, e.what());
	}

	vsapi->createFilter(in, out, "DGDecode", VSVideoSource::Init, VSVideoSource::GetFrame, VSVideoSource::Free, fmUnordered, nfMakeLinear, vs, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc,
	VSPlugin* plugin)
{
	configFunc("com.vapoursynth.dgdecode", "dgdecode", "DGDecode for VapourSynth", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("MPEG2Source", "d2v:data;idct:int:opt;showQ:int:opt;info:int:opt;", CreateDGDecode, nullptr, plugin);
}