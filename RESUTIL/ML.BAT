REM ML.BAT, suitable for an old version of MASM.
REM Produces non-debug version.
masm /c %1;
copy %@name[%1].obj dosobjsr
