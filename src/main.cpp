#include <iostream>
#include <string>
#include "./command/command_utils.h"

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
                        std::cout << arg << " not found" << std::endl;
                    }
                    break;
                  
                case CMD_EXIT:
                    return 0;
                  
                case CMD_UNKNOWN:
                default:
                    std::cout << input << ": command not found" << std::endl;
                    break;
          }
    }

    return 0;
}
