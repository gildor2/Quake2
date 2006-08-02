#!/bin/bash

#PLATFORM="cygwin"
OUT="TestApp"

source ../build-common

cd ..
time $build makefile-$PLATFORM TESTAPP
$tools/SymInfoBuilder/work.pl $OUT $maptype
