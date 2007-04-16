@set um=-umount ./baseq2/mar* -umount ./baseq2/q2maps* -umount ./baseq2/ztn2* -umount ./baseq2/dm*
@call q.bat %um% -umount -mount ../quake3/baseq3/*.pk3 -developer -map q3dm7 -deathmatch -logfile -gl_clear
