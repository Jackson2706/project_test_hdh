#!/bin/bash
cd Client
chmod u+x ./run.sh
./run.sh
cd ..
cd Server
chmod u+x ./run.sh
./run.sh
cd ..
g++ -I./Server/include   -o main.o -c main.cpp  -lz -std=c++17
g++ main.o ./Server/libcrcRoutine.so ./Server/libServer.so ./Client/libcrcRoutine.so ./Client/libClient.so  -o main  -lz -std=c++17
./main