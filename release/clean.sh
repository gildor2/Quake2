#!/bin/bash

rm -rf ../ref_{gl,soft,gl.old}/{release,debug}/*
rm -rf ../debug

rm -f *.obj *.pch *.exp *.lib *.idb *.pdb
rm -f q2.res build.log
