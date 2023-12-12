#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <cstdint>
#include <string>

using namespace std;
using namespace std::filesystem;
using json = nlohmann::json;


// string crcFile = "";
// string folderPath = "";

void crcIter(const string& path);
void crcOfFile(const string& absFile);
string crc(const string& fileName);


void crcIter(const string& path) {
    try {
        for (const auto& entry : recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().filename() == crcFile) {
                try {
                    remove(entry.path());
                } catch (...) {
                    // Handle removal error if needed
                }
            }
        }

        for (const auto& entry : recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                crcOfFile(entry.path().string());
            }
        }
    } catch (const filesystem_error& ex) {
        // Handle filesystem error if needed
        cerr << "Error: " << ex.what() << endl;
    }
}

void crcOfFile(const string& absFile) {
    string crcPath = folderPath + "/" + crcFile;
    json crcs;

    try {
        
        ifstream crcFileIn(crcPath);
        crcFileIn >> crcs;
    } catch (...) {
        // Ignore errors for now
    }

    crcs[absFile.substr(folderPath.length())] = crc(absFile);

    ofstream crcFileOut(crcPath);
    crcFileOut << crcs;
}

string crc(const string& fileName) {
    ifstream file(fileName, ios::binary);
    if (!file.is_open()) {
        // Handle file opening error if needed
        return "";
    }

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    unsigned long crcValue = crc32(0L, Z_NULL, 0);
    crcValue = crc32(crcValue, reinterpret_cast<const Bytef*>(content.data()), content.size());

    stringstream ss;
    ss << hex << crcValue;
    return ss.str();
}
int main(){

    path scriptPath = filesystem::absolute(filesystem::path(__FILE__));
    path scriptLocation = scriptPath.parent_path();
    string script_location = scriptLocation.string();
    string config_json = script_location + "/config.json";
    ifstream configFile(config_json, ifstream::in);
    if (!configFile.is_open()) {
        cerr << "Error: Unable to open config file." << endl;
        return 1;
    }

    json conf;
    configFile >> conf;
    folderPath = conf.at("folderPath");
    crcFile = conf.at("crcFile");
    
    crcIter(folderPath);

    return 0;
}