REM MAKEST.BAT, batch file for making medstres.RES
del ..\resfiles\medstres.res
resutil ..\resfiles\medstres.res @medstres.rsp
