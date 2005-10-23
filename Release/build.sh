#!/bin/bash

if [ -z "$target" ]; then
	# default target
	target="STATIC"
fi

cd ..

if ! [ -f "lib/libs.lib" ]; then	#?? integrate libs.project into quake2.project
	cd lib
	vc32tools --make libs-vc-win32
	cd ..
fi

export logfile="Release/build.log"
rm -f $logfile

#export vc_ver=7

TIMEFORMAT="Total time: %1R sec"
time vc32tools --make makefile-vc-win32 $target

Tools/SymInfoBuilder/work.pl Release vc

echo "Build done."
