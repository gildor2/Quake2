#/bin/bash

../Tools/GenMake/genmake libs.project TARGET=vc-win32 > libs-vc-win32
../Tools/GenMake/genmake libs.project TARGET=mingw32  > libs-mingw32

export logfile="build.log"
rm -f $logfile

vc32tools --make libs-vc-win32

echo "Build done."
