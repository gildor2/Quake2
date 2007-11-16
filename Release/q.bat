@call common.bat
rem Can launch with following line (without call common.bat), but will create "baseq2" and copy config to it:
rem quake2 -nosound -r_drawfps=1 -r_stats=1 -nointro -cheats=1 -cddir=/games/quake2
start quake2.exe -nosound=1 -r_drawfps=1 -r_stats=1 -nointro -cheats=1 %*
