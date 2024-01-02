g++ -I./Server/include   -o main.o -c main.cpp  -lz -std=c++17
g++ main.o ./Server/libcrcRoutine.so -o main  -lz -std=c++17