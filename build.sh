#!/bin/bash

# Default settings
BUILD_TYPE="Debug"
ASAN="OFF"
COMPILER=""
RUN_APP=false
EXE_NAME="zincbox"
CLEAN=false

usage() {
    echo "Usage: ./run.sh [flags]"
    echo "Flags:"
    echo "  asan     - Enable AddressSanitizer"
    echo "  release  - Build in release mode"
    echo "  debug    - Build in debug mode (default)"
    echo "  gcc      - Use GCC"
    echo "  clang    - Use Clang (default)"
    echo "  run      - Run the program after build"
    echo "  clean    - Remove build directory"
    echo "  help     - Display this help"
    exit 0
}

HAS_GCC=false
HAS_CLANG=false
HAS_ASAN=false
HAS_RELEASE=false
HAS_DEBUG=false

for arg in "$@"; do
    case $arg in
        clean)   CLEAN=true ;;
        asan)    HAS_ASAN=true; ASAN="ON" ;;
        release) HAS_RELEASE=true; BUILD_TYPE="Release" ;;
        debug)   HAS_DEBUG=true; BUILD_TYPE="Debug" ;;
        gcc)     HAS_GCC=true; COMPILER="gcc" ;;
        clang)   HAS_CLANG=true; COMPILER="clang" ;;
        run)     RUN_APP=true ;;
        help)    usage ;;
        *)       echo "Unknown parameter: $arg"; usage ;;
    esac
done

# Clean
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Compiler mutual exclusion
if [ "$HAS_GCC" = true ] && [ "$HAS_CLANG" = true ]; then
    echo "Error: 'gcc' and 'clang' are mutually exclusive."
    exit 1
fi
# Set default compiler if none specified
[ -z "$COMPILER" ] && COMPILER="clang"

# Release/ASAN mutual exclusion
if [ "$HAS_ASAN" = true ] && [ "$HAS_RELEASE" = true ]; then
    echo "Error: 'asan' and 'release' are mutually exclusive."
    exit 1
fi

set -e
clear

# Compiler configuration
if [ "$COMPILER" == "clang" ]; then
    export CXX_PATH=/usr/bin/clang++
    export CC_PATH=/usr/bin/clang
else
    export CXX_PATH=/usr/bin/g++
    export CC_PATH=/usr/bin/gcc
fi

# Cleanup old binary
rm -f ./$EXE_NAME

# Project generation
cmake -B build -S . \
  -DCMAKE_CXX_COMPILER=$CXX_PATH \
  -DCMAKE_C_COMPILER=$CC_PATH \
  -DENABLE_ASAN=$ASAN \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DMY_FLAGS="" \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold"

# Resource bundling
if [ -d "./resources" ]; then
    cd ./resources
    [ -f ../build/resources.zip ] && rm ../build/resources.zip
    zip -rq ../build/resources.zip .
    cd ..
fi

# Compilation
cd ./build
make -j$(nproc)

# Move binary
mv ./$EXE_NAME ../$EXE_NAME
cd ..

# Execution logic
if [ "$RUN_APP" = true ]; then
    if [ "$ASAN" == "ON" ]; then
        export ASAN_OPTIONS="symbolize=1:handle_abort=1:print_stacktrace=1"
        export UBSAN_OPTIONS="print_stacktrace=1"
    fi
    ./$EXE_NAME
fi
