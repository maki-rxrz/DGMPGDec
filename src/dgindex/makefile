#-----------------------------
# makefile for DGIndex
#-----------------------------

### Target ###

OUTDIR=.\Release_nmake

TARGET=DGIndex
TARGETEXE=$(OUTDIR)\$(TARGET).exe


### build utils ###

MASM = "$(MASM_PATH)ml.exe"
AS  = nasm.exe
CC  = cl.exe
CPP = cl.exe
LD  = link.exe
RC  = rc.exe


### build flags ###

ASFLAGS = -f win32 -DPREFIX -DWIN32

CPPFLAGS = \
    /c /Zi /nologo /W3 /WX- /O2 /Ob1 /Oi /Ot /Oy- \
    /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_WARNINGS" /D "_MBCS" \
    /GF /Gm- /EHsc /MT /GS /fp:precise /Zc:wchar_t /Zc:forScope \
    /Fp"$(OUTDIR)/$(TARGET).pch" /Fd"$(OUTDIR)/" \
    /Gd /analyze- /errorReport:none $(EXTRA_CPPFLAGS)

LDFLAGS = \
    /OUT:"$(TARGETEXE)" \
    /INCREMENTAL:NO \
    /NOLOGO \
    /MANIFEST \
    /ManifestFile:"$(TARGETEXE).intermediate.manifest" \
    /ALLOWISOLATION \
    /MANIFESTUAC:"level='asInvoker' uiAccess='false'" \
    /SUBSYSTEM:WINDOWS,5.01 \
    /STACK:"4096000" \
    /TLBID:1 \
    /DYNAMICBASE \
    /NXCOMPAT \
    /MACHINE:X86 \
    /PDB:"$(OUTDIR)/$(TARGET).pdb" \
    /ERRORREPORT:NONE $(EXTRA_LDFLAGS)

RFLAGS = /D "NDEBUG" /l 0x0409 /nologo


### link targets ###

LIBS = vfw32.lib winmm.lib odbc32.lib odbccp32.lib shlwapi.lib kernel32.lib \
    user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
    ole32.lib  oleaut32.lib uuid.lib

OBJS = \
    idctmmx.obj \
    "$(OUTDIR)\gui.res" \
    "$(OUTDIR)\simple_idct_mmx.obj" \
    "$(OUTDIR)\skl_dct_sse.obj" \
    "$(OUTDIR)\d2vparse.obj" \
    "$(OUTDIR)\getbit.obj" \
    "$(OUTDIR)\gethdr.obj" \
    "$(OUTDIR)\getpic.obj" \
    "$(OUTDIR)\gui.obj" \
    "$(OUTDIR)\idctfpu.obj" \
    "$(OUTDIR)\idctref.obj" \
    "$(OUTDIR)\initial_parse.obj" \
    "$(OUTDIR)\misc.obj" \
    "$(OUTDIR)\motion.obj" \
    "$(OUTDIR)\mpeg2dec.obj" \
    "$(OUTDIR)\norm.obj" \
    "$(OUTDIR)\parse_cli.obj" \
    "$(OUTDIR)\pat.obj" \
    "$(OUTDIR)\store.obj" \
    "$(OUTDIR)\strverscmp.obj" \
    "$(OUTDIR)\wavefs44.obj" \
    "$(OUTDIR)\AC3Dec\bit_allocate.obj" \
    "$(OUTDIR)\AC3Dec\bitstream.obj" \
    "$(OUTDIR)\AC3Dec\coeff.obj" \
    "$(OUTDIR)\AC3Dec\crc.obj" \
    "$(OUTDIR)\AC3Dec\decode.obj" \
    "$(OUTDIR)\AC3Dec\downmix.obj" \
    "$(OUTDIR)\AC3Dec\exponent.obj" \
    "$(OUTDIR)\AC3Dec\imdct.obj" \
    "$(OUTDIR)\AC3Dec\parse.obj" \
    "$(OUTDIR)\AC3Dec\rematrix.obj" \
    "$(OUTDIR)\AC3Dec\sanity_check.obj"


### build ###

.SUFFIXES : .exe .obj .asm .cpp .res .rc


ALL: "$(TARGETEXE)"

"$(TARGETEXE)": "$(OUTDIR)" "$(OUTDIR)\AC3Dec" $(OBJS)
    @echo $(LD) $(LDFLAGS) $(OBJS) $(LIBS)
    @$(LD) $(LDFLAGS) $(OBJS) $(LIBS)

idctmmx.obj: idctmmx.asm
    @if not "$(MASM_PATH)" == "" @$(MASM) /c /coff /Cx /nologo /Fo idctmmx.obj idctmmx.asm

.asm{$(OUTDIR)}.obj:
    @echo nasm $<
    @$(AS) $(ASFLAGS) $< -o $@

{AC3Dec\}.cpp{$(OUTDIR)\AC3Dec}.obj:
    @$(CPP) $(CPPFLAGS) $< /Fo"$@"

.cpp{$(OUTDIR)}.obj:
    @$(CPP) $(CPPFLAGS) $< /Fo"$@"

.rc{$(OUTDIR)}.res:
    @echo rc $<
    @$(RC) $(RFLAGS) /fo"$@" $<

clean:
    @echo delete "*.obj *.res *.pdb *.manifest"
    @if exist "$(OUTDIR)\AC3Dec" @rmdir /S /Q "$(OUTDIR)\AC3Dec" >NUL
    @if exist "$(OUTDIR)" @del /Q "$(OUTDIR)\*.res" "$(OUTDIR)\*.obj" "$(OUTDIR)\*.pdb" "$(OUTDIR)\*.manifest" 2>NUL 1>NUL

"$(OUTDIR)":
    @mkdir "$(OUTDIR)"

"$(OUTDIR)\AC3Dec":
    @mkdir "$(OUTDIR)\AC3Dec"


### depend ####

!include "makefile.dep"
