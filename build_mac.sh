#!/bin/sh
cmake -B ./build_mac -G "Xcode" -DAAX_SIGN_ID="7P7G2A8U54" -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=7P7G2A8U54  --fresh
cmake --build ./build_mac --config RelWithDebInfo --clean-first
cd ./build_mac
cpack -C RelWithDebInfo
