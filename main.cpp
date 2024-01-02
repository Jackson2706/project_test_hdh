#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/asio.hpp>
#include <fstream>
#include "Server/crcRoutine.h"
#include "Server/Server.h"

using namespace boost::asio;
using namespace std;

const int SERVER_ROLE = 1;
const int CLIENT_ROLE = 2;
const string WORKSAPCE = "./";
const string SERVER_CONFIG = WORKSAPCE + "server_config.json";
const string CLIENT_CONFIG = WORKSAPCE + "client_config.json";
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

int main(){
    bool is_continue = true;
    int choice;
    int role_choice;
    string continue_choice;
    string serverIP;
    fs::path scriptPath;
    CRCRoutine* crcRoutine;
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
                 // CRC function running
                crcRoutine = new CRCRoutine(); 
                int crc_result = crcRoutine->crcRoutine(SERVER_CONFIG);
                delete crcRoutine;

                // Contruct server
                json conf;
                std::ifstream inputFile(SERVER_CONFIG);
                inputFile >> conf;
                inputFile.close();
                string folderPath = conf.at("folderPath");
                string crcFile = conf.at("crcFile");
                int port = conf.at("port");
                try{
                    MyServer server(folderPath, crcFile);
                    server.run_server(port);
                } catch (const std::exception& e){
                    std::cerr << e.what() << std::endl;
                }


            } else if (role_choice == CLIENT_ROLE){
                cout << "Your choice: [2] - Client" << endl;
                // Nhap dia chi IP cua Server
                cout << "Nhap dia chi IP cua Server: ";
                cin >> serverIP;

                // Kiem tra dia chi IP co hop le hay khong
                if (isIPAddress(serverIP)){
                    cout << "IP: " << serverIP << endl;
                } else {
                    cout << "Dia chi IP khong hop le" << endl;
                }
            } else {
                cout << "Lua chon khong dung !" << endl;

            }

            
            
        }


        /// TODO: Process if choice == 2 ( xu ly 2 chieu)
        if (choice == 2){

        }


        cout << "Ban muon tiep tuc? [y/N]? ";
        cin >> continue_choice;
        if (continue_choice == "y") {
            system("clear");
            continue;
        } else is_continue = false; 

    }while(is_continue);
    return 0;
}