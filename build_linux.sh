#!/bin/sh
cmake -B ./build_linux --fresh
cmake --build ./build_linux --config RelWithDebInfo --clean-first
cd ./build_linux
cpack -C RelWithDebInfo
