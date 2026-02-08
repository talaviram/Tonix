@echo off 
REM This assumes you have an AAX codesign tools. to build without, simply don't pass the AAX_SIGN_ID
cmake -B ./build_win -DAAX_SIGN_ID=7915a408860606418b22b8942bb2f3c0f5fe3eee` --fresh
cmake --build ./build_win --config RelWithDebInfo --clean-first
cd .\build_win
cpack -C RelWithDebInfo
