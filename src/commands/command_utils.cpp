#include<iostream>
#include "command_utils.h"
#include "../helper/helper_utils.h"
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

std::vector<std::string> parseArgs(const std::string& input) {
    std::vector<std::string> args;
    std::string arg;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }

        if (c == ' ' && !in_single_quote && !in_double_quote) {
            // Skip multiple spaces
            while (i + 1 < input.size() && input[i + 1] == ' ') {
                ++i;
            }
            if (!arg.empty()) {
                args.push_back(arg);
                arg.clear();
            }
        } else {
            arg += c;
        }
    }

    if (!arg.empty()) {
        args.push_back(arg);
    }

    return args;
}

void executeEcho(const std::string& arg) {
    std::string trimmed_arg = removeExtraSpaces(arg);
    if (trimmed_arg.empty()) {
        std::cout << std::endl;
        return;
    }

    std::vector<std::string> tokens = parseArgs(trimmed_arg);
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i];
        if (i != tokens.size() - 1) std::cout << " ";
    }

    std::cout << std::endl;
}

void executeType(const std::string& arg) {
    if (arg == "echo" || arg == "type" || arg == "exit" || arg == "pwd") {
        std::cout << arg << " is a shell builtin" << std::endl;
    } else {
        // search in path for the command
        std::string path_found = findInPath(arg);
        if (!path_found.empty()) {
            std::cout << arg << " is " << path_found << std::endl;
        } else {
            std::cout << arg << " not found" << std::endl;
        }
    }
}

void executeUnknownCommand(const std::string& input) {
    std::vector<std::string> tokens = parseArgs(input);

    if (tokens.empty()) return;

    std::string command = tokens[0];
    std::string cmdPath = findInPath(command);

    if (cmdPath.empty()) {
        std::cout << command << ": command not found" << std::endl;
        return;
    }

    // convert vector<string> to char* array for execv
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.c_str()));

    for (size_t i = 1; i < tokens.size(); ++i) {
        argv.push_back(const_cast<char*>(tokens[i].c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    } else if (pid == 0) {
        execv(cmdPath.c_str(), argv.data());
        perror("execv failed");
        exit(1);
    } else {
        wait(nullptr); 
    }
}

void executePwd() {
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::cout << currentPath.string() << std::endl;
}

void executeCd(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "cd: missing argument" << std::endl;
        return;
    }

    // If the argument is "~", change to home directory 
    if (arg == "~") {
        const char* home = getenv("HOME");
        if (home) {
            std::filesystem::path homePath(home);
            std::filesystem::current_path(homePath);
            return;
        } else {
            std::cout << "cd: HOME not set" << std::endl;
            return;
        }
    }

    std::error_code ec;
    std::filesystem::path newPath = std::filesystem::absolute(arg, ec);

    if (ec) {
        std::cout << "cd: " << ec.message() << std::endl;
        return;
    }

    if (!std::filesystem::exists(newPath, ec)) {
        std::cout << "cd: " << newPath.string() << ": No such file or directory" << std::endl;
        return;
    }

    if (!std::filesystem::is_directory(newPath, ec)) {
        std::cout << "cd: not a directory: " << newPath.string() << std::endl;
        return;
    }

    std::filesystem::current_path(newPath, ec);
    if (ec) {
        std::cout << "cd: " << ec.message() << std::endl;
    }
}

void executeCat(const std::string& arg) {
    std::vector<std::string> files = parseArgs(arg);

    for (const auto& filename : files) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "cat: " << filename << ": No such file or directory" << std::endl;
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::cout << content;
        file.close();
    }
    std::cout << std::endl;
}