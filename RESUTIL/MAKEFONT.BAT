REM MAKEFONT.BAT, batch file for making FONT.RES
del ..\resfiles\font.res
resutil ..\resfiles\font.res @font.rsp
