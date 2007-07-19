@echo off
set um=-umount ./baseq2/mar* -umount ./baseq2/q2maps* -umount ./baseq2/ztn2* -umount ./baseq2/dm*
rem set map=q3dm2
set map=q3dm10
call q.bat %um% -mount ../quake3/baseq3/*.pk3 -developer -map %map% -deathmatch -logfile -gl_clear %*
