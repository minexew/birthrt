wmake OS=DOS DEBUG=OFF SOUND=ON RELEASE=ON > errord.log
copy nova.exe novar.exe /Y
REM wmake OS=WIN95 DEBUG=ON SOUND=ON RAND=ON > errorm.log
REM copy nova95.exe nova95m.exe /Y
REM del nova95.exe
wmake OS=WIN95 DEBUG=OFF SOUND=ON RELEASE=ON > errorw.log
copy nova95.exe nova95r.exe /Y
copy nova95.exe winbr.exe /Y
