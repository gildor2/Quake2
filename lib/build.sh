#/bin/bash

source vc32tools

# set "single" to "1" (builtin ref_xxx) or "0" (external ref_xxx)
log="build.log"

PrepareVC
rm -f *.pch $log

logfile="release/$log"
if ! Make lib; then
	echo "errors ..." >> $logfile
	exit 1;
fi

echo "Build done."
