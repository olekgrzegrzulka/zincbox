set -e 
export CXX=/usr/bin/clang++
clear
rm -f ./music
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DMY_FLAGS=""
# cmake -DMY_FLAGS="" .
cd ./build
make -j

mv  ./music ../music
cd ..
./music




# cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DMY_FLAGS=""
# cmake --build build -j
# mv ./build/Redrawn ./RedrawnDebug
