#include "command_utils.h"

#include<string>

std::string removeExtraSpaces(std::string s) {
    size_t start = s.find_first_not_of(" \t");
    return (start == std::string::npos) ? "" : s.substr(start);
}

CommandType getCommandType(const std::string& input, std::string& arg) {
    std::string trimmed = removeExtraSpaces(input);
    if (trimmed == "exit 0") return CMD_EXIT;

    if (trimmed.rfind("echo ", 0) == 0) {
        arg = trimmed.substr(5); 
        return CMD_ECHO;
    }

    if (trimmed.rfind("type ", 0) == 0) {
        arg = removeExtraSpaces(trimmed.substr(5));
        return CMD_TYPE;
    }

    return CMD_UNKNOWN;
}

