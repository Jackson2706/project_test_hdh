#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/asio.hpp>
#include <fstream>
#include <pthread.h>
#include <csignal>
#include <chrono>
#include "argparse/argparse.hpp"

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

// reset init
void reset(){
    crcSignal = false;
    clientSignal = false;
    shouldTerminate = false;
    serverOffline = false;
}

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

string getCurrentTime() {
    // Get the current time
    auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Convert to local time
    std::tm* timeInfo = std::localtime(&currentTime);

    // Format the time as a string
    std::stringstream ss;
    ss << std::put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    
    return "[ " + ss.str() +" ]:\t";
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
            cerr << getCurrentTime() <<"Error: Unable to open config file." << endl;
            return;
        }
        json conf;
        configFile >> conf;
        cout << getCurrentTime() << "Thong tin tu server config: "<<endl;
        cout << "\tfolderPath: "<< "\t" <<conf.at("folderPath")<<endl;
        cout << "\tport:       " << "\t" << conf.at("port") <<endl;
        cout << "\t.crcFile:   "<< "\t" << conf.at("crcFile")<<endl;
    } else {
        cout << getCurrentTime() << "Canh bao: \t Duong dan khong ton tai !" << endl;
    }
}

void show_client_config(const std::string config_json){
    if (isPathExists(config_json)){
        ifstream configFile(config_json, ifstream::in);
        if(!configFile.is_open()){
            cerr << getCurrentTime() << "Error: Unable to open config file." << endl;
            return;
        }
        json conf;
        configFile >> conf;
        cout << getCurrentTime() << "Thong tin tu client config: "<<endl;
        cout << "\tfolderPath: "<< "\t" <<conf.at("folderPath")<<endl;
        cout << "\tport:       " << "\t" << conf.at("port") <<endl;
        cout << "\tip:         "<< "\t" << conf.at("ip")<<endl;
        cout << "\t.crcFile:   "<< "\t" << conf.at("crcFile")<<endl;
        cout << "\tsubToSync:  "<< "\t" << conf.at("subToSync")<<endl;
    } else {
        cout << getCurrentTime() << "Canh bao: \t Duong dan khong ton tai !" << endl;
    }
}

void show_sync_config(const std::string config_json){
    if (isPathExists(config_json)){
        ifstream configFile(config_json, ifstream::in);
        if (!configFile.is_open()) {
            cerr << getCurrentTime() << "Error: Unable to open config file." << endl;
            return;
        }
        json conf;
        configFile >> conf;
        cout <<getCurrentTime() << "Thong tin tu client config: "<<endl;
        cout << "\tfolderPath:       "<< "\t" <<conf.at("folderPath")<<endl;
        cout << "\tport for calling: " << "\t" << conf.at("port") <<endl;
        cout << "\tport for hosting: " << "\t" << conf.at("port_host") <<endl;
        cout << "\tip:               "<< "\t" << conf.at("ip")<<endl;
        cout << "\t.crcFile:         "<< "\t" << conf.at("crcFile")<<endl;
        cout << "\tsubToSync:        "<< "\t" << conf.at("subToSync")<<endl;
    } else {
        cout << getCurrentTime() << "Canh bao: \t Duong dan khong ton tai !" << endl;
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
            cout << getCurrentTime() << "CrcRoutine running...." << endl;
            CRCRoutine* crcRoutine = new CRCRoutine(); 
            int crc_result = crcRoutine->crcRoutine(args->config);
            delete crcRoutine;
            crcSignal = false;
        }
    }
    std::cout << getCurrentTime() << "crcThread terminating..." << std::endl;
    pthread_exit(NULL);
}

void* clientCallFunction(void* arg){
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    while(!shouldTerminate){
        if(clientSignal){
            cout << getCurrentTime() << "Dong bo du lieu: ";
            try{
                Client client(args->config);
                client.synchronizeData();
            } catch(const std::exception& e){
                std::cerr << e.what() << std::endl;
            }
            clientSignal = false;
        }
    }
    std::cout << getCurrentTime() << "client call thread terminating..." << std::endl;
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
    std::cout << getCurrentTime() << "Server host thread terminating..." << std::endl;
    pthread_exit(NULL);
}
int main(int argc, char *argv[]) {
    // Register the signal handler
    signal(SIGUSR1, handleSignal);
    signal(SIGUSR2, handleSignal);
    bool is_continue = true;
    string continue_choice;
    string serverIP;
    fs::path scriptPath;
    MyServer* myServer;
    int role_choice;
    bool configProvided;
    // argparse
    argparse::ArgumentParser program("App");
    program.add_argument("--mode").default_value(string("1"));
    program.add_argument("--role").default_value(string("server"));
    program.add_argument("--config").default_value(string(""));

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::exit(1);
    }

    auto choice = stoi(program.get<string>("--mode"));
    if (choice == 1 or choice == 2 ){}
    else {
        cout << getCurrentTime() << "Chon sai che do: 1 - Cap nhat 1 chieu \t 2 - Cap nhat 2 chieu" <<endl;
        return 1;
    }
    auto role = program.get<string>("--role");
    if (choice == 1){
        if (role == "server" || role == "Server") role_choice = 1;
        else if (role == "client" || role == "Client") role_choice = 2;
        else {
            cout << getCurrentTime() << "Chon sai vai tro: 1 - server \t 2- client" << endl;
            return 1;
        }
    }
    configProvided = program.is_used("--config");
    if (configProvided){
        if (choice == 1){
            ///mode 1
            if (role_choice == SERVER_ROLE){
                // SERVER_ROLE config in mode 1
                if (isPathExists(SERVER_CONFIG)){
                    // Da ton tai file config
                    cout << getCurrentTime() << "Mode 1 - Server config: " << endl;
                    show_server_config(SERVER_CONFIG);
                    cout <<endl << "Chinh sua: " << endl;
                } else {
                    // chua ton tai file config
                    cout << getCurrentTime() << "Tao file config cho Server(mode 1)" << endl;
                }
                string folderpath;
                string crcFile;
                string port;
                do{
                    cout << "\tfolderPath: \t";
                    getline(std::cin, folderpath);
                    if(!folderpath.empty() &&!isFolderPath(folderpath)) continue;
                    else break;
                } while (!folderpath.empty());
                do{
                    cout << "\tPort: \t";
                    getline(std::cin, port);
                    if(!isInteger(port) && !port.empty()) continue;
                    else break;
                }while(!port.empty());
                do{
                    cout << "\t.crcFile: \t";
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
                        cout << getCurrentTime() << "Error: Loi file config...1" << endl;
                        return 1;
                    }
                    if(!port.empty()) existingJsonData["port"] = stoi(port);
                    else {
                        cout << getCurrentTime() << "Error: Loi file config...2" << endl;
                        return 1;
                    }
                    if(!crcFile.empty()) existingJsonData["crcFile"] = crcFile;
                    else {
                        cout << getCurrentTime() << "Error: Loi file config...3" << endl;
                        return 1;
                    }
                    std::ofstream outputFile(SERVER_CONFIG);
                    outputFile << existingJsonData;
                    outputFile.close();
                }
                cout << endl <<getCurrentTime() << "Config thanh cong !" << endl;
                
            } else {
                // CLIENT_ROLE config in mode 1
                if(isPathExists(CLIENT_CONFIG)){
                    // file con fig da ton tai
                    cout << getCurrentTime() <<"Mode 1 - Client config: " << endl;
                    show_client_config(CLIENT_CONFIG);
                    cout <<endl << "Chinh sua: " << endl;
                }else {
                    // file config chua ton tai
                    cout << getCurrentTime() << "Tao file config cho Client(mode 1)" << endl;
                }
                string folderpath;
                string crcFile;
                string port;
                string ip;
                string subToSync;
                do{
                    cout << "\tfolderPath: \t";
                    getline(std::cin, folderpath);
                    if(!folderpath.empty() &&!isFolderPath(folderpath)) continue;
                    else break;
                } while (!folderpath.empty());
                do{
                    cout << "\tPort: \t";
                    getline(std::cin, port);
                    if(!isInteger(port) && !port.empty()) continue;
                    else break;
                }while(!port.empty());
                do{
                    cout << "\tIP: \t";
                    getline(std::cin, ip);
                    if(!isIPAddress(ip) && !ip.empty()) continue;
                    else break;
                }while(!ip.empty());
                do{
                    cout << "\t.crcFile: \t";
                    getline(std::cin, crcFile);
                    if(!isValidPath(crcFile) && !crcFile.empty()) continue;
                    else break;
                } while (!crcFile.empty());
                do{
                    cout << "\tsubToSync: \t";
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
                        cout << getCurrentTime() <<  "Error: Loi file config...1" << endl;
                        cout << folderpath << endl;
                        return 1;
                    }
                    if(!port.empty()) existingJsonData["port"] = stoi(port);
                    else {
                        cout << getCurrentTime() <<  "Error: Loi file config...2" << endl;
                        return 1;
                    }
                    if(!ip.empty()) existingJsonData["ip"] = ip;
                    else{
                        cout << getCurrentTime() << "Error: Loi file config...3" << endl;
                        return 1;
                    }
                    if(!crcFile.empty()) existingJsonData["crcFile"] = crcFile;
                    else {
                        cout << getCurrentTime() << "Error: Loi file config...4" << endl;
                        return 1;
                    }
                    if(!subToSync.empty()) existingJsonData["subToSync"] = subToSync;
                    else {
                        cout << getCurrentTime() << "Error: Loi file config...5" << endl;
                        return 1;
                    }
                    std::ofstream outputFile(CLIENT_CONFIG);
                    outputFile << existingJsonData;
                    outputFile.close();
                }
                cout << endl <<getCurrentTime() << "Config thanh cong !" << endl;
            }
        } else {
            //mode 2
            if (isPathExists(SYNC_CONFIG)){
                // file con fig ton tai
                cout <<getCurrentTime() <<  "Mode 2 - config: " << endl;
                show_sync_config(SYNC_CONFIG);
                cout <<endl << "Chinh sua: " << endl;

            } else {
                // file config k ton tai
                cout << getCurrentTime() << "Tao file config cho mode 2" << endl;
            }
            string folderpath;
            string crcFile;
            string port;
            string port_host;
            string ip;
            string subToSync;
            do{
                cout << "\tfolderPath: \t";
                getline(std::cin, folderpath);
                if(!folderpath.empty() &&!isFolderPath(folderpath)) continue;
                else break;
            } while (!folderpath.empty());
            do{
                cout << "\tPort for calling: \t";
                getline(std::cin, port);
                if(!isInteger(port) && !port.empty()) continue;
                else break;
            }while(!port.empty());
            do{
                cout << "\tPort for hosting: \t";
                getline(std::cin, port_host);
                if(!isInteger(port_host) && !port_host.empty()) continue;
                else break;
            }while(!port_host.empty());
            do{
                cout << "\tIP: \t";
                getline(std::cin,ip);
                if(!isIPAddress(ip) && !ip.empty()) continue;
                else break;
            }while(!ip.empty());
            do{
                cout << "\t.crcFile: \t";
                getline(std::cin, crcFile);
                if(!isValidPath(crcFile) && !crcFile.empty()) continue;
                else break;
            } while (!crcFile.empty());
            do{
                cout << "\tsubToSync: \t";
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
                if(!port_host.empty()) existingJsonData.at("port_host") = stoi(port_host);
                if(!ip.empty()) existingJsonData.at("ip") = ip;
                if(!crcFile.empty()) existingJsonData.at("crcFile") = crcFile;
                if(!subToSync.empty()) existingJsonData.at("subToSync") = subToSync;
                std::ofstream outputFile(SYNC_CONFIG);
                outputFile << existingJsonData;
                outputFile.close();
            } else{
                if(!folderpath.empty()) existingJsonData["folderPath"] = folderpath;
                else {
                    cout << getCurrentTime() << "Error: Loi file config...1" << endl;
                    cout << folderpath << endl;
                    return 1;
                }
                if(!port.empty()) existingJsonData["port"] = stoi(port);
                else {
                    cout << getCurrentTime() << "Error: Loi file config...2" << endl;
                    return 1;
                }
                if(!port_host.empty()) existingJsonData["port_host"] = stoi(port_host);
                else {
                    cout << getCurrentTime() << "Error: Loi file config...2.1" << endl;
                    return 1;
                }
                if(!ip.empty()) existingJsonData["ip"] = ip;
                else{
                    cout << getCurrentTime() << "Error: Loi file config...3" << endl;
                    return 1;
                }
                if(!crcFile.empty()) existingJsonData["crcFile"] = crcFile;
                else {
                    cout << getCurrentTime() << "Error: Loi file config...4" << endl;
                    return 1;
                }
                if(!subToSync.empty()) existingJsonData["subToSync"] = subToSync;
                else {
                    cout << getCurrentTime() <<  "Error: Loi file config...5" << endl;
                    return 1;
                }
                std::ofstream outputFile(SYNC_CONFIG);
                outputFile << existingJsonData;
                outputFile.close();
            }
            cout << endl << getCurrentTime() << "Config thanh cong !" << endl;
        }
        return 0;
    }
    if ((choice == 1)&& (role_choice == SERVER_ROLE)){
        // process for server mode 1
        if(!isPathExists(SERVER_CONFIG)){
            cout << "file config khong ton tai" << endl;
            return 1;
        }
        ThreadArgs* args = new ThreadArgs;
        args->config = SERVER_CONFIG;
        pthread_t crcThread;
        // Tao crc thread
        if(pthread_create(&crcThread, NULL, crcRoutineFunction, args) != 0){
            std::cerr << getCurrentTime() << "Failed to create crcThread" << std::endl;
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
            std::cerr << getCurrentTime() << "Failed to create server host thread" << std::endl;
        }

        pthread_detach(crcThread);
        pthread_detach(serverHostThread);

        while (true){
            char userInput;
            sleep(1);
            cout <<getCurrentTime() << "Nhap yeu cau: \t";
            cin >> userInput;

            if (userInput == 'U'){
                pthread_kill(crcThread, SIGUSR1);
            }
            if (userInput == 'Q'){
                serverOffline = true;
                shouldTerminate = true;
                sleep(3);
                cout << getCurrentTime()<<"Phien lam viec ket thuc" << endl;
                break;
            }
        }
        delete args;
        delete server_args;
    }
    else if (choice == 1 && role_choice == CLIENT_ROLE){
        // process for client mode 1
        if (!isPathExists(CLIENT_CONFIG)){
            cout << getCurrentTime() << "File config khong ton tai" << endl;
            return 1;
        }
        CRCRoutine* crcRoutine = new CRCRoutine(); 
        int crc_result = crcRoutine->crcRoutine(CLIENT_CONFIG);
        delete crcRoutine;
        cout << getCurrentTime() << "Dong bo du lieu: ";
        try{
            Client client(CLIENT_CONFIG);
            client.synchronizeData();
        } catch(const std::exception& e){
            std::cerr << e.what() << std::endl;
        }
        cout << getCurrentTime() << "Phien lam viec ket thuc" << endl;
    } else {
        // crc function
        if(!isPathExists(SYNC_CONFIG)){
            cout << getCurrentTime() << "file config khong ton tai" << endl;
            return 1;
        }
        ThreadArgs* args = new ThreadArgs;
        args->config = SYNC_CONFIG;
        pthread_t crcThread;
        if(pthread_create(&crcThread, NULL, crcRoutineFunction,args)!=0){
            std::cerr << getCurrentTime() << "Failed to create crcThread" << std::endl;
        }

        // call sync 
        pthread_t callThread;
        if(pthread_create(&callThread, NULL, clientCallFunction, args) != 0){
            std::cerr << getCurrentTime() << "Failed to create call thread" << std::endl;
        }
        // constructor host
        json conf;
        std::ifstream inputFile(SYNC_CONFIG);
        inputFile >> conf;
        inputFile.close();
        string folderPath = conf.at("folderPath");
        string crcFile_ = conf.at("crcFile");
        int port_ = conf.at("port_host");
        ServerThreadArgs *host_args = new ServerThreadArgs;
        host_args->folderPath = folderPath;
        host_args->crcFile = crcFile_;
        host_args->port = port_;
        pthread_t hostThread;
        if (pthread_create(&hostThread, NULL, serverHostFunction, host_args) != 0){
            std::cerr << getCurrentTime() << "Failed to create host thread" << std::endl;
        }
        pthread_detach(callThread);
        pthread_detach(crcThread);
        pthread_detach(hostThread);
        while (true) {
            char userInput;
            sleep(1);
            cout <<getCurrentTime() << "Nhap yeu cau: \t";
            cin >> userInput;

            if (userInput == 'U') {
                // Send signal to crcThread
                pthread_kill(crcThread, SIGUSR1);
            }

            if (userInput == 'S') {
                pthread_kill(callThread, SIGUSR2);
            }
            if(userInput == 'Q') {
                shouldTerminate = true;
                sleep(3);
                cout << getCurrentTime() << getCurrentTime() << "Phien lam viec ket thuc" << endl;
                break;
            }
        }
        delete args;
        delete host_args;
    }

    reset();
    return 0;
}