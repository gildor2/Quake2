@call common.bat
rem start quake2.exe -nosound -logfile=2 -developer -game=dirty
rem start quake2.exe -nosound -r_drawfps=1 -r_stats=1 -developer -game=dirty
start quake2.exe -nosound=1 -r_drawfps -r_stats -game=dirty -nointro -cheats %*
rem BUG: start quake2.exe -nosound -r_drawfps -r_stats -game=dirty -nointro -cheats -map ground3 -r_showBrush=1585
