# g++  -I ./include crcRoutine.cpp -L/usr/local/lib -lz  -o crcRoutine.o -std=c++17
# g++ -I./include -shared -fPIC crcRoutine.cpp -L/usr/local/lib -lz -o libcrcRoutine.so -std=c++17
g++ -I./include -shared -fPIC hashRoutine.cpp -L/usr/local/lib -lz -o libhashRoutine.so -std=c++17 -lssl -lcrypto
# g++  -I ./include main.cpp -L/usr/local/lib -lz  -o main.o -std=c++17 -lcpr
g++ -I ./include -shared -fPIC Client.cpp -L/usr/local/lib -lz  -o libClient.so -std=c++17 -lcpr