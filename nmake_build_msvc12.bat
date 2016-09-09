@echo off

setlocal

rem Set up NASMPATH (Path termination must be a separator character.)
rem set NASMPATH=<YouNasmPath>\

set EXTRA_CPPFLAGS=/FS

cd .\src\dgindex

call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" x86

jom /version >NUL 2>&1

if "%ERRORLEVEL%" == "0" (
	jom
) else (
	nmake /nologo
)

echo.

endlocal

pause
