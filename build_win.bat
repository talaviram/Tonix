@echo off 
REM This assumes you have an AAX codesign tools. to build without, simply don't pass the AAX_SIGN_ID
cmake -B ./build -DAAX_SIGN_ID=7915a408860606418b22b8942bb2f3c0f5fe3eee`
cmake --build ./build --config RelWithDebInfo
cd .\build
cpack -C RelWithDebInfo
