DGDecode
--------

This is DGDecode.dll for Vapoursynth/AviSynth+ and 64-bit operation.
The single DLL supports both Vapoursynth and Avisynth operation.

Usage
-----

DGDecode(string "d2v", int "idct", bool "showQ", int "info")
  
Vapoursynth script
------------------
  
import vapoursynth as vs
core = vs.get_core()
core.std.LoadPlugin(".../DGDecode.dll")
video = core.dgdecode.MPEG2Source(r"cut.d2v")
video.set_output()

Parameters
----------

- d2v
    The path of the dv2 file.
    
- idct
    The iDCT algorithm to use.
    0: Follow the d2v specification.
    1,2,3,6,7: AP922 integer (same as SSE2/MMX).
    4: SSE2/AVX2 LLM (single precision floating point, SSE2/AVX2 determination is automatic).
    5: IEEE 1180 reference (double precision floating point).
    Default: -1.
    
- showQ
    It displays macroblock quantizers.
    Default: False.
    
- info
    It prints debug information.
    0: Do not display.
    1: Overlay on video frame.
    2: Output with OutputDebugString(). (The contents are confirmed by DebugView.exe).
    3: Embed hints in 64 bytes of the frame upper left corner.
    Default: 0.

