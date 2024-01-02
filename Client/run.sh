g++  -I ./include crcRoutine.cpp -L/usr/local/lib -lz  -o crcRoutine.o -std=c++17
g++  -I ./include main.cpp -L/usr/local/lib -lz  -o main.o -std=c++17 -lcpr