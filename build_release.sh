set -e
export CXX=/usr/bin/clang++
clear
rm -f ./music
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DMY_FLAGS=""
cd ./build
make -j

mv  ./music ../music
cd ..
./music
