#pragma once

#include "include/out/httplib.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
std::string pathJoin(const std::string& a, const std::string& b);

std::string urlDecode(const std::string& str);

json parse_query_string(const std::string& query);
class MyServer {
private:
    httplib::Server server;
    std::string folderPath;
    std::string crcFile;

    bool is_valid_token(const std::string& token);

    std::string scan_folder();

    bool file_exist(const std::string& filepath);

    std::string get_file_bytes(const std::string& filepath) const;

public:
    MyServer(const std::string& _folderPath = "", const std::string& _crcFile = "");

    void handle_request(const httplib::Request& req, httplib::Response& res);

    void run_server(int port);
};
