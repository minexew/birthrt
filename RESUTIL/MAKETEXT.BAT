REM MAKETEXT.BAT, batch file for making TEXTURE.RES
del ..\resfiles\textures.res
resutil ..\resfiles\textures @textures.rsp
