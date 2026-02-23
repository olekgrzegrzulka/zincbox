export CXX=/usr/bin/clang++
clear
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DMY_FLAGS=""
cmake --build build -j
mv ./build/music ./music
