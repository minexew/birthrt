copy makeinit.rel makeinit
del *.obj
del *.exe
wmake
rename skel.exe testrel.exe
wlib -n -c -p=64 solaudw.lib @libfiles.lbc
copy makeinit.dbg makeinit
del *.obj
wmake
rename skel.exe testdbg.exe
wlib -n -c -p=64 solaudwd.lib @libfiles.lbc

copy *.lib ..\..\lib\win\w
