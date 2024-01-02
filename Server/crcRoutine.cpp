#include "crcRoutine.h"
#include <iostream>
#include <fstream>
#include <sstream>

CRCRoutine::CRCRoutine() {
}
CRCRoutine::~CRCRoutine() {
    // Destructor logic
}
void CRCRoutine::crcIter(const std::string& path) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().filename() == crcFile) {
                try {
                    fs::remove(entry.path());
                } catch (...) {
                    // Handle removal error if needed
                }
            }
        }

        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                crcOfFile(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& ex) {
        // Handle filesystem error if needed
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

void CRCRoutine::crcOfFile(const std::string& absFile) {
    std::string crcPath = folderPath + "/" + crcFile;
    json crcs;

    try {
        std::ifstream crcFileIn(crcPath);
        crcFileIn >> crcs;
    } catch (...) {
        // Ignore errors for now
    }

    crcs[absFile.substr(folderPath.length())] = crc(absFile);

    std::ofstream crcFileOut(crcPath);
    crcFileOut << crcs;
}

std::string CRCRoutine::crc(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        // Handle file opening error if needed
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    unsigned long crcValue = crc32(0L, Z_NULL, 0);
    crcValue = crc32(crcValue, reinterpret_cast<const Bytef*>(content.data()), content.size());

    std::stringstream ss;
    ss << std::hex << crcValue;
    return ss.str();
}

int CRCRoutine::crcRoutine(const std::string config_json) {
    std::ifstream configFile(config_json);
    if (!configFile.is_open()) {
        std::cerr << "Error: Unable to open config file." << std::endl;
        return 1;
    }
    json conf;
    configFile >> conf;
    folderPath = conf.at("folderPath");
    crcFile = conf.at("crcFile");
    
    crcIter(folderPath);

    return 0;
}
