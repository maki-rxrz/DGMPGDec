@echo off
echo.
set PROJECT=DGDecode
NMAKE /F %PROJECT%.mak /NOLOGO
if exist Release\*.lib del Release\*.lib
if exist Release\*.exp del Release\*.exp
if exist Release\*.pdb del Release\*.pdb
