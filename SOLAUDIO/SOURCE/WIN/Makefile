include = .;c:\watcom\h;c:\watcom\h\nt;c:\dxsdk5\sdk\inc

target	= skel
debugOpt = 
bannerOpt= -Q

objects =  &
	skel.obj &
	audiodir.obj &
	custmsys.obj &
	dsound.obj &
	intrptw.obj &
	kernaud.obj &
	audioa.obj


$(target).exe : $(debugObjs) $(objects)
        *wlink name $(target) option stack=40k option map &
    option osname='Windows NT windowed' &
    libpath %WATCOM%\lib386 &
    libpath %WATCOM%\lib386\nt &
    format windows nt ^ &
                debug all file {$(objects)} 

    wrc /bt=nt $(target).rc

