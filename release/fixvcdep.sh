#!/bin/bash

tmpfile="tmp.dep"

function FixDep()
{
	if [ -f $1 ]; then
		grep --invert-match --regexp=basetsd.h $1 > $tmpfile
		mv -f $tmpfile $1
#		echo "parse $1"
	fi
}


# fix main dependencies
cd ..
FixDep quake2s.dep
FixDep quake2.dep

# fix ref_gl dependencies
cd ref_gl.old
FixDep ref_gl.dep
