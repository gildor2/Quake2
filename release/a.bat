@echo off
rem del \games\quake2\quake2.exe
rem del \games\quake2\ref_gl.dll
copy /b quake2.exe \games\quake2\*.*
copy /b ref_gl.dll \games\quake2\*.*
copy /b ref_oldgl.dll \games\quake2\*.*
copy /b ref_soft.dll \games\quake2\*.*
copy /b ..\4.XX_Changes.txt \games\quake2\*.*
cd \games\quake2\
rem start \games\quake2\quake2.exe +set s_initsound 0 +set logfile 2 +set developer 1 +game dirty
rem start \games\quake2\quake2.exe +set s_initsound 0 +set r_drawfps 1 +set r_speeds 1 +set developer 1 +set game dirty +echo
start \games\quake2\quake2.exe +set s_initsound 0 +set r_drawfps 1 +set r_speeds 1 +set game dirty +echo
