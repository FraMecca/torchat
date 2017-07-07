#!/bin/bash

git submodule init
git submodule update
cd ./include/argparse
git pull origin master
cd ../proxysocket
git pull origin master
cd ../uthash
git pull origin master
cd ../libConfuse
git pull origin master

