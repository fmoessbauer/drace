root=$(pwd)
dynamorio32=${root}"/DynamoRIO-i386-Linux-7.91.18271-0/cmake"

mkdir build32
cd build32
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_C_FLAGS=-m32 -DBOOST_LIBRARYDIR=/usr/lib/i386-linux-gnu -DDynamoRIO_DIR=$dynamorio32 -DDRACE_ENABLE_RUNTIME=1 -DBUILD_TESTING=1 -DDRACE_ENABLE_BENCH=0 -DCMAKE_INSTALL_PREFIX:PATH=package  ..

cmake --build .
cmake --build .  --target install
