@echo off
copy /b quake2.exe \games\quake2\*.*
copy /b ref_gl.dll \games\quake2\*.*
copy /b ref_soft.dll \games\quake2\*.*
copy /b ..\4.XX_Changes.txt \games\quake2\*.*
cd \games\quake2\
rem start \games\quake2\quake2.exe +set s_initsound 0 +set developer 1 +game dirty
start \games\quake2\quake2.exe +set s_initsound 0 +game dirty +loadpak ../hl/pak0rip +loadpak ../hl/pak1
rem  +map boot_camp
