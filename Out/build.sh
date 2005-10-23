#!/bin/bash

if [ -z "$target" ]; then
	# default target
	target="STATIC"
fi

cd ..

#if ! [ -f "lib/lib.lib" ]; then	#?? integrate lib.project into quake2.project
#	cd lib
#	vc32tools --make lib
#	cd ..
#fi

export logfile="Out/build.log"
rm -f $logfile

TIMEFORMAT="Total time: %1R sec"
PATH=/bin:/usr/bin:$PATH
time gccfilt make -f makefile-mingw32 $target

Tools/SymInfoBuilder/work.pl Out gcc

echo "Build done."
