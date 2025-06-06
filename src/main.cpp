#include <iostream>
#include <string>
#include <sstream>
#include "./helper/helper_utils.h"
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    while (true) {
        std::cout << "$ ";
        std::string input, arg;
        std::getline(std::cin, input);
        CommandType cmd = getCommandType(input, arg);
        switch (cmd) {
            case CMD_ECHO:
                std::cout << arg << std::endl;
                break;
        
            case CMD_TYPE:
                if (arg == "echo" || arg == "type" || arg == "exit") {
                    std::cout << arg << " is a shell builtin" << std::endl;
                } else {
                    // search in path for the command
                    std::string path_found = findInPath(arg);
                    if(!path_found.empty()) {
                        std::cout << arg << " is " << path_found << std::endl;
                    } else {
                        std::cout << arg << " not found" << std::endl;
                    }
                }
                break;
              
            case CMD_EXIT:
                return 0;
              
            case CMD_UNKNOWN:
                default:
                    std::istringstream iss(input);
                    std::vector<std::string> tokens;
                    std::string token;
                    
                    while(iss >> token) 
                        tokens.push_back(token);    

                    if(tokens.empty()) break;
                    
                    std::string command = tokens[0];
                    std::string cmdPath = findInPath(command);

                    if (cmdPath.empty()) {
                        std::cout << command << ": command not found" << std::endl;
                        break;
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
                    } else if (pid == 0) {
                        // Child process - just execute the program
                        execv(cmdPath.c_str(), argv.data());
                        perror("exec failed");
                        exit(1);
                    } else {
                        // Parent process - wait for child to finish
                        wait(nullptr);
                    }
                    break;
        }
    }

    return 0;
}
