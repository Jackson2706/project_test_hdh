#include <string>
#include <cpr/cpr.h>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <nlohmann/json.hpp>
using namespace std::filesystem;
using namespace std;
using namespace cpr;
using json = nlohmann::json;

string folderPath = "";
string crcFile = "";
string ip = "";
int port = 0;

json downloadCRCS(){
    Header headers{{"TokenKey", "aaa"}};
    string url = "http://" + ip + ":" + to_string(port) + "/ListOfAll";
    Response r = Get(Url{url}, headers);
    return r.text;
}

json scanFolder(){
    json filedict = {};
    for (const auto& entry: recursive_directory_iterator(folderPath)){
        if (entry.path().filename() == crcFile){
            ifstream crcFile(entry.path().string(), ifstream::in);
            crcFile >> filedict;
        }
    }
    return filedict;
}

json dictReduce( json& masterJson,  json& minorJson) {
    for(auto& element : minorJson.items()){
        string key = element.key();
        if (masterJson.find(key) != masterJson.end()) {
            std::string minorValueAsString = element.value().dump();
            std::string masterValueAsString = masterJson[key].dump();
            std::cout << key << "\t" << masterValueAsString << minorValueAsString <<endl;
        }

    }
    return masterJson;
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
    ip = conf.at("ip");
    port = conf.at("port");

    json serverCrcs = downloadCRCS();
    json clientCrcs = scanFolder();
    serverCrcs = dictReduce(serverCrcs, clientCrcs);
    return 0;
}
