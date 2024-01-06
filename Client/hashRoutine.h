#ifndef HASH_ROUTINE_H
#define HASH_ROUTINE_H

#include <string>
#include <filesystem>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

class HashRoutine {
public:
    HashRoutine();
    ~HashRoutine();

    void removeFileIfExists(const std::string& filePath);
    json readJsonFile(const std::string& filePath);
    void writeJsonFile(const std::string& filePath, const json& value);
    std::string calculateHash(const std::string& fileName);
    void hashIter(const std::string& path);
    void hashOfFile(const std::string& absFile);
    int hashRoutine(const std::string& configJson);

private:
    std::string folderPath;
    std::string hashFile;
};

#endif // HASH_ROUTINE_H
