@echo off
set QDIR=\games\quake2
copy /b *.exe %QDIR%\*.* >NUL
if exist *.dll copy /b *.dll %QDIR%\*.* >NUL
copy /b ..\4.XX_Changes.txt %QDIR%\*.* >NUL
cd %QDIR%
