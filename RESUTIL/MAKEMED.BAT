REM MAKEMED.BAT, batch file for making medres.RES
del ..\resfiles\medres.res
resutil ..\resfiles\medres.res @medres.rsp
