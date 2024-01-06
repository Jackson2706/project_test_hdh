#include "Client.h"

Client::Client(const std::string& configPath) {
    std::ifstream configFile(configPath, std::ifstream::in);
    if (!configFile.is_open()) {
        std::cerr << "Error: Unable to open config file." << std::endl;
        // Handle the error or throw an exception if needed
    }

    json conf;
    configFile >> conf;
    folderPath = conf.at("folderPath");
    hashFile = conf.at("hashFile");
    ip = conf.at("ip");
    port = conf.at("port");
    subToSync = conf.at("subToSync");

}

json Client::downloadHASHS() {
    cpr::Header headers{{"TokenKey", "aaa"}};
    std::string url = "http://" + ip + ":" + std::to_string(port) + "/ListOfAll";
    cpr::Response r = cpr::Get(cpr::Url{url}, headers);
    std::string res = r.text;
    return json::parse(res);
}

json Client::scanFolder() {
    json filedict = {};
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (entry.path().filename() == hashFile) {
            std::ifstream hashFile(entry.path().string(), std::ifstream::in);
            hashFile >> filedict;
        }
    }
    return filedict;
}

json Client::dictReduce(json& masterJson, json& minorJson) {
    for (json::iterator it = masterJson.begin(); it != masterJson.end(); ++it) {
        std::string key = it.key();
        std::string value = it.value();
        try {
            if (minorJson.at(key) == value) {
                it = masterJson.erase(it);
            }
        } catch (const std::exception& e) {
            // std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    }
    return masterJson;
}

std::string Client::downloadFile(const std::string& filepath) {
    cpr::Parameters parameters{{"FilePath", filepath}};
    cpr::Header headers{{"TokenKey", "aaa"}};

    auto url = cpr::Url{"http://" + ip + ":" + std::to_string(port) + "/File"};
    auto response = cpr::Get(url, headers, parameters);
    if (response.error) {
        std::cerr << "Error downloading file: " << response.error.message << std::endl;
        return "";
    }

    return response.text;
}

bool Client::writeDatatoFile(const std::string& absPath, const std::string& data) {
    std::ofstream file(absPath);
    if (file.is_open()) {
        file << data;
        file.close();
        return true;
    }
    return false;
}

void Client::synchronizeData() {
    json serverHashs = downloadHASHS();
    json clientHashs = scanFolder();
    serverHashs = dictReduce(serverHashs, clientHashs);

    std::vector<std::string> serverHashsFilteredKeys;

    for (const auto& entry : serverHashs.items()) {
        const std::string& key = entry.key();
        if (key.find(subToSync) == 0) {
            serverHashsFilteredKeys.push_back(key);
        }
    }
    std::cout << "Pass 1" << std::endl;
    int i = 0;
    std::string absPath;
    std::string data;
    bool write_status;

    for (const auto& k : serverHashsFilteredKeys) {
        i++;
        absPath = folderPath + "/" + k;
        std::cout << "[" << i << "] Start download of: " << k;
        data = downloadFile(k);
        write_status = writeDatatoFile(absPath, data);

        if (write_status) {
            std::cout << " - Status: Success" << std::endl;
        } else {
            std::cout << " - Status: Fail" << std::endl;
        }
    }
}

