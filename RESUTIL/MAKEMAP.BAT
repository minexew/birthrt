REM MAKEMAP.BAT, batch file for making MAP.RES
del ..\resfiles\map.res
resutil ..\resfiles\map.res @map.rsp
