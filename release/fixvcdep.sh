#!/bin/bash

tmpfile="tmp.dep"

function FixDep()
{
  grep --revert-match --regexp=basetsd.h $1 > $tmpfile
  mv -f $tmpfile $1
}


# fix main dependencies
cd ..
FixDep quake2s.dep

# fix ref_gl dependencies
cd ref_gl.old
FixDep ref_gl.dep
