#include "hashRoutine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

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
    } catch (...) {
        // Ignore errors for now
    }
    return j;
}

void HashRoutine::writeJsonFile(const std::string& filePath, const json& value) {
    std::ofstream fileOut(filePath);
    fileOut << std::setw(4) << value << std::endl;
}

std::string HashRoutine::calculateHash(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        // Handle file opening error if needed
        return "";
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&sha256, buffer, sizeof(buffer));
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream hashStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hashStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return hashStream.str();
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
