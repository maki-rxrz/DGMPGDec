@echo off

rem Set up NASMPATH (Path termination must be a separator character.)
rem set NASMPATH=<YouNasmPath>\

cd .\src\dgindex

call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x86 /xp /Release

jom /version >NUL 2>&1

if "%ERRORLEVEL%" == "0" (
	jom
) else (
	nmake /nologo
)

echo.
pause
