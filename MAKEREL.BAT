@echo off
echo Cleaning out old files
call clean DOS
call clean WIN95

echo Building a Release DOS copy of Birthright
call make DOS RELEASE=ON
call make WIN95 RELEASE=ON

echo Building the backup zip
