REM MAKELOW.BAT, batch file for making lowres.RES
del ..\resfiles\lowres.res
resutil ..\resfiles\lowres.res @lowres.rsp
