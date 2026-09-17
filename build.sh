#!/bin/bash

# Default settings
BUILD_TYPE="Debug"
ASAN="OFF"
COMPILER=""
RUN_APP=false
EXE_NAME="zincbox"
CLEAN=false

usage() {
    echo "Usage: ./build.sh [flags]"
    echo "Flags:"
    echo "  asan     - Enable AddressSanitizer"
    echo "  release  - Build in release mode"
    echo "  debug    - Build in debug mode (default)"
    echo "  clang    - Use Clang (default)"
    echo "  gcc      - Use GCC"
    echo "  ninja    - Use Ninja build system (default)"
    echo "  make     - Use Make build system"
    echo "  run      - Run the program after build"
    echo "  clean    - Remove build directory"
    echo "  help     - Display this help"
    exit 0
}

check_cmd() {
    command -v "$1" &> /dev/null
}

HAS_GCC=false
HAS_CLANG=false
HAS_MAKE=false
HAS_NINJA=false
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
        ninja)        HAS_NINJA=true ;;
        make)         HAS_MAKE=true ;;
        run)     RUN_APP=true ;;
        help)    usage ;;
        *)       echo "Unknown parameter: $arg"; usage ;;
    esac
done

# Release/ASAN mutual exclusion
if [ "$HAS_ASAN" = true ] && [ "$HAS_RELEASE" = true ]; then
    echo "Error: 'asan' and 'release' are mutually exclusive."
    exit 1
fi

# Clean
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf build
    exit 0;
fi

# Compiler mutual exclusion
if [ "$HAS_GCC" = true ] && [ "$HAS_CLANG" = true ]; then
    echo "Error: 'gcc' and 'clang' are mutually exclusive."
    exit 1
fi

if [ "$HAS_GCC" = true ]; then
    if ! check_cmd gcc || ! check_cmd g++; then
        echo "Error: 'gcc' was explicitly requested but is not installed."
        exit 1
    fi
    COMPILER="gcc"
elif [ "$HAS_CLANG" = true ]; then
    if ! check_cmd clang || ! check_cmd clang++; then
        echo "Error: 'clang' was explicitly requested but is not installed."
        exit 1
    fi
    COMPILER="clang"
else
    # Fallback logic for compiler
    if check_cmd clang && check_cmd clang++; then
        COMPILER="clang"
    elif check_cmd gcc && check_cmd g++; then
        COMPILER="gcc"
    else
        echo "Error: Neither 'clang' nor 'gcc' was found on the system."
        exit 1
    fi
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

# Generator resolution and checks
if [ "$HAS_MAKE" = true ] && [ "$HAS_NINJA" = true ]; then
    echo "Error: 'make' and 'ninja' are mutually exclusive."
    exit 1
fi

if [ "$HAS_NINJA" = true ]; then
    if ! check_cmd ninja; then
        echo "Error: 'ninja' was explicitly requested but is not installed."
        exit 1
    fi
    GENERATOR="ninja"
elif [ "$HAS_MAKE" = true ]; then
    if ! check_cmd make; then
        echo "Error: 'make' was explicitly requested but is not installed."
        exit 1
    fi
    GENERATOR="make"
else
    # Fallback logic for generator
    if check_cmd ninja; then
        GENERATOR="ninja"
    elif check_cmd make; then
        GENERATOR="make"
    else
        echo "Error: Neither 'ninja' nor 'make' was found on the system."
        exit 1
    fi
fi

if [ "$GENERATOR" == "ninja" ]; then
    CMAKE_GENERATOR="-G Ninja"
else
    CMAKE_GENERATOR="-G Unix Makefiles"
fi

# Cleanup old binary
rm -f ./$EXE_NAME

# Project generation
if [ ! -f "build/build.ninja" ] && [ ! -f "build/Makefile" ]; then
cmake $CMAKE_GENERATOR -B build -S . \
  -DCMAKE_CXX_COMPILER=$CXX_PATH \
  -DCMAKE_C_COMPILER=$CC_PATH \
  -DENABLE_ASAN=$ASAN \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DMY_FLAGS="" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS="ON"
fi

# Compile
cmake --build build -j$(nproc)

# Copy binary
cp ./build/$EXE_NAME ./$EXE_NAME

# Execute
if [ "$RUN_APP" = true ]; then
    if [ "$ASAN" == "ON" ]; then
        export ASAN_OPTIONS="symbolize=1:handle_abort=1:print_stacktrace=1"
        export UBSAN_OPTIONS="print_stacktrace=1"
    fi
    ./$EXE_NAME
fi
