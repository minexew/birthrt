REM MAKEFINL.BAT, batch file for making FINALE.RES
del ..\resfiles\finale.res
resutil ..\resfiles\finale.res @finale.rsp
