sudo apt install zlib1g-dev
g++  -I ./include crcRoutine.cpp -L/usr/local/lib -lz  -o crcRoutine.o