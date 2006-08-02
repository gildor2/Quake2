#/bin/bash

function Make()
{
	while [ $# -gt 0 ]; do
		../Tools/genmake libs.project TARGET=$1 > libs-$1 || exit 1
		shift
	done
}

if [ "$OSTYPE" != "linux-gnu" ]; then
	Make vc-win32 mingw32 cygwin
else
	Make linux
fi
