#!/bin/bash

cd ./include/libConfuse/
bash autogen.sh
./configure
make
cp src/.libs/libconfuse.so.1.1.0 ../../build/libconfuse.so
