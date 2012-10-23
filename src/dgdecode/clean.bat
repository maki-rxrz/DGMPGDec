@echo off
if exist Release\*.lib del Release\*.lib
if exist Release\*.exp del Release\*.exp
if exist Release\*.pdb del Release\*.pdb
if exist Release\*.dll del Release\*.dll
devclean -confirm
