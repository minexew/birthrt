REM MLD.BAT, suitable for an old version of MASM.
REM Produces debug version.
REM masm /Gz /c /Cp /Zm /Zp4 /DDEBUG %1;
masm /c /DDEBUG /Zi %1;
copy %@name[%1].obj dosobjsd
