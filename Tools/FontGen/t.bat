@echo off
bash build.sh

if not exist test mkdir test
cd test

::set styles=-styles antialias
set font=-winname "Consolas" -size 14
::set font=-winname "Courier New" -size 10

..\fontgen.exe -name Debug %font% %styles% -tga packed,8bit
..\fontgen.exe -name Debug-packed %font% %styles% -cpp packed
..\fontgen.exe -name Debug-unpacked %font% %styles% -cpp

::start debug.tga
