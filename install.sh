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

function update_libs () {
	cd $MAIN
	wget https://raw.githubusercontent.com/emilk/loguru/master/loguru.hpp -O include/loguru.hpp
	wget https://raw.githubusercontent.com/nlohmann/json/develop/src/json.hpp -O include/json.hpp
	wget https://raw.githubusercontent.com/brechtsanders/proxysocket/master/src/proxysocket.c -O include/proxysocket.c
	wget https://raw.githubusercontent.com/brechtsanders/proxysocket/master/src/proxysocket.h -O include/proxysocket.h
	wget https://raw.githubusercontent.com/FraMecca/libargparse/master/src/argparse.c -O include/argparse.c
	wget https://raw.githubusercontent.com/FraMecca/libargparse/master/src/argparse.h -O include/argparse.h
}

# clone and build libdill
dill_clone

# copy necessary libs to include
cd $MAIN
cp libdill/{fd.*,utils.*,iol.*} ./include/
# now update single header libs in include directory
update_libs

# build torchat with target
cd $MAIN
make $TG
# set permissions on tor directory
chmod -R 700 $MAIN/tor
