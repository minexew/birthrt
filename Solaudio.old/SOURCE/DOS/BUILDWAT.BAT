copy makeinit.rel makeinit
del *.obj
del *.exe
wmake
wlib -n -c -p=64 solaudd.lib @libfiles.lbc
rename skel.exe testrel.exe
copy makeinit.dbg makeinit
del *.obj
wmake
wlib -n -c -p=64 solauddd.lib @libfiles.lbc
rename skel.exe testdbg.exe

copy *.lib ..\..\lib\dos\w
