@call common.bat
rem start quake2.exe +set nosound 1 +set logfile 2 +set developer 1 +game dirty
rem start quake2.exe +set nosound 1 +set r_drawfps 1 +set r_speeds 1 +set developer 1 +set game dirty +
start quake2.exe -nosound -r_drawfps=1 -r_speeds=1 -nointro -cheats=1 -map ztn2dm3 -give all
