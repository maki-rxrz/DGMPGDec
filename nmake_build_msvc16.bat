@echo off

setlocal

set VS_NUMBER=2019
set VS_EDITION=Community

set VSINSTPATH=%ProgramFiles(x86)%\Microsoft Visual Studio\%VS_NUMBER%\%VS_EDITION%

rem Check installed version of Visual Studio (15.2 or later)
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
    for /f "delims=" %%a in ('"%VSWHERE%" -products * -property installationPath') do set VSINSTPATH=%%a
)

rem Set up NASMPATH (Path termination must be a separator character.)
rem set NASMPATH=<YouNasmPath>\

rem Windows XP support
rem set WINXPISDEAD=1

set EXTRA_CPPFLAGS=/FS

cd .\src\dgindex

call "%VSINSTPATH%\VC\Auxiliary\Build\vcvarsall.bat" x86

jom /version >NUL 2>&1

if "%ERRORLEVEL%" == "0" (
	jom
) else (
	nmake /nologo
)

echo.

endlocal

pause
