@echo off

if %1 == WIN95 goto MakeWin
if %1 == DOS goto MakeDOS

input "Press 1 for DOS or 2 for Win95 : " %%os

if %os == 1 goto MakeDOS
if %os == 2 goto MakeWin

echo Request fuzzy, Try again later.

goto Done

:MakeDOS
	echo Cleaning RELEASE for DOS
	wmake OS=DOS DEBUG=OFF clean
	echo Cleaning DEBUG for DOS
	wmake OS=DOS DEBUG=ON clean
	goto Done

:MakeWin	
	echo Cleaning RELEASE for WIN95
	wmake OS=WIN95 DEBUG=OFF clean
	echo Cleaning DEBUG for WIN95
	wmake OS=WIN95 DEBUG=ON clean
	
:Done	
