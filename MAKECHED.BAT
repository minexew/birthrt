@echo off

REM MAKECHED.BAT, Batch file to build the Birthright Character Editor
REM 8-22-97

TEXT
Enter Selection:
  1 Character Editor (debug)
  2 Character Editor (rel)
ENDTEXT

inkey /k"12[Esc]" %%mk1

iff %@ascii[%mk1] == 27 then
   quit

iff %mk1 == 1 then
   wmake /f chared.mak /e OS=WIN95 DEBUG=ON SOUND=OFF EDIT=OFF JPC=OFF RELEASE=OFF DEMO=OFF NETDEMO=OFF CRCCHECK=OFF CHRED=ON STATUS=OFF > makeched.err
elseiff %mk1 == 2 then
   wmake /f chared.mak /e OS=WIN95 DEBUG=OFF SOUND=OFF EDIT=OFF JPC=OFF RELEASE=ON DEMO=OFF NETDEMO=OFF CRCCHECK=OFF CHRED=ON STATUS=OFF > makeched.err
endiff

copy /p strdat.dat chared.dat
