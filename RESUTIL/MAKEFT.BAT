REM MAKEFT.BAT, batch file for making medftres.RES
del ..\resfiles\medftres.res
resutil ..\resfiles\medftres.res @medftres.rsp
