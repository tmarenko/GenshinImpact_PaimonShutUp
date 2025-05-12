#pragma once

#include <fstream>
#include <map>
#include <sstream>
#include <codecvt>

std::map<std::string, std::string> ParseConfig(const std::string &fileName);
std::wstring convertStringToWstring(std::string &input);
