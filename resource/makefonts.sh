#!/bin/bash

fontgen="$PWD/../Tools/FontGen/fontgen"
params="-tga packed,8bit"

cd res/baseq2/fonts

$fontgen -name debug -winname "Consolas" -size 10 $params
$fontgen -name console -winname "Consolas" -size 15 $params
