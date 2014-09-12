@echo off
bash build.sh

mkdir test
cd test

rem ..\fontgen.exe -name Abc1 -winname "Courier" -size 10
rem ..\fontgen.exe -name Abc2 -winname "Courier New" -size 10 -tga packed,8bit
..\fontgen.exe -name debug -winname "Consolas" -size 14 -tga packed,8bit -cpp

#start debug.tga
