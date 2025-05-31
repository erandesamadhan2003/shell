#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H
#include <string>

enum CommandType{
    CMD_ECHO,
    CMD_TYPE,
    CMD_EXIT,
    CMD_UNKNOWN
};

std::string removeExtraSpaces(std::string s);

CommandType getCommandType(const std::string& input, std::string& arg);

#endif
