#ifndef CRC_ROUTINE_H
#define CRC_ROUTINE_H

#include <string>
#include <filesystem>
#include <zlib.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

class CRCRoutine {
public:
    CRCRoutine();
    ~CRCRoutine();
    void crcIter(const std::string& path);
    void crcOfFile(const std::string& absFile);
    std::string crc(const std::string& fileName);
    int crcRoutine(const std::string scriptPath);

private:
    std::string folderPath;
    std::string crcFile;
};

#endif
