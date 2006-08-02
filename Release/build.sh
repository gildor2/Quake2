#!/bin/bash

#PLATFORM="mingw32"
TARGET="STATIC"
#export vc_ver=7

source ../build-common

# logfile
export logfile="$rootdir/$OUT/build.log"
rm -f $logfile


#----------------------------------------------------------
#	build
#----------------------------------------------------------

# root directory
cd ..

# verify makefile date
[ makefile-$PLATFORM -ot quake2.project ] &&
	echo -e "\e[31mWARNING: makefile for $PLATFORM is older than project file -- recreate it!\e[0m"

time $build makefile-$PLATFORM $TARGET || exit 1
# generate symbols.dbg
$tools/SymInfoBuilder/work.pl $OUT $maptype

echo "Build done."
