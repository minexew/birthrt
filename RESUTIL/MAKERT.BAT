REM MAKERT.BAT, batch file for making medrtres.RES
del ..\resfiles\medrtres.res
resutil ..\resfiles\medrtres.res @medrtres.rsp
