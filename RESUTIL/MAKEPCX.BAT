REM MAKEPCX.BAT, batch file for making thingpcx.rsp
del ..\resfiles\thingpcx.res
resutil ..\resfiles\thingpcx.res @thingpcx.rsp
