#include <iostream>
#include "helper_utils.h"
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>
#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>

std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

std::string removeExtraSpaces(std::string s) {
    size_t start = s.find_first_not_of(" \t");
    return (start == std::string::npos) ? "" : s.substr(start);
}

CommandType getCommandType(const std::string& input, std::string& arg) {
    std::string trimmed = removeExtraSpaces(input);
    if (trimmed == "exit 0") {
        arg = "";
        return CMD_EXIT;
    }

    if (trimmed.rfind("echo ", 0) == 0) {
        arg = trimmed.substr(5); 
        return CMD_ECHO;
    }

    if (trimmed.rfind("type ", 0) == 0) {
        arg = removeExtraSpaces(trimmed.substr(5));
        return CMD_TYPE;
    }

    if(trimmed == "pwd") {
        arg = "";
        return CMD_PWD;
    }

    if(trimmed.rfind("cd ", 0) == 0) {
        arg = removeExtraSpaces(trimmed.substr(3));
        return CMD_CD;
    }

    if(trimmed == "cd") {
        arg = "";
        return CMD_CD;
    }

    if(trimmed.rfind("cat ", 0) == 0) {
        arg = removeExtraSpaces(trimmed.substr(4));
        return CMD_CAT;
    }

    if(trimmed.rfind("ls ", 0) == 0) {
        arg = removeExtraSpaces(trimmed.substr(3));
        return CMD_LS;
    }

    if(trimmed.rfind("wc ", 0) == 0) {
        arg = removeExtraSpaces(trimmed.substr(3));
        return CMD_WC;
    }

    return CMD_UNKNOWN;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> directories;  // store the different directories  
    std::stringstream ss(s);
    std::string item;
    
    while(getline(ss, item, delim)) {
        item = removeExtraSpaces(item); 
        directories.push_back(item);
    }
    return directories;
}

bool isExcutable (const std::filesystem::path& path) {
    std::error_code error; 
    if(!std::filesystem::exists(path, error)) return false;
    if(!std::filesystem::is_regular_file(path, error)) return false;  
    return true;
}

std::string findInPath(const std::string& cmd) {
    const char* path_env = getenv("PATH");
    if(!path_env) return "";

    std::string path_str(path_env); 
    std::vector<std::string> dirs = split(path_str, ':');

    for(const std::string& dir : dirs) {
        std::filesystem::path full_path = std::filesystem::path(dir) / cmd;
        if(isExcutable(full_path)) {
            return full_path.string();
        }
    }

    return "";
}

std::vector<std::string> splitPath(const std::string& pathEnv) {
    std::vector<std::string> paths;
    std::stringstream ss(pathEnv);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        paths.push_back(dir);
    }
    return paths;
}

bool isExecutable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0;
}


std::optional<std::string> findExecutableMatch(const std::string& partialCmd) {
    std::string pathEnv = getenv("PATH");
    auto dirs = splitPath(pathEnv);

    for (const auto& dir : dirs) {
        DIR* dp = opendir(dir.c_str());
        if (!dp) continue;

        struct dirent* entry;
        while ((entry = readdir(dp)) != nullptr) {
            std::string fileName(entry->d_name);
            if (fileName.rfind(partialCmd, 0) == 0) { 
                std::string fullPath = dir + "/" + fileName;
                if (isExecutable(fullPath)) {
                    closedir(dp);
                    return fileName;
                }
            }
        }
        closedir(dp);
    }
    return std::nullopt;
}

