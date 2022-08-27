#ifndef GENSHINIMPACT_PAIMONSHUTUP_CONFIG_H
#define GENSHINIMPACT_PAIMONSHUTUP_CONFIG_H

#include <fstream>
#include <sstream>
#include <map>
#include <codecvt>

std::map<std::string, std::string> ParseConfig(const std::string &fileName);
std::wstring convertStringToWstring(std::string &input);

#endif //GENSHINIMPACT_PAIMONSHUTUP_CONFIG_H
