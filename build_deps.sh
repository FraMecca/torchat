#!/bin/bash

MAIN=$(pwd)

cd include/libConfuse/
bash autogen.sh
./configure
make

cp src/.libs/libconfuse.so.1.1.0 $MAIN/build/libconfuse.so.1.1.0
cd $MAIN
cd build
ln -s  libconfuse.so.1.1.0 libconfuse.so
ln -s  libconfuse.so.1.1.0 libconfuse.so.1
