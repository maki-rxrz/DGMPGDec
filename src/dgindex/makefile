# UserChange

TARGETNAME=DGIndex
OUTDIR=.\Release_nmake

AS=nasm.exe
ASFLAGS=-f win32 -DPREFIX -DWIN32

LINK32=link.exe


.SUFFIXES: .cpp .obj
.SUFFIXES: .rc .res



ALL : "$(OUTDIR)/$(TARGETNAME).exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# compile

CPPFLAGS = /c\
    /Zi /nologo /W3 /WX- /O2 /Ob1 /Oi /Ot /Oy-\
    /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CRT_SECURE_NO_WARNINGS" /D "_MBCS"\
    /GF /Gm- /EHsc /MT /GS /fp:precise /Zc:wchar_t /Zc:forScope\
    /Fp"$(OUTDIR)/$(TARGETNAME).pch" /Fo"$(OUTDIR)/" /Fd"$(OUTDIR)/"\
    /Gd
#    /Fp"$(OUTDIR)/$(TARGETNAME).pch" /Fa"$(OUTDIR)/" /Fo"$(OUTDIR)/" /Fd"$(OUTDIR)/"\
#    /Gd /analyze- /errorReport:queue

# link

LINK32_FLAGS =\
    /OUT:"$(OUTDIR)/$(TARGETNAME).exe"\
    /INCREMENTAL:NO\
    /NOLOGO\
    /MANIFEST\
    /ManifestFile:"$(OUTDIR)/$(TARGETNAME).exe.intermediate.manifest"\
    /ALLOWISOLATION\
    /MANIFESTUAC:"level='asInvoker' uiAccess='false'"\
    /SUBSYSTEM:WINDOWS\
    /STACK:"4096000"\
    /TLBID:1\
    /DYNAMICBASE\
    /NXCOMPAT\
    /MACHINE:X86\
    /PDB:"$(OUTDIR)/$(TARGETNAME).pdb"\
    /ERRORREPORT:QUEUE\
        "vfw32.lib"\
        "winmm.lib"\
        "odbc32.lib"\
        "odbccp32.lib"\
        "shlwapi.lib"\
        "kernel32.lib"\
        "user32.lib"\
        "gdi32.lib"\
        "winspool.lib"\
        "comdlg32.lib"\
        "advapi32.lib"\
        "shell32.lib"\
        "ole32.lib"\
        "oleaut32.lib"\
        "uuid.lib"

#    /PGD:"$(OUTDIR)/$(TARGETNAME).pgd"\




LINK32_OBJS=\
    "d2vparse.obj"\
    "getbit.obj"\
    "gethdr.obj"\
    "getpic.obj"\
    "gui.obj"\
    "idctfpu.obj"\
    "idctref.obj"\
    "initial_parse.obj"\
    "misc.obj"\
    "motion.obj"\
    "mpeg2dec.obj"\
    "norm.obj"\
    "parse_cli.obj"\
    "pat.obj"\
    "store.obj"\
    "strverscmp.obj"\
    "wavefs44.obj"\
    "AC3Dec\bit_allocate.obj"\
    "AC3Dec\bitstream.obj"\
    "AC3Dec\coeff.obj"\
    "AC3Dec\crc.obj"\
    "AC3Dec\decode.obj"\
    "AC3Dec\downmix.obj"\
    "AC3Dec\exponent.obj"\
    "AC3Dec\imdct.obj"\
    "AC3Dec\parse.obj"\
    "AC3Dec\rematrix.obj"\
    "AC3Dec\sanity_check.obj"\
    "gui.res"

# resource compiler

#RFLAGS=/D "NDEBUG" /l 0x0409 /nologo /fo"$(OUTDIR)/"
RFLAGS=/D "NDEBUG" /l 0x0409 /nologo /fo"$(OUTDIR)\gui.res"



"$(OUTDIR)/$(TARGETNAME).exe" : "$(OUTDIR)" custom_step $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) "$(OUTDIR)\*.obj" "$(OUTDIR)\*.res"


custom_step:
    $(AS) $(ASFLAGS) -o "$(OUTDIR)\simple_idct_mmx.obj" "simple_idct_mmx.asm"
    $(AS) $(ASFLAGS) -o "$(OUTDIR)\skl_dct_sse.obj" "skl_dct_sse.asm"
    @echo copy "idctmmx.obj" "$(OUTDIR)"
    @copy /y "idctmmx.obj" "$(OUTDIR)" >NUL


.cpp{$(OUTDIR)}.obj:
    $(CPP) $(CPPFLAGS) $<

.rc{$(OUTDIR)}.res:
    $(RC) $(RFLAGS) $<


clean:
    del /Q "$(OUTDIR)\*.*"
#    del /Q "$(OUTDIR)\*.obj" "$(OUTDIR)\*.res" "$(OUTDIR)\*.manifest" "$(OUTDIR)\*.pdb"
