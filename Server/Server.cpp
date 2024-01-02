#include "include/out/httplib.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <locale>
#include <codecvt>

using namespace std;
using namespace httplib;
using json = nlohmann::json;
namespace fs = std::filesystem;

string pathJoin(const std::string& a, const std::string& b) {
    if (a.empty()) {
        return b;
    }

    if (a.back() == '/' || a.back() == '\\') {
        return a + b;
    } else {
        return a + '/' + b;
    }
}

string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            // Nếu gặp ký tự '%', giải mã hai ký tự tiếp theo và thêm vào chuỗi giải mã
            if (i + 2 < str.size()) {
                int hexValue;
                std::istringstream(str.substr(i + 1, 2)) >> std::hex >> hexValue;
                result += static_cast<char>(hexValue);
                i += 2;  // Bỏ qua hai ký tự đã giải mã
            }
            // Nếu không đủ ký tự để giải mã, giữ nguyên '%'
            else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            // Thay thế ký tự '+' bằng dấu cách
            result += ' ';
        } else {
            // Giữ nguyên các ký tự khác
            result += str[i];
        }
    }

    return result;
}


json parse_query_string(const std::string &query) {
    json params;

    std::istringstream iss(query);
    for (std::string token; std::getline(iss, token, '&');) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);
            params[key] = value;
        }
    }

    return params;
}

class MyServer{
    private:
        httplib::Server server;
        string folderPath;
        string crcFile;

        bool is_valid_token(const string &token){
            static const vector<string> token_keys = {"aaa", "bbb"};
            return find(token_keys.begin(), token_keys.end(), token) != token_keys.end(); 
        }

        string scan_folder() {
            json filedict;
            for (const auto &entry : fs::recursive_directory_iterator(folderPath)) {
                if (entry.path().filename() == crcFile) {
                    ifstream file(entry.path().string());
                    if (file.is_open()) {
                        try {
                            json crcs;
                            file >> crcs;
                            filedict.merge_patch(crcs);
                        } catch (const json::parse_error &e) {
                            cerr << "Error parsing JSON file: " << entry.path().string() << " - " << e.what() << endl;
                        }
                        file.close();
                    }
                }
            }
            return filedict.dump();
        }

        bool file_exist(const string& filepath){
            fs::path path = fs::path(folderPath) / filepath;
            return fs::exists(path) && fs::is_regular_file(path);
        }

        std::string get_file_bytes(const std::string& filepath) const {
            fs::path path = fs::path(folderPath) / filepath;
            try {
                std::ifstream in_file(path, std::ios::binary);
                std::string data((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
                return data;
            } catch (const std::exception& e) {
                return "";
            }
        }
    public:
        MyServer(string _folderPath = "", string _crcFile = ""){
            this->folderPath = _folderPath;
            this->crcFile = _crcFile;
        }
        void handle_request(const Request &req, Response &res){
            string token_key = req.get_header_value("TokenKey");
            if (is_valid_token(token_key)){
                string env = detail::decode_url(req.path, false);
                if (env == "/ListOfAll"){
                    string body = scan_folder();
                    res.set_content(body, "text/html");
                } else if (env.compare(0, 6, "/File") == 0){
                    auto query_components = parse_query_string(req.target);
                    if(query_components.find("/File?FilePath") != query_components.end()){
                        string filepath = query_components.at("/File?FilePath");
                        filepath = urlDecode(filepath);
                        filepath = pathJoin(folderPath, filepath);
                        cout << filepath << endl;
                        if(file_exist(filepath)){
                            res.status = 200;
                            std::string body = get_file_bytes(filepath);
                            res.set_content(body, "application/octet-stream");
                        } else {
                            res.status = 500;
                            res.set_content("File not found", "text/plain");
                        }
                    } else {
                        res.status = 500;
                        res.set_content("Invalid request", "text/plain");
                    }
                } else {
                    res.status = 501;
                    res.set_content("Not implemented", "text/plain");
                }
            } else {
                res.status = 502;
                res.set_content("Invalid token", "text/plain");
            }
        }

        void run_server(int port) {
            server.Get("/.*", [&](const httplib::Request& req, httplib::Response& res) {
                handle_request(req, res);
            });

            std::cout << "Server Starts - 0.0.0.0:" << port << std::endl;
            server.listen("0.0.0.0", port);
        }
};

int main(){
    fs::path scriptPath = filesystem::absolute(filesystem::path(__FILE__));
    fs::path scriptLocation = scriptPath.parent_path();
    string script_location = scriptLocation.string();
    string config_json = script_location + "/config.json";
    ifstream configFile(config_json, ifstream::in);
    if (!configFile.is_open()) {
        cerr << "Error: Unable to open config file." << endl;
        return 1;
    }

    json conf;
    configFile >> conf;
    string folderPath = conf.at("folderPath");
    string crcFile = conf.at("crcFile");
    int port = conf.at("port");
    try{
        MyServer server(folderPath, crcFile);
        server.run_server(port);
    } catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}