#-----------------------------
# makefile for DGIndex
#-----------------------------

### Target ###

OUTDIR=.\Release_nmake
INTDIR=$(OUTDIR)\obj

TARGET=DGIndex
TARGETEXE=$(OUTDIR)\$(TARGET).exe


### build utils ###

MASM = ml.exe
NASM = "$(NASMPATH)"nasm.exe
CC   = cl.exe
CPP  = cl.exe
LD   = link.exe
RC   = rc.exe


### build flags ###

MASM_FLAGS = /c /coff /Cx /nologo

NASM_FLAGS = -f win32 -DPREFIX -DWIN32

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
    /ERRORREPORT:NONE $(EXTRA_LDFLAGS)

LDFLAGS_DEBUG = \
    /DEBUG \
    /PDB:"$(OUTDIR)\$(TARGET).pdb"

RFLAGS = /D "NDEBUG" /l 0x0409 /nologo


### link targets ###

LIBS = vfw32.lib winmm.lib odbc32.lib odbccp32.lib shlwapi.lib kernel32.lib \
    user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
    ole32.lib  oleaut32.lib uuid.lib

OBJS = \
    "$(INTDIR)\gui.res" \
    "$(INTDIR)\masm\idctmmx.obj" \
    "$(INTDIR)\nasm\simple_idct_mmx.obj" \
    "$(INTDIR)\nasm\skl_dct_sse.obj" \
    "$(INTDIR)\d2vparse.obj" \
    "$(INTDIR)\getbit.obj" \
    "$(INTDIR)\gethdr.obj" \
    "$(INTDIR)\getpic.obj" \
    "$(INTDIR)\gui.obj" \
    "$(INTDIR)\idctfpu.obj" \
    "$(INTDIR)\idctref.obj" \
    "$(INTDIR)\initial_parse.obj" \
    "$(INTDIR)\misc.obj" \
    "$(INTDIR)\motion.obj" \
    "$(INTDIR)\mpeg2dec.obj" \
    "$(INTDIR)\norm.obj" \
    "$(INTDIR)\parse_cli.obj" \
    "$(INTDIR)\pat.obj" \
    "$(INTDIR)\store.obj" \
    "$(INTDIR)\strverscmp.obj" \
    "$(INTDIR)\wavefs44.obj" \
    "$(INTDIR)\AC3Dec\bit_allocate.obj" \
    "$(INTDIR)\AC3Dec\bitstream.obj" \
    "$(INTDIR)\AC3Dec\coeff.obj" \
    "$(INTDIR)\AC3Dec\crc.obj" \
    "$(INTDIR)\AC3Dec\decode.obj" \
    "$(INTDIR)\AC3Dec\downmix.obj" \
    "$(INTDIR)\AC3Dec\exponent.obj" \
    "$(INTDIR)\AC3Dec\imdct.obj" \
    "$(INTDIR)\AC3Dec\parse.obj" \
    "$(INTDIR)\AC3Dec\rematrix.obj" \
    "$(INTDIR)\AC3Dec\sanity_check.obj"


### build ###

.SUFFIXES : .exe .obj .asm .cpp .res .rc


ALL: "$(TARGETEXE)"

"$(TARGETEXE)": "$(OUTDIR)" $(OBJS)
    @echo   LD    $(TARGET).exe
    @$(LD) $(LDFLAGS) $(OBJS) $(LIBS) >NUL

.asm{$(INTDIR)\masm}.obj:
    @echo   MASM  $<
    @$(MASM) $(MASM_FLAGS) /Fo"$@" $< >NUL

.asm{$(INTDIR)\nasm}.obj:
    @echo   NASM  $<
    @$(NASM) $(NASM_FLAGS) $< -o $@ >NUL

{AC3Dec}.cpp{$(INTDIR)\AC3Dec}.obj:
    @echo   CXX   $<
    @$(CPP) $(CPPFLAGS) $< /Fo"$@" >NUL

.cpp{$(INTDIR)}.obj:
    @echo   CXX   $<
    @$(CPP) $(CPPFLAGS) $< /Fo"$@" >NUL

.rc{$(INTDIR)}.res:
    @echo   RC    $<
    @$(RC) $(RFLAGS) /fo"$@" $< >NUL

clean:
    @echo delete "*.obj *.res *.pdb *.manifest"
    @if exist "$(INTDIR)" @rmdir /S /Q "$(INTDIR)" >NUL
    @if exist "$(OUTDIR)" @del /Q "$(OUTDIR)\*.obj" "$(OUTDIR)\*.pdb" "$(OUTDIR)\*.manifest" 2>NUL 1>NUL

"$(OUTDIR)": "$(INTDIR)" "$(INTDIR)\AC3Dec" "$(INTDIR)\masm" "$(INTDIR)\nasm"
    @if not exist "$(OUTDIR)" @mkdir "$(OUTDIR)"

"$(INTDIR)":
    @if not exist "$(INTDIR)" @mkdir "$(INTDIR)"

"$(INTDIR)\AC3Dec":
    @if not exist "$(INTDIR)\AC3Dec" @mkdir "$(INTDIR)\AC3Dec"

"$(INTDIR)\masm":
    @if not exist "$(INTDIR)\masm" @mkdir "$(INTDIR)\masm"

"$(INTDIR)\nasm":
    @if not exist "$(INTDIR)\nasm" @mkdir "$(INTDIR)\nasm"


### depend ####

!include "makefile.dep"
