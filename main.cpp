#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/asio.hpp>
#include <fstream>
#include <pthread.h>
#include <csignal>
#include <chrono>


#include "Server/crcRoutine.h"
#include "Server/Server.h"
#include "Client/crcRoutine.h"
#include "Client/Client.h"

using namespace boost::asio;
using namespace std;

const int SERVER_ROLE = 1;
const int CLIENT_ROLE = 2;

const string WORKSPACE = "./";



// config path for mode 1
const string SERVER_CONFIG = WORKSPACE + "server_config.json";
const string CLIENT_CONFIG = WORKSPACE + "client_config.json";

// config path for mode 2
const string SYNC_CONFIG = WORKSPACE + "sync_config.json";

// flag signal
volatile bool crcSignal = false;
volatile bool clientSignal = false;
volatile bool shouldTerminate = false;
volatile bool serverOffline = false;
// args for crc Thread/ client call thread
struct ThreadArgs {
    string config;
};

// args for server host thread
struct ServerThreadArgs{
    string folderPath;
    string crcFile;
    int port;
};

int menu(){
    int choice = -1;
    cout << "Cac lua chon: "<< endl;
    cout << "1. Cap nhat du lieu theo 1 chieu" << endl;
    cout << "2. Cap nhat du lieu theo 2 chieu" << endl;
    cout << "3. Ket thuc" << endl;
    cout << "Lua chon cua ban: ";
    cin >> choice;
    if (choice < 1 or choice >3) choice = -1;
    return choice;
}

time_t getTimeNow(){
    auto current_time = std::chrono::system_clock::now();
    time_t time = std::chrono::system_clock::to_time_t(current_time);
    return time;
}

void printIPAddress() {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::resolver resolver(ioContext);

    try {
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve("localhost", "0");
        for (const auto& endpoint : endpoints) {
            boost::asio::ip::address address = endpoint.endpoint().address();
            if (address.is_v4()) {
                std::cout << "IPv4 Address: " << address.to_string() << std::endl;
            } else if (address.is_v6()) {
                std::cout << "IPv6 Address: " << address.to_string() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

bool isIPAddress(const std::string& input) {
    try {
        boost::asio::ip::address::from_string(input);
        return true; // If no exception is thrown, it's a valid IP address
    } catch (const std::exception& e) {
        return false; // Exception is thrown for invalid IP address
    }
}

bool isPathExists(const std::string& pathToCheck) {
    return fs::exists(pathToCheck);
}

void show_server_config(const std::string config_json){
    if (isPathExists(config_json)){
        ifstream configFile(config_json, ifstream::in);
        if (!configFile.is_open()) {
            cerr << "Error: Unable to open config file." << endl;
            return;
        }
        json conf;
        configFile >> conf;
        cout << "Thong tin tu server config: "<<endl;
        cout << "folderPath: "<< "\t" <<conf.at("folderPath")<<endl;
        cout << "port: " << "\t" << conf.at("port") <<endl;
        cout << ".crcFile: "<< "\t" << conf.at("crcFile")<<endl;
    } else {
        cout << "Canh bao: \t Duong dan khong ton tai !" << endl;
    }
}

void show_client_config(const std::string config_json){
    if (isPathExists(config_json)){
        ifstream configFile(config_json, ifstream::in);
        if(!configFile.is_open()){
            cerr << "Error: Unable to open config file." << endl;
            return;
        }
        json conf;
        configFile >> conf;
        cout << "Thong tin tu client config: "<<endl;
        cout << "folderPath: "<< "\t" <<conf.at("folderPath")<<endl;
        cout << "port: " << "\t" << conf.at("port") <<endl;
        cout << "ip: "<< "\t" << conf.at("ip")<<endl;
        cout << ".crcFile: "<< "\t" << conf.at("crcFile")<<endl;
        cout << "subToSync: "<< "\t" << conf.at("subToSync")<<endl;
    } else {
        cout << "Canh bao: \t Duong dan khong ton tai !" << endl;
    }
}

void show_sync_config(const std::string config_json){
    if (isPathExists(config_json)){
        ifstream configFile(config_json, ifstream::in);
        if (!configFile.is_open()) {
            cerr << "Error: Unable to open config file." << endl;
            return;
        }
        json conf;
        configFile >> conf;
        cout << "folderPath: "<< "\t" <<conf.at("folderPath")<<endl;
        cout << "port: " << "\t" << conf.at("port") <<endl;
        cout << "ip: "<< "\t" << conf.at("ip")<<endl;
        cout << ".crcFile: "<< "\t" << conf.at("crcFile")<<endl;
        cout << "subToSync: "<< "\t" << conf.at("subToSync")<<endl;
    } else {
        cout << "Canh bao: \t Duong dan khong ton tai !" << endl;
    }
}

bool isFolderPath(const std::string& path) {
    return fs::is_directory(path);
}

bool isInteger(const std::string& s) {
    if (s.empty()) {
        return false;
    }

    for (char c : s) {
        if (!std::isdigit(c)) {
            return false;
        }
    }

    return true;
}

bool isValidPath(const std::string& path) {
    try {
        // Sử dụng hàm absolute để kiểm tra tính hợp lệ của đường dẫn
        fs::path absolutePath = fs::absolute(path);
        return true;
    } catch (const std::exception& e) {
        // Bắt ngoại lệ nếu có lỗi xảy ra
        return false;
    }
}

// Function to open an existing JSON file
bool openJsonFile(const std::string& filename, json& jsonData) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        return false;
    }

    inputFile >> jsonData;
    inputFile.close();
    return true;
}


// Signal handler function
void handleSignal(int signo) {
    if (signo == SIGUSR1) {
        crcSignal = true;
    } else if (signo == SIGUSR2){
        clientSignal = true;
    }
}


void* crcRoutineFunction(void* arg){
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    while (!shouldTerminate){

        if(crcSignal){
            time_t timeNow = getTimeNow();
            cout <<"[ " << std::put_time(std::localtime(&timeNow), "%c") << " ]" << "CrcRoutine running...." << endl;
            CRCRoutine* crcRoutine = new CRCRoutine(); 
            int crc_result = crcRoutine->crcRoutine(args->config);
            delete crcRoutine;
            crcSignal = false;
        }
    }
    std::cout << "crcThread terminating..." << std::endl;
    pthread_exit(NULL);
}

void* clientCallFunction(void* arg){
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    while(!shouldTerminate){
        if(clientSignal){
            cout << "Dong bo du lieu: ";
            try{
                Client client(args->config);
                client.synchronizeData();
            } catch(const std::exception& e){
                std::cerr << e.what() << std::endl;
            }
            clientSignal = false;
        }
    }
    std::cout << "client call thread terminating..." << std::endl;
    pthread_exit(NULL);
}

void* serverHostFunction(void *arg){
    ServerThreadArgs* args = static_cast<ServerThreadArgs*>(arg);
    while(!shouldTerminate){
        if(!serverOffline){
            try{
                MyServer server(args->folderPath, args->crcFile);
                server.run_server(args->port);
            } catch (const std::exception& e){
                std::cerr << e.what() << std::endl;
            }
        }
    }
    std::cout << "Server host thread terminating..." << std::endl;
    pthread_exit(NULL);
}
int main(){

    // Register the signal handler
    signal(SIGUSR1, handleSignal);
    signal(SIGUSR2, handleSignal);
    bool is_continue = true;
    int choice;
    int role_choice;
    string continue_choice;
    string serverIP;
    fs::path scriptPath;
    MyServer* myServer;
    do{
        choice = menu();
        if (choice == -1) {
            cout << "Lua chon khong dung. Ban muon tiep tuc [y/N]? ";
            cin >> continue_choice;
            if (continue_choice == "y") {
            system("clear");
            continue;
            } else is_continue = false; 
        }
        if (choice == 3) {
            is_continue = false;
            break;
        }
        /// To: Process choice = 1 ( 1 chieu)
        if (choice == 1){
            system("clear");
            cout << "Che do 1: Cap nhat du lieu 1 chieu" << endl;
            cout << "Lua chon vai tro: [1] Server \t [2] Client\t";
            cin >> role_choice;
            if (role_choice == SERVER_ROLE){ 
                cout << "Your choice: [1] - server" << endl;
                cout << endl;
                printIPAddress();
                // Show server config
                cout << endl;
                show_server_config(SERVER_CONFIG);

                // server config 
                cout << "Thay doi/Tao moi server config? [y/N]" << "\t";
                cin >> continue_choice;
                if (continue_choice == "y") {
                    string folderpath;
                    string crcFile;
                    string port;
                    system("clear");
                    cout << "Sua thong tin cua Server Config" << endl;
                    do{
                        cout << "folderPath: \t";
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        getline(std::cin, folderpath);
                        if(!folderpath.empty() &&!isFolderPath(folderpath)) continue;
                        else break;
                    } while (!folderpath.empty());
                    do{
                        cout << "Port: \t";
                        getline(std::cin, port);
                        if(!isInteger(port) && !port.empty()) continue;
                        else break;
                    }while(!port.empty());
                    do{
                        cout << ".crcFile: \t";
                        getline(std::cin, crcFile);
                        if(!isValidPath(crcFile) && !crcFile.empty()) continue;
                        else break;
                    } while (!crcFile.empty());
                    json existingJsonData;
                    if(openJsonFile(SERVER_CONFIG,existingJsonData)){
                        std::ifstream inputFile(SERVER_CONFIG);
                        inputFile >> existingJsonData;
                        inputFile.close();
                        if(!folderpath.empty()) existingJsonData.at("folderPath") = folderpath;
                        if(!port.empty()) existingJsonData.at("port") = stoi(port);
                        if(!crcFile.empty()) existingJsonData.at("crcFile") = crcFile;

                        std::ofstream outputFile(SERVER_CONFIG);
                        outputFile << existingJsonData;
                        outputFile.close();
                    } else {
                        if(!folderpath.empty()) existingJsonData["folderPath"] = folderpath;
                        else {
                            cout << "Error: Loi file config...1" << endl;
                            cout << folderpath << endl;
                            continue;
                        }
                        if(!port.empty()) existingJsonData["port"] = stoi(port);
                        else {
                            cout << "Error: Loi file config...2" << endl;
                            continue;
                        }
                        if(!crcFile.empty()) existingJsonData["crcFile"] = crcFile;
                        else {
                            cout << "Error: Loi file config...3" << endl;
                            continue;
                        }
                        std::ofstream outputFile(SERVER_CONFIG);
                        outputFile << existingJsonData;
                        outputFile.close();
                    }
                }

                
                ThreadArgs* args = new ThreadArgs;
                args->config = SERVER_CONFIG;
                pthread_t crcThread;
                // Tao crc thread
                if(pthread_create(&crcThread, NULL, crcRoutineFunction, args) != 0){
                    std::cerr <<"Failed to create crcThread" << std::endl;
                }
                // Contruct server
                json conf;
                std::ifstream inputFile(SERVER_CONFIG);
                inputFile >> conf;
                inputFile.close();
                string folderPath = conf.at("folderPath");
                string crcFile = conf.at("crcFile");
                int port = conf.at("port");
                ServerThreadArgs* server_args = new ServerThreadArgs;
                server_args->folderPath = folderPath;
                server_args->crcFile = crcFile;
                server_args->port = port;
                
                pthread_t serverHostThread;

                if(pthread_create(&serverHostThread, NULL, serverHostFunction, server_args)!=0){
                    std::cerr << "Failed to create server host thread" << std::endl;
                }

                pthread_detach(crcThread);
                pthread_detach(serverHostThread);

                while (true){
                    char userInput;
                    sleep(1);
                    cout << "Nhap yeu cau: \t";
                    cin >> userInput;

                    if (userInput == 'U'){
                        pthread_kill(crcThread, SIGUSR1);
                    }
                    if (userInput == 'Q'){
                        serverOffline = true;
                        shouldTerminate = true;
                        sleep(1);
                        cout << "Phien lam viec ket thuc" << endl;
                        break;
                    }
                }


            } else if (role_choice == CLIENT_ROLE){
                cout << "Your choice: [2] - Client" << endl;
                cout << endl;
                // Show Client config
                show_client_config(CLIENT_CONFIG);

                // Client config
                cout << "Thay doi/Tao moi Client config? [y/N]" << "\t";
                cin >> continue_choice;
                if (continue_choice == "y"){
                    string folderpath;
                    string crcFile;
                    string port;
                    string ip;
                    string subToSync;
                    system("clear");
                    cout << "Sua thong tin cua Client Config" << endl;
                    do{
                        cout << "folderPath: \t";
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        getline(std::cin, folderpath);
                        if(!folderpath.empty() &&!isFolderPath(folderpath)) continue;
                        else break;
                    } while (!folderpath.empty());
                    do{
                        cout << "Port: \t";
                        getline(std::cin, port);
                        if(!isInteger(port) && !port.empty()) continue;
                        else break;
                    }while(!port.empty());
                    do{
                        cout << "IP: \t";
                        getline(std::cin, ip);
                        if(!isIPAddress(ip) && !ip.empty()) continue;
                        else break;
                    }while(!ip.empty());
                    do{
                        cout << ".crcFile: \t";
                        getline(std::cin, crcFile);
                        if(!isValidPath(crcFile) && !crcFile.empty()) continue;
                        else break;
                    } while (!crcFile.empty());
                    do{
                        cout << "subToSync: \t";
                        getline(std::cin, subToSync);
                        if(!(subToSync == "/" or subToSync == "\\") && !subToSync.empty()) continue;
                        else break;
                    } while (!subToSync.empty());
                    json existingJsonData;
                    if(openJsonFile(CLIENT_CONFIG,existingJsonData)){
                        std::ifstream inputFile(CLIENT_CONFIG);
                        inputFile >> existingJsonData;
                        inputFile.close();
                        if(!folderpath.empty()) existingJsonData.at("folderPath") = folderpath;
                        if(!port.empty()) existingJsonData.at("port") = stoi(port);
                        if(!ip.empty()) existingJsonData.at("ip") = ip;
                        if(!crcFile.empty()) existingJsonData.at("crcFile") = crcFile;
                        if(!subToSync.empty()) existingJsonData.at("subToSync") = subToSync;
                        std::ofstream outputFile(CLIENT_CONFIG);
                        outputFile << existingJsonData;
                        outputFile.close();
                    } else {
                        if(!folderpath.empty()) existingJsonData["folderPath"] = folderpath;
                        else {
                            cout << "Error: Loi file config...1" << endl;
                            cout << folderpath << endl;
                            continue;
                        }
                        if(!port.empty()) existingJsonData["port"] = stoi(port);
                        else {
                            cout << "Error: Loi file config...2" << endl;
                            continue;
                        }
                        if(!ip.empty()) existingJsonData["ip"] = ip;
                        else{
                            cout << "Error: Loi file config...3" << endl;
                            continue;
                        }
                        if(!crcFile.empty()) existingJsonData["crcFile"] = crcFile;
                        else {
                            cout << "Error: Loi file config...4" << endl;
                            continue;
                        }
                        if(!subToSync.empty()) existingJsonData["subToSync"] = subToSync;
                        else {
                            cout << "Error: Loi file config...5" << endl;
                            continue;
                        }
                        std::ofstream outputFile(CLIENT_CONFIG);
                        outputFile << existingJsonData;
                        outputFile.close();
                    }
                }

                ThreadArgs* args = new ThreadArgs;
                args->config = CLIENT_CONFIG;
                pthread_t crcThread;
                pthread_t clientCallThread;
                // Tạo thread
                if (pthread_create(&crcThread, NULL, crcRoutineFunction, args) != 0) {
                    std::cerr << "Failed to create crcThread" << std::endl;
                }

                if(pthread_create(&clientCallThread, NULL, clientCallFunction, args) != 0){
                    std::cerr << "Failed to create client call thread" << std::endl;
                }

                pthread_detach(crcThread);
                pthread_detach(clientCallThread);

                while (true) {
                    char userInput;
                    sleep(1);
                    cout << "Nhap yeu cau: \t";
                    cin >> userInput;

                    if (userInput == 'U') {
                        // Send signal to crcThread
                        pthread_kill(crcThread, SIGUSR1);
                    }

                    if (userInput == 'S') {
                        pthread_kill(clientCallThread, SIGUSR2);
                    }
                    if(userInput == 'Q') {
                        shouldTerminate = true;
                        sleep(1);
                        cout << "Phien lam viec ket thuc" << endl;
                        break;
                    }
                }
            } else {
                cout << "Lua chon khong dung !" << endl;

            }

            
            
        }


        /// TODO: Process if choice == 2 ( xu ly 2 chieu)
        if (choice == 2){
            system("clear");
            cout << "Che do : Cap nhat du lieu 2 chieu " << endl;
            cout << endl;
            printIPAddress();

            cout << endl;
            show_sync_config(SYNC_CONFIG);
            cout << "Ban muon tiep tuc? [y/N]? ";
            cin >> continue_choice;
            if (continue_choice == "y") {
                string folderpath;
                string crcFile;
                string port;
                string ip;
                string subToSync;
                system("clear");
                cout << "Sua thong tin cua Sync Config" << endl;
                do{
                    cout << "folderPath: \t";
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    getline(std::cin, folderpath);
                    if(!folderpath.empty() &&!isFolderPath(folderpath)) continue;
                    else break;
                } while (!folderpath.empty());
                do{
                    cout << "Port: \t";
                    getline(std::cin, port);
                    if(!isInteger(port) && !port.empty()) continue;
                    else break;
                }while(!port.empty());
                do{
                    cout << "IP: \t";
                    getline(std::cin,ip);
                    if(!isIPAddress(ip) && !ip.empty()) continue;
                    else break;
                }while(!ip.empty());
                do{
                    cout << ".crcFile: \t";
                    getline(std::cin, crcFile);
                    if(!isValidPath(crcFile) && !crcFile.empty()) continue;
                    else break;
                } while (!crcFile.empty());
                do{
                    cout << "subToSync: \t";
                    getline(std::cin, subToSync);
                    if(!(subToSync == "/" or subToSync == "\\") && !subToSync.empty()) continue;
                    else break;
                } while (!subToSync.empty());
                json existingJsonData;
                if(openJsonFile(SYNC_CONFIG,existingJsonData)){
                    std::ifstream inputFile(SYNC_CONFIG);
                    inputFile >> existingJsonData;
                    inputFile.close();
                    if(!folderpath.empty()) existingJsonData.at("folderPath") = folderpath;
                    if(!port.empty()) existingJsonData.at("port") = stoi(port);
                    if(!ip.empty()) existingJsonData.at("ip") = ip;
                    if(!crcFile.empty()) existingJsonData.at("crcFile") = crcFile;
                    if(!subToSync.empty()) existingJsonData.at("subToSync") = subToSync;
                    std::ofstream outputFile(SYNC_CONFIG);
                    outputFile << existingJsonData;
                    outputFile.close();
                } else{
                    if(!folderpath.empty()) existingJsonData["folderPath"] = folderpath;
                    else {
                        cout << "Error: Loi file config...1" << endl;
                        cout << folderpath << endl;
                        continue;
                    }
                    if(!port.empty()) existingJsonData["port"] = stoi(port);
                    else {
                        cout << "Error: Loi file config...2" << endl;
                        continue;
                    }
                    if(!ip.empty()) existingJsonData["ip"] = ip;
                    else{
                        cout << "Error: Loi file config...3" << endl;
                        continue;
                    }
                    if(!crcFile.empty()) existingJsonData["crcFile"] = crcFile;
                    else {
                        cout << "Error: Loi file config...4" << endl;
                        continue;
                    }
                    if(!subToSync.empty()) existingJsonData["subToSync"] = subToSync;
                    else {
                        cout << "Error: Loi file config...5" << endl;
                        continue;
                    }
                    std::ofstream outputFile(SYNC_CONFIG);
                    outputFile << existingJsonData;
                    outputFile.close();
                }
            } else is_continue = false; 
        }
        

    }while(is_continue);
    return 0;
}