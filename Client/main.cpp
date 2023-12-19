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

// Khai báo biến
string folderPath = "";
string crcFile = "";
string ip = "";
int port = 0;

//  Thực hiện 1 HTTP Get request đến 1 địa chỉ cụ thể và trả về dữ liệu dưới dạng json
json downloadCRCS(){
    Header headers{{"TokenKey", "aaa"}}; // Định nghĩa headers của http
    string url = "http://" + ip + ":" + to_string(port) + "/ListOfAll"; // Định nghĩa biến url
    Response r = Get(Url{url}, headers); // sử dụng method Get để thực hiện HTTP request
    std::string res = r.text; // Dữ liệu nhận được lấy từ thuộc tính text của đối tượng Response
    json res_json = json::parse(res); // Chuyển dữ liệu sang dạng json
    return res_json;
}

json scanFolder(){
    json filedict = {}; // Khảo tạo biến json
    for (const auto& entry: recursive_directory_iterator(folderPath)){
        if (entry.path().filename() == crcFile){ // Kiểm tra tên tệp có bằng tên biến crcFile không
        // Dữ liệu đọc được sau đó được chuyển đổi thành một đối tượng JSON và lưu vào biến filedict.
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
            try{ // kiểm tra xem có key và value tuogw ứng trong minorJson không
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

// Tải 1 tệp xuống từ máy chủ
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
    // Lấy Đường dẫn Tuyệt đối của Script và Mở Tệp Tin Cấu hình
    path scriptPath = filesystem::absolute(filesystem::path(__FILE__));
    path scriptLocation = scriptPath.parent_path();
    string script_location = scriptLocation.string();
    string config_json = script_location + "/config.json";
    ifstream configFile(config_json, ifstream::in);
    // Đọc Dữ liệu Cấu hình từ Tệp Tin
    /*Nếu không thể mở tệp tin cấu hình, in thông báo lỗi và thoát với mã lỗi 1.
    Đọc nội dung của tệp tin cấu hình vào một đối tượng JSON (conf).*/
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

    //Tải và Đồng bộ hóa Dữ liệu
    /*
    Bước 1: Tải danh sách CRCs từ máy chủ bằng cách sử dụng hàm downloadCRCS.
    Bước 2: Quét thư mục và tạo danh sách CRCs của máy khách bằng cách sử dụng hàm scanFolder.  
    */
    json serverCrcs = downloadCRCS();
    json clientCrcs = scanFolder();
    serverCrcs = dictReduce(serverCrcs, clientCrcs);
    
    /*
    Lọc và Tải về Tệp tin từ Máy Chủ:
        Lọc danh sách CRCs của máy chủ để chỉ giữ lại những CRCs có tên bắt đầu bằng chuỗi subToSync.
        Duyệt qua danh sách lọc và thực hiện việc tải về từng tệp tin từ máy chủ và lưu vào thư mục cục bộ.
        Hiển thị trạng thái của quá trình tải về.
    */
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
    // Vòng lặp để tải về và lưu trữ từng tệp tin
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
