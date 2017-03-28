#!/bin/bash

MAIN=$(pwd)
BUILD=$MAIN/build
DILL=$MAIN/libdill
DPATH=https://github.com/sustrik/libdill.git

if [ $# -gt 1 ]; then
	echo "USAGE: ./install.sh [target] OR ./install.sh for default build target."
	echo "Possible targets are: default, debug, asan"
	exit
elif [ $# -eq 0 ]; then
	TG="default"
else
	TG=$1
fi

# in a function since we change directory (conceptually cleaner)
function dill_clone () {
	if [ ! -d "$DILL" ]; then
		# if it doesn't exist, clone it, otherwise skip
		git clone $DPATH
	fi
	cd $DILL
	./autogen.sh
	if [ $? -ne 0 ]; then
		./autogen.sh
	fi
	./configure --prefix=$BUILD
	make
}

# clone and build libdill
dill_clone
cd $MAIN
# build torchat with target
make $TG
# set permissions on tor directory
chmod -R 700 $MAIN/tor
