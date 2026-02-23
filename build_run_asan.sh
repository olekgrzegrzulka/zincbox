export CXX=/usr/bin/clang++
clear; rm ./music;  cmake -DCMAKE_BUILD_TYPE=Debug -DMY_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g -O1" . && make -j && ./music
