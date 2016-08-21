@echo off

rem Set up NASMPATH (Path termination must be a separator character.)
rem set NASMPATH=<YouNasmPath>\

cd .\src\dgindex

call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" x86

jom /version >NUL 2>&1

if "%ERRORLEVEL%" == "0" (
	jom
) else (
	nmake /nologo
)

echo.
pause
