# Microsoft Developer Studio Project File - Name="DGDecode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DGDecode - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DGDecode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DGDecode.mak" CFG="DGDecode - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DGDecode - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DGDecode - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DGDecode - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DGDecode_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Fr /YX /FD /O3 /D /"DGDecode_EXPORTS" /openmp /Qip /Qunroll /Qparallel /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /dll /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DGDecode - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DGDecode_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /Ob1 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DGDecode_EXPORTS" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "DGDecode - Win32 Release"
# Name "DGDecode - Win32 Debug"
# Begin Group "sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\alloc.cpp
# End Source File
# Begin Source File

SOURCE=.\getbit.cpp
# End Source File
# Begin Source File

SOURCE=.\gethdr.cpp
# End Source File
# Begin Source File

SOURCE=.\getpic.cpp
# End Source File
# Begin Source File

SOURCE=.\global.cpp
# End Source File
# Begin Source File

SOURCE=.\gui.rc
# End Source File
# Begin Source File

SOURCE=.\idctfpu.cpp
# End Source File
# Begin Source File

SOURCE=.\idctref.cpp
# End Source File
# Begin Source File

SOURCE=.\motion.cpp
# End Source File
# Begin Source File

SOURCE=.\MPEG2DEC.cpp
# End Source File
# Begin Source File

SOURCE=.\store.cpp
# End Source File
# Begin Source File

SOURCE=".\text-overlay.cpp"
# End Source File
# Begin Source File

SOURCE=.\Utilities.cpp
# End Source File
# Begin Source File

SOURCE=.\vfapidec.cpp
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\avisynth2.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\skl_nasm.h
# End Source File
# Begin Source File

SOURCE=".\text-overlay.h"
# End Source File
# Begin Source File

SOURCE=.\Utilities.h
# End Source File
# End Group
# Begin Group "DGDecode"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AVISynthAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\AvisynthAPI.h
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\PostProcess.cpp
# End Source File
# Begin Source File

SOURCE=.\PostProcess.h
# End Source File
# End Group
# Begin Group "Motion Compensation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mc.cpp
# End Source File
# Begin Source File

SOURCE=.\mc.h
# End Source File
# Begin Source File

SOURCE=.\mc3dnow.cpp
# End Source File
# Begin Source File

SOURCE=.\mcmmx.cpp
# End Source File
# Begin Source File

SOURCE=.\mcsse.asm

!IF  "$(CFG)" == "DGDecode - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release
InputPath=.\mcsse.asm
InputName=mcsse

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	d:\Dosbin\nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "DGDecode - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug
InputPath=.\mcsse.asm
InputName=mcsse

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	d:\dosbin\nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\idctmmx.asm

!IF  "$(CFG)" == "DGDecode - Win32 Release"

# Begin Custom Build - masm $(InputPath)
InputPath=.\idctmmx.asm
InputName=idctmmx

"$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /coff /Cx /nologo $(InputName).asm

# End Custom Build

!ELSEIF  "$(CFG)" == "DGDecode - Win32 Debug"

# Begin Custom Build
InputPath=.\idctmmx.asm
InputName=idctmmx

"$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /coff /Cx /nologo $(InputName).asm

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simple_idct_mmx.asm

!IF  "$(CFG)" == "DGDecode - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\simple_idct_mmx.asm
InputName=simple_idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	d:\Dosbin\nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "DGDecode - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\simple_idct_mmx.asm
InputName=simple_idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	d:\dosbin\nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\skl_dct_sse.asm

!IF  "$(CFG)" == "DGDecode - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\skl_dct_sse.asm
InputName=skl_dct_sse

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	d:\Dosbin\nasm -f win32 -DPREFIX -DWIN32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "DGDecode - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\skl_dct_sse.asm
InputName=skl_dct_sse

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	d:\dosbin\nasm -f win32 -DPREFIX -DWIN32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
