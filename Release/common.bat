@echo off
set QDIR=\games\quake2
copy /b *.exe %QDIR%\*.* >NUL
if exist *.dll copy /b *.dll %QDIR%\*.* >NUL
if exist *.dbg copy /b *.dbg %QDIR%\*.* > NUL
if exist *.pdb copy /b *.pdb %QDIR%\*.* > NUL
copy /b ..\4.XX_Changes.txt %QDIR%\*.* >NUL
cd %QDIR%
