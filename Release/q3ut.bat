@set um=-umount ./baseq2/mar* -umount ./baseq2/q2maps* -umount ./baseq2/ztn2* -umount ./baseq2/dm*
@call q.bat -game=dirty %um% -mount ../quake3/baseq3/*.pk3 -mount ../quake3/ut/*.pk3 -developer -deathmatch -logfile -gl_clear %*
