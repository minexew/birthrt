REM MAKEUI.BAT, batch file for making UI.RES
del ..\resfiles\ui.res
resutil ..\resfiles\ui.res @ui.rsp
