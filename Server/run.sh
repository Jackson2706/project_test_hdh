# g++  -I ./include crcRoutine.cpp -L/usr/local/lib -lz  -o crcRoutine.o -std=c++17
g++ -I./include -shared -fPIC crcRoutine.cpp -L/usr/local/lib -lz -o libcrcRoutine.so -std=c++17

# g++ main.cpp  -o main.o -lpthread ./include/out/httplib.cc -I ./include/ -std=c++17
g++ -I ./include/ -shared -fPIC Server.cpp ./include/out/httplib.cc -lz -o libServer.so -std=c++17