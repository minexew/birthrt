@echo off

rem check for 4dos
if "%@eval[2+2]" == "4" goto 4Dos

if "%1"=="WIN95" goto MakeWin1
if "%1"=="win95" goto MakeWin1
if "%1"=="DOS" goto MakeDOS1
if "%1"=="dos" goto MakeDOS1

choice /C:12 /N Press 1 for DOS or 2 for Win95 : 

if errorlevel 2 goto MakeWin1
if errorlevel 1 goto MakeDOS1

echo Request fuzzy, Try again later.

goto Done

:MakeDOS1
	echo Building DEBUG NOVA.EXE for DOS
        wmake OS=DOS DEBUG=ON SOUND=ON %2 > error.log
        grep Error error.log
	goto Done

:MakeWin1
	echo Building DEBUG NOVA95.EXE for WIN95
        wmake OS=WIN95 DEBUG=ON SOUND=ON SIMUL=ON %2 > error.log
        grep Error error.log
	goto Done

:4Dos

if %1 == WIN95 goto MakeWin
if %1 == win95 goto MakeWin
if %1 == DOS goto MakeDOS
if %1 == dos goto MakeDOS

input "Press 1 for DOS or 2 for Win95 : " %%os

if %os == 1 goto MakeDOS
if %os == 2 goto MakeWin

echo Request fuzzy, Try again later.

goto Done

:MakeDOS
	echo Building DEBUG NOVA.EXE for DOS
	alias doit=dmake DOS
	wmake OS=DOS DEBUG=ON SOUND=ON %2 > error.log
	grep Error error.log
	goto Done

:MakeWin	
	echo Building DEBUG NOVA95.EXE for WIN95
	alias doit=dmake WIN95
	wmake OS=WIN95 DEBUG=ON SOUND=ON SIMUL=ON %2 > error.log
	grep Error error.log
	
:Done	
