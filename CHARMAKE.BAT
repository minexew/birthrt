echo Building RELEASE CHARED.EXE for WIN95
wmake OS=WIN95 DEBUG=OFF RELEASE=ON CHARED=ON chared.exe > error.log
grep Error error.log
