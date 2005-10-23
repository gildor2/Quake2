#!/bin/bash

if [ -z "$target" ]; then
	# default target
	target="STATIC"
fi

cd ..

if ! [ -f "lib/libs.lib" ]; then	#?? integrate lib.project into quake2.project
	cd lib
	vc32tools --make libs-vc-win32
	cd ..
fi

export logfile="TestApp/build.log"
rm -f $logfile

TIMEFORMAT="Total time: %1R sec"
time vc32tools --make makefile-vc-win32 TestApp

Tools/SymInfoBuilder/work.pl TestApp

echo "Build done."
