#/bin/bash

source vc32tools

# set "single" to "1" (builtin ref_xxx) or "0" (external ref_xxx)
single=1
log="build.log"

PrepareVC
rm -f *.pch $log
cd ..

echo "----- Building Quake2 -----"
logfile="release/$log"
if [ "$single" == "1" ]; then
	prj="quake2s"
else
	prj="quake2"
fi

echo "** building $prj" >> $logfile
if ! Make $prj; then
	echo "errors ..." >> $logfile
	exit 1;
fi

echo "----- Building old ref_gl -----"
logfile="../release/$log"
cd ref_gl.old

echo "** building old ref_gl" >> $logfile
if ! Make ref_gl; then
	echo "errors ..." >> $logfile
	exit 1
fi

cd ..
echo "Build done."
