@call common.bat
rem start quake2.exe +set nosound 1 +set logfile 2 +set developer 1 +game dirty
rem start quake2.exe +set nosound 1 +set r_drawfps 1 +set r_speeds 1 +set developer 1 +set game dirty +
start quake2.exe -dedicated -rcon_password=123 -cheats -map ztn2dm2
copy quake2.exe ..\quake2.aaa\*.*
cd ..\quake2.aaa
start quake2.exe -nosound -nointro -rcon_password=123 -rcon_address=127.0.0.1 -connect 127.0.0.1 -cl_extProtocol=0
