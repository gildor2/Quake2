#!/bin/bash

if [ -z "$target" ]; then
	# default target
	target="static"
fi

cd ..

if ! [ -f "lib/lib.lib" ]; then	#?? integrate lib.prj into quake2.prj
	cd lib
	vc32tools --make lib
	cd ..
fi

export logfile="Release/build.log"
rm -f $logfile

TIMEFORMAT="Total time: %1R sec"
time vc32tools --make quake2 $target

Tools/SymInfoBuilder/work.pl Release

echo "Build done."
