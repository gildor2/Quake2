@call common.bat
rem Can launch with following line, but will create "baseq2" and uses empty cfg
set um=-umount ./baseq2/mar* -umount ./baseq2/q2maps* -umount ./baseq2/ztn2* -umount ./baseq2/dm*
start quake2.exe %um% -game=hl -nosound -r_drawfps=1 -r_stats=1 -nointro -cheats=1 -developer -map ahl_casagrande
rem other maps: bounce
