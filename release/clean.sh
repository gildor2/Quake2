#!/bin/bash

rm -rf ../lib/Release
rm -f ../lib/*.{ncb,plg,opt}

rm -f ../*.{ncb,plg,opt}
rm -rf ../ref_{gl,soft,gl.old}/{Release/*,Debug}
rm -rf ../Debug
rm -f ../ref_{gl,soft,gl.old}/*.{ncb,plg,opt}

rm -f *.obj *.pch *.exp *.lib *.idb *.pdb
rm -f q2.res *.log
