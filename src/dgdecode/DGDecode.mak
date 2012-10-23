CFG=DGDecode - Win32 Release

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=.\Release
INTDIR=.\Intermediate
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\DGDecode.dll"


CLEAN :
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\AVISynthAPI.obj"
	-@erase "$(INTDIR)\getbit.obj"
	-@erase "$(INTDIR)\gethdr.obj"
	-@erase "$(INTDIR)\getpic.obj"
	-@erase "$(INTDIR)\global.obj"
	-@erase "$(INTDIR)\idctfpu.obj"
	-@erase "$(INTDIR)\idctref.obj"
	-@erase "$(INTDIR)\mc.obj"
	-@erase "$(INTDIR)\mcmmx.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\MPEG2DEC.obj"
	-@erase "$(INTDIR)\PostProcess.obj"
	-@erase "$(INTDIR)\gui.res"
	-@erase "$(INTDIR)\store.obj"
	-@erase "$(INTDIR)\text-overlay.obj"
	-@erase "$(INTDIR)\Utilities.obj"
	-@erase "$(INTDIR)\vfapidec.obj"
	-@erase "$(OUTDIR)\DGDecode.dll"
	-@erase "$(OUTDIR)\DGDecode.exp"
	-@erase "$(OUTDIR)\DGDecode.lib"

"$(OUTDIR)" :
    @if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    @if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=@icl.exe
CPP_PROJ=/nologo /MT /W0 /GX /G6 /O2 /Qipo /QxK /Qunroll-aggressive /Qprof-dir profiling /Qprof-use /D "WIN32" /D "NDEBUG" /D WINVER=0x0501 /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Fo"$(INTDIR)\\" /D /"DGDecode_EXPORTS" /c

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /D WINVER=0x0501 /mktyplib203 /win32 
RSC=@rc.exe
RSC_PROJ=/l 0x407 /fo"$(INTDIR)\gui.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DGDecode.bsc" 
BSC32_SBRS= \
	
LINK32=@xilink.exe
LINK32_FLAGS=winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /dll /pdb:none /machine:I386 /out:"$(OUTDIR)\DGDecode.dll" /implib:"$(OUTDIR)\DGDecode.lib" /OPT:NOWIN98 /ignore:4089 
LINK32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\getbit.obj" \
	"$(INTDIR)\gethdr.obj" \
	"$(INTDIR)\getpic.obj" \
	"$(INTDIR)\global.obj" \
	"$(INTDIR)\idctfpu.obj" \
	"$(INTDIR)\idctref.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\MPEG2DEC.obj" \
	"$(INTDIR)\store.obj" \
	"$(INTDIR)\text-overlay.obj" \
	"$(INTDIR)\Utilities.obj" \
	"$(INTDIR)\vfapidec.obj" \
	"$(INTDIR)\AVISynthAPI.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\PostProcess.obj" \
	"$(INTDIR)\mc.obj" \
	"$(INTDIR)\mcmmx.obj" \
	"$(INTDIR)\mcsse.obj" \
	"$(INTDIR)\gui.res" \
	".\idctmmx.obj" \
	"$(INTDIR)\simple_idct_mmx.obj" \
	"$(INTDIR)\skl_dct_sse.obj"

"$(OUTDIR)\DGDecode.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!IF "$(CFG)" == "DGDecode - Win32 Release"
SOURCE=.\alloc.cpp

"$(INTDIR)\alloc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getbit.cpp

"$(INTDIR)\getbit.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gethdr.cpp

"$(INTDIR)\gethdr.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getpic.cpp

"$(INTDIR)\getpic.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\global.cpp

"$(INTDIR)\global.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gui.rc

"$(INTDIR)\gui.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)

SOURCE=.\idctfpu.cpp

"$(INTDIR)\idctfpu.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\idctref.cpp

"$(INTDIR)\idctref.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\motion.cpp

"$(INTDIR)\motion.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MPEG2DEC.cpp

"$(INTDIR)\MPEG2DEC.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\store.cpp

"$(INTDIR)\store.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\text-overlay.cpp"

"$(INTDIR)\text-overlay.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Utilities.cpp

"$(INTDIR)\Utilities.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vfapidec.cpp

"$(INTDIR)\vfapidec.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\AVISynthAPI.cpp

"$(INTDIR)\AVISynthAPI.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\misc.cpp

"$(INTDIR)\misc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PostProcess.cpp

"$(INTDIR)\PostProcess.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mc.cpp

"$(INTDIR)\mc.obj" : $(SOURCE) "$(INTDIR)"

SOURCE=.\mcmmx.cpp

"$(INTDIR)\mcmmx.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mcsse.asm
IntDir=.\Intermediate
InputPath=.\mcsse.asm
InputName=mcsse

"$(INTDIR)\mcsse.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	@nasm -w-orphan-labels -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)
	
SOURCE=.\idctmmx.asm
InputPath=.\idctmmx.asm
InputName=idctmmx

".\idctmmx.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	@ml /c /coff /Cx /nologo $(InputName).asm
	
SOURCE=.\simple_idct_mmx.asm
IntDir=.\Intermediate
InputPath=.\simple_idct_mmx.asm
InputName=simple_idct_mmx

"$(INTDIR)\simple_idct_mmx.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	@nasm -w-orphan-labels -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)
	
SOURCE=.\skl_dct_sse.asm
IntDir=.\Intermediate
InputPath=.\skl_dct_sse.asm
InputName=skl_dct_sse

"$(INTDIR)\skl_dct_sse.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	@nasm -w-orphan-labels -f win32 -DPREFIX -DWIN32 -o $(IntDir)\$(InputName).obj $(InputPath)
	

!ENDIF 

