REM MAKEHI.BAT, batch file for making highres.RES
del ..\resfiles\highres.res
resutil ..\resfiles\highres.res @highres.rsp
