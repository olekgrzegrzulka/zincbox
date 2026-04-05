#!/bin/bash

set -e
export CXX=/usr/bin/clang++
export CC=/usr/bin/clang
clear

rm -f ./music

cmake -B build -S . \
  -DENABLE_ASAN=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMY_FLAGS="" \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold"

cd ./build
make -j$(nproc)

mv  ./music ../music
cd ..
export ASAN_OPTIONS="symbolize=1:handle_abort=1:print_stacktrace=1"
export UBSAN_OPTIONS="print_stacktrace=1"
./music
