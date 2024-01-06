#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <cpr/cpr.h>
using json = nlohmann::json;
namespace fs = std::filesystem;

class Client {
private:
    std::string folderPath;
    std::string hashFile;
    std::string ip;
    int port;
    std::string subToSync;

    json downloadHASHS();
    json scanFolder();
    json dictReduce(json& masterJson, json& minorJson);
    std::string downloadFile(const std::string& filepath);
    bool writeDatatoFile(const std::string& absPath, const std::string& data);

public:
    Client(const std::string& configPath);
    void synchronizeData();
};
