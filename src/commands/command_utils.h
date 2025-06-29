#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include <string>
#include <vector>
#include <filesystem>

void executeEcho(const std::string& arg);
void executeType(const std::string& arg);
void executeUnknownCommand(const std::string& input);
void executePwd();
void executeCd(const std::string& arg);
void executeCat(const std::string& arg);
void executeLs(std::string& arg);
void executeWc(const std::string& input);

#endif