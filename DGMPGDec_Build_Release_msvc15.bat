@echo off

setlocal

echo -----------------------------------------------------
echo                   S T A R T
echo -----------------------------------------------------

set VisualStudioVersion=15.0
set TOOLS_VER=%VisualStudioVersion%
set MSVC_VER=msvc15
set VS_NUMBER=2017
set VS_EDITION=Community

set VSINSTPATH=%ProgramFiles(x86)%\Microsoft Visual Studio\%VS_NUMBER%\%VS_EDITION%

rem Check installed version of Visual Studio (15.2 or later)
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
    for /f "delims=. tokens=1,2" %%a in ('"%VSWHERE%" -products * -property installationVersion') do set VisualStudioVersion=%%a.%%b
    for /f "delims=. tokens=1,2" %%a in ('"%VSWHERE%" -products * -property installationVersion') do set TOOLS_VER=%%a.%%b
    for /f "delims=. tokens=1"   %%a in ('"%VSWHERE%" -products * -property installationVersion') do set MSVC_VER=msvc%%a
    for /f "delims="             %%a in ('"%VSWHERE%" -products * -property installationPath'   ) do set VSINSTPATH=%%a
)

rem Set up for MSBuild
rem [memo] 4.0: %SystemRoot%\Microsoft.NET\Framework\v4.0.30319
rem  12.0-14.0: %ProgramFiles(x86)%\MSBuild\%TOOLS_VER%\Bin
set MSBUILD_PATH=%VSINSTPATH%\MSBuild\%TOOLS_VER%\Bin
set MSBUILD_EXEC=%MSBUILD_PATH%\MSBuild.exe

rem [memo] verbosity-level: q[uiet] m[inimal] n[ormal] d[etailed] diag[nostic]
set BUILD_OPT=/nologo /maxcpucount:1 /toolsversion:%TOOLS_VER% /verbosity:normal /fl
set FLOG_VERBOSITY=normal

rem List up the target project
set TGT_PROJECT=DGMPGDec
set TGT_SLN=.\msvc\%TGT_PROJECT%.sln
set TGT_PLATFORM=Win32
set TGT_CONFIG=Release

rem Set up NASMPATH (Path termination must be a separator character.)
rem set NASMPATH=<YourNasmPath>\

rem Windows XP support
rem set WINXPISDEAD=1

rem Build
for %%c in (%TGT_CONFIG%  ) do (
for %%p in (%TGT_PLATFORM%) do (
    echo -Build-  %%c^|%%p
    "%MSBUILD_EXEC%" %TGT_SLN% /p:Configuration=%%c;Platform=%%p %BUILD_OPT% /flp:logfile=_build_%TGT_PROJECT%_%%c_%%p_%MSVC_VER%.log;verbosity=%FLOG_VERBOSITY%
))

echo -----------------------------------------------------
echo                   F I N I S H
echo -----------------------------------------------------

endlocal
