#!/bin/bash

rm -rf ../lib/Release
rm -f ../lib/*.{ncb,plg}

rm -f ../*.{ncb,plg}
rm -rf ../ref_{gl,soft,gl.old}/{Release/*,Debug}
rm -rf ../Debug
rm -f ../ref_{gl,soft,gl.old}/*.{ncb,plg}

rm -f *.obj *.pch *.exp *.lib *.idb *.pdb
rm -f q2.res build.log
