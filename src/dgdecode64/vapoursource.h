#include <windows.h>
#include "VapourSynth.h"
#include <stdint.h>
#include "MPEG2Decoder.h"

class VSVideoSource
{
private:
	VSVideoInfo VI[1];
	uint8_t* bufY, * bufU, * bufV; // for 4:2:2 input support
	bool luminanceFlag;
	uint8_t luminanceTable[256];
	friend class CMPEG2Decoder;
public:
	VSVideoSource(const char *d2v, int64_t idct, int64_t showQ, int64_t info, const VSAPI* vsapi, VSCore* core);
	~VSVideoSource();

	static void VS_CC Init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi);
	static const VSFrameRef *VS_CC GetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi);
	static void VS_CC Free(void *instanceData, VSCore *core, const VSAPI *vsapi);
	CMPEG2Decoder *m_decoder;
	DWORD threadId;
	static const unsigned short vsfont[500][20];
	static void VSDrawDigit(VSFrameRef *Dst, const VSAPI *vsapi, int x, int y, int offsetX, int offsetY, int num, bool full);
	static void VSDrawString(VSFrameRef *Dst, const VSAPI *vsapi, int x, int y, int offsetX, int offsetY, const char *s, bool full);
};
