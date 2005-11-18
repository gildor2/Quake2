#!/bin/bash

#PLATFORM="mingw32"

[ "$PLATFORM" ] || PLATFORM="vc-win32"
# default output directory
[ "$OUT" ]      || OUT="Release"
# default target for make
[ "$TARGET" ]   || TARGET="STATIC"

# force PLATFORM=linux under Linux OS
#?? check this, when cross-compile under wine
[ "$OSTYPE" == "linux-gnu" ] && PLATFORM="linux"

#export vc_ver=7

#----------------------------------------------------------
#	configure build tools
#----------------------------------------------------------

case "$PLATFORM" in
	"vc-win32")
		build="vc32tools --make"
		maptype="vc"
		libext=".lib"
		;;
	"mingw32"|"cygwin")
		PATH=/bin:/usr/bin:$PATH		# configure CygWin paths
		build="gccfilt make -f"			#?? use Tools/gccfilt ?
		maptype="gcc"
		libext=".a"
		;;
	"linux")
		build="Tools/gccfilt make -f"	# logging + colorizing
		maptype="gcc"
		libext=".a"
		;;
	"")
		echo "PLATFORM is not specified"
		exit 1
		;;
	*)
		echo "Unknown PLATFORM=\"$PLATFORM\""
		exit 1
esac

# root directory
cd ..
# logfile
export logfile="$OUT/build.log"
rm -f $logfile


#----------------------------------------------------------
#	make libs
#----------------------------------------------------------

#?? integrate libs.project into quake2.project
#!! "lib/libs.a": linux/mingw/cygwin: should distinguish!
if ! [ -f "lib/libs$libext" ]; then
	cd lib
	#?? logfile is not correct here (should be ../$OUT ...)
	$build libs-$PLATFORM
	cd ..
fi

#----------------------------------------------------------
#	build
#----------------------------------------------------------

TIMEFORMAT="Build time: %1R sec"
time $build makefile-$PLATFORM $TARGET || exit 1
# generate symbols.dbg
Tools/SymInfoBuilder/work.pl $OUT $maptype

echo "Build done."
