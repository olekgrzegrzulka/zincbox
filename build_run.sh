set -e
export CXX=/usr/bin/clang++
clear

rm -f ./music

cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMY_FLAGS="" \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold"

cd ./build
make -j

mv  ./music ../music
cd ..
./music
