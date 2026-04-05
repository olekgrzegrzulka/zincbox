#!/bin/bash

set -e
export CXX=/usr/bin/clang++
export CC=/usr/bin/clang
clear

rm -f ./music

cmake -B build -S . \
  -DENABLE_ASAN=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DMY_FLAGS="" \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold"

cd ./build
make -j$(nproc)

mv  ./music ../music
cd ..
./music
