#!/bin/bash
cd Client
chmod u+x ./run.sh
./run.sh
cd ..
cd Server
chmod u+x ./run.sh
./run.sh
cd ..
g++ -I./Server/include   -o main.o -c main.cpp  -lz -std=c++17 -lssl -lcrypto
g++ main.o ./Server/libhashRoutine.so ./Server/libServer.so ./Client/libhashRoutine.so ./Client/libClient.so  -o App  -lz -std=c++17
