REM MAKEWAVE.BAT, batch file for making WAVE.RES
del ..\resfiles\wave.res
resutil ..\resfiles\wave.res s 65535 hi @wave.rsp
