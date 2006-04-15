@call common.bat
rem Can launch with following line, but will create "baseq2" and uses empty cfg
set um=-umount ./baseq2/mar* -umount ./baseq2/q2maps* -umount ./baseq2/ztn2* -umount ./baseq2/dm*
rem start quake2.exe  %um% -nosound -r_drawfps=1 -r_stats=1 -nointro -cheats=1 -game=hl -map ahl_hkheat -wait 10 -noclip -developer
rem start quake2.exe  %um% -nosound -r_drawfps=1 -r_stats=1 -nointro -cheats=1 -map q1dm2 -wait -noclip -developer
start quake2.exe %um% -game=hl -nosound -r_drawfps=1 -r_stats=1 -nointro -cheats=1 -mount ../quake1/id1/pak2.pak -developer -load save1
rem other maps: bounce
