#include "Server.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;
std::string pathJoin(const std::string& a, const std::string& b) {
    if (a.empty()) {
        return b;
    }

    if (a.back() == '/' || a.back() == '\\') {
        return a + b;
    } else {
        return a + '/' + b;
    }
}

std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.size()) {
                int hexValue;
                std::istringstream(str.substr(i + 1, 2)) >> std::hex >> hexValue;
                result += static_cast<char>(hexValue);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

json parse_query_string(const std::string& query) {
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
MyServer::MyServer(const std::string& _folderPath, const std::string& _crcFile) : folderPath(_folderPath), crcFile(_crcFile) {}

bool MyServer::is_valid_token(const std::string& token) {
    static const std::vector<std::string> token_keys = {"aaa", "bbb"};
    return std::find(token_keys.begin(), token_keys.end(), token) != token_keys.end();
}

std::string MyServer::scan_folder() {
    json filedict;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (entry.path().filename() == crcFile) {
            std::ifstream file(entry.path().string());
            if (file.is_open()) {
                try {
                    json crcs;
                    file >> crcs;
                    filedict.merge_patch(crcs);
                } catch (const json::parse_error& e) {
                    std::cerr << "Error parsing JSON file: " << entry.path().string() << " - " << e.what() << std::endl;
                }
                file.close();
            }
        }
    }
    return filedict.dump();
}

bool MyServer::file_exist(const std::string& filepath) {
    fs::path path = fs::path(folderPath) / filepath;
    return fs::exists(path) && fs::is_regular_file(path);
}

std::string MyServer::get_file_bytes(const std::string& filepath) const {
    fs::path path = fs::path(folderPath) / filepath;
    try {
        std::ifstream in_file(path, std::ios::binary);
        std::string data((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
        return data;
    } catch (const std::exception& e) {
        return "";
    }
}

void MyServer::handle_request(const httplib::Request& req, httplib::Response& res) {
    std::string token_key = req.get_header_value("TokenKey");
    if (is_valid_token(token_key)) {
        std::string env = httplib::detail::decode_url(req.path, false);
        if (env == "/ListOfAll") {
            std::string body = scan_folder();
            res.set_content(body, "text/html");
        } else if (env.compare(0, 6, "/File") == 0) {
            auto query_components = parse_query_string(req.target);
            if (query_components.find("/File?FilePath") != query_components.end()) {
                std::string filepath = query_components.at("/File?FilePath");
                filepath = urlDecode(filepath);
                filepath = pathJoin(folderPath, filepath);
                std::cout << filepath << std::endl;
                if (file_exist(filepath)) {
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

void MyServer::run_server(int port) {
    server.Get("/.*", [&](const httplib::Request& req, httplib::Response& res) {
        handle_request(req, res);
    });

    std::cout << "Server Starts - 0.0.0.0:" << port << std::endl;
    server.listen("0.0.0.0", port);
}
