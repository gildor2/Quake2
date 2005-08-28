@call common.bat
rem using "vid_restart" for reloading shader scripts
start quake2.exe -nosound -cheats -nointro -unloadpak q2pl* -loadpak ../../quake3/baseq3/pak0* -vid_restart
