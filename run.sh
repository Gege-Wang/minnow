#! /bin/bash
rm -rf ./build
cmake -S . -B build
cmake --build build
./build/apps/webget cs144.keithw.org /hello