#include "hashRoutine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <crypto++/sha.h>

HashRoutine::HashRoutine() {
}

HashRoutine::~HashRoutine() {
    // Destructor logic
}

void HashRoutine::removeFileIfExists(const std::string& filePath) {
    if (fs::exists(filePath)) {
        try {
            fs::remove(filePath);
        } catch (...) {
            // Handle removal error if needed
        }
    }
}

json HashRoutine::readJsonFile(const std::string& filePath) {
    json j;
    try {
        std::ifstream fileIn(filePath);
        fileIn >> j;
        fileIn.close();
    } catch (const std::exception& e) {
    }
    return j;
}

void HashRoutine::writeJsonFile(const std::string& filePath, const json& value) {
    std::ofstream fileOut(filePath);
    fileOut << std::setw(4) << value << std::endl;
}

std::string HashRoutine::calculateHash(const std::string& fileName) {
    std::ifstream f(fileName);
    if (!f.is_open()) {
        std::cout << "Không thể mở file." << std::endl;
        return "";
    }

    // Khởi tạo hàm hash
    CryptoPP::SHA256 hash;

    // Đọc nội dung file
    char buffer[1024];
    while (f.read(buffer, sizeof(buffer))) {
        hash.Update(reinterpret_cast<const CryptoPP::byte*>(buffer), f.gcount());
    }

    // Tính toán hash
    CryptoPP::byte hash_digest[CryptoPP::SHA256::DIGESTSIZE];
    hash.Final(hash_digest);
    std::string result = "";
    std::stringstream hashStream;
    hashStream << std::hex << std::setw(2) << std::setfill('0');
    for (int i = 0; i < CryptoPP::SHA256::DIGESTSIZE; i++) {
        hashStream << (int)hash_digest[i];
    }

    // Lưu giá trị hash vào std::string
    std::string hashValue = hashStream.str();
    return hashValue;
}
void HashRoutine::hashIter(const std::string& path) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().filename() == hashFile) {
                removeFileIfExists(entry.path().string());
            }
        }

        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                hashOfFile(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

void HashRoutine::hashOfFile(const std::string& absFile) {
    std::string hashPath = folderPath + "/" + hashFile;
    json hashes = readJsonFile(hashPath);
    hashes[absFile.substr(folderPath.length())] = calculateHash(absFile);
    
    writeJsonFile(hashPath, hashes);
}

int HashRoutine::hashRoutine(const std::string& configJson){
    std::ifstream configFile(configJson);
    if (!configFile.is_open()) {
        std::cerr << "Error: Unable to open config file." << std::endl;
        std::exit(1);
    }

    json conf;
    configFile >> conf;
    folderPath = conf.at("folderPath").get<std::string>();
    hashFile = conf.at("hashFile").get<std::string>();
    hashIter(folderPath);
    return 0;
}
