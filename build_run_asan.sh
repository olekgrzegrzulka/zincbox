# export CXX=/usr/bin/clang++
# clear; rm ./music;  cmake -DCMAKE_BUILD_TYPE=Debug -DMY_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g -O1" . && make -j && ./music




set -e
export CXX=/usr/bin/clang++
clear
rm -f ./music
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=""
# cmake -DMY_FLAGS="" .
cd ./build
make -j

mv  ./music ../music
cd ..
./music
