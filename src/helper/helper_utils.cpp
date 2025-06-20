#include <iostream>
#include "helper_utils.h"
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>

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
    std::error_code error; // is an object which handle errors silently withour crashing the program
    if(!std::filesystem::exists(path, error)) return false;
    if(!std::filesystem::is_regular_file(path, error)) return false;  // check the whether the path is normal path or not(isfolder or symbolic path)
    return true;
}

std::string findInPath(const std::string& cmd) {
    const char* path_env = getenv("PATH");
    if(!path_env) return "";

    std::string path_str(path_env); // convert C style string to c++ string so that c++ operation can be done on command string
    std::vector<std::string> dirs = split(path_str, ':');

    for(const std::string& dir : dirs) {
        std::filesystem::path full_path = std::filesystem::path(dir) / cmd;
        if(isExcutable(full_path)) {
            return full_path.string();
        }
    }

    return "";
}

