@echo off

rem Set up NASMPATH (Path termination must be a delimiter.)
rem set NASMPATH=<YouNasmPath>\

set EXTRA_CPPFLAGS=/FS

cd .\src\dgindex

call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" x86

jom /version >NUL 2>&1

if "%ERRORLEVEL%" == "0" (
	jom
) else (
	nmake /nologo
)

echo.
pause
