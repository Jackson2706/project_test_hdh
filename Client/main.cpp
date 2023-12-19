#include <string>
#include <cpr/cpr.h> // tải
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp> // tải
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
    std::string res = r.text;
    json res_json = json::parse(res);
    return res_json;
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
    for (json::iterator it = masterJson.begin(); it != masterJson.end(); ++it) {
            // Lấy key và value
            std::string key = it.key();
            std::string value = it.value();
            try{
                if (minorJson.at(key)  == value) {
                // Erase the element and get the next iterator
                    it = masterJson.erase(it);
                }
            } catch(const std::exception& e){
                // std::cerr << "Caught exception: " << e.what() << std::endl;
            }
    }
    return masterJson;
}

string downloadFile(const string &filepath, const string& ip, int port){
    Parameters parameters{{"FilePath", filepath}};
    Header headers{{"TokenKey", "aaa"}};

    auto url = Url{"http://"+ ip+ ":" + to_string(port)+ "/File"};
    auto response = Get(url, headers, parameters);
    if (response.error){
        std::cerr << "Error downloading file: " << response.error.message << std::endl;
        return "";
    }

    return response.text;
}

bool writeDatatoFile(const string& absPath, const string& data){
    ofstream file(absPath);
    if(file.is_open()){
        file << data;
        file.close();
        return true;
    }
    return false;
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
    string subToSync = conf.at("subToSync");
    json serverCrcs = downloadCRCS();
    json clientCrcs = scanFolder();
    serverCrcs = dictReduce(serverCrcs, clientCrcs);
    
    std::vector<std::string> serverCrcsFilteredKeys;

    for (const auto& entry : serverCrcs.items()) {
        const std::string& key = entry.key();
        if(key.find(subToSync) == 0){
            serverCrcsFilteredKeys.push_back(key);
        }
    }
    
    int i = 0;
    string absPath;
    string data;
    bool write_status;
    for (const auto& k: serverCrcsFilteredKeys){
        i++;
        absPath = folderPath +"/" + k;
        cout << "[" << i << "] Start download of: " << k; 
        data = downloadFile(k, ip,port);
        write_status = writeDatatoFile(absPath, data);

        if (write_status){
            cout << " - Status: Success" << endl;
        } else {
            cout << " - Status: Fail" << endl;
        }

        cout << "[" << i << "] End download of: " << k << endl;

    }
    return 0;
}