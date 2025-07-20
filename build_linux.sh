#!/bin/sh
cmake -B ./build_linux
cmake --build ./build_linux --config RelWithDebInfo
cd .\\build_linux
cpack -C RelWithDebInfo
