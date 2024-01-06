#!/bin/bash
### Update 
sudo apt update && sudo apt upgrade
### 1. install g++ and cmake
sudo apt install g++ cmake

### 2. install zlib
sudo apt install zlib1g zlib1g-dev

### 3. install cpr lib
sudo apt-get install libssl-dev -y
cd Client/include
git clone https://github.com/libcpr/cpr.git
cd cpr && mkdir build && cd build
cmake .. -DCPR_USE_SYSTEM_CURL=OFF
cmake --build . --parallel
sudo cmake --install .
cd ../../../../

### 4. install boost lib
sudo apt-get install libboost-all-dev -y

### 5. install argparse lib
mkdir include && cd include
# Clone the repository
git clone https://github.com/p-ranav/argparse
cd argparse

# Build the tests
mkdir build
cd build
cmake -DARGPARSE_BUILD_SAMPLES=on -DARGPARSE_BUILD_TESTS=on ..
make
./test/tests
sudo make install

cd ../../../

# ### 6. Build app
# chmod u+x ./run.sh
# ./run.sh