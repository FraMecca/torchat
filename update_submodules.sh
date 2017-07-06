#!/bin/bash

git submodule init
git submodule update
cd ./include/argparse
git pull origin master
