#include "config.h"


std::map<std::string, std::string> ParseConfig(const std::string &fileName) {
    std::map<std::string, std::string> configMap = {
            {"language",    "eng"},
            {"genshin_eng", "Genshin Impact"},
            {"paimon_eng",  "Paimon"}
    };
    std::ifstream file(fileName);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    configMap[key] = value;
                }
            }
        }
    }
    return configMap;
}

std::wstring convertStringToWstring(std::string &input) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}
