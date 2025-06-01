#ifndef HELPER_UTILS_H
#define HELPER_UTILS_H
#include <string>
#include <vector>
#include <filesystem>
enum CommandType{ CMD_ECHO, CMD_TYPE, CMD_EXIT, CMD_UNKNOWN };

// remove the extra spaces in the input at start of the string
std::string removeExtraSpaces(std::string s);

// get command type for the switch case
CommandType getCommandType(const std::string& input, std::string& arg);

// helper function to split a string by delimiter
std::vector<std::string> split(const std::string& s, char delim);

// check file is executable or not 
bool isExcutable(const std::filesystem::path& path);

// search command in path directories
std::string findInPath(const std::string& cmd);

#endif

