#/bin/bash

../genmake lib.prj COMPILER=VisualC > lib.mak

export logfile="build.log"
rm -f $logfile

vc32tools --make lib

echo "Build done."
