#!/bin/bash
set -e

rm -rf build
rm -f ./music.exe

cmake -B build -S . \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DENABLE_ASAN=OFF \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXE_LINKER_FLAGS="-static"

cmake --build build -j$(nproc)

mv ./build/music.exe ./music.exe
