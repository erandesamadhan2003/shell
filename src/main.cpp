#include <iostream>
#include <string>
#include <sstream>
#include "./helper/helper_utils.h"
#include "./commands/command_utils.h"
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
                executeEcho(arg);
                break;
        
            case CMD_TYPE:
                executeType(arg);
                break;
              
            case CMD_EXIT:
                return 0;

            case CMD_PWD:
                executePwd();
                break;
              
            case CMD_UNKNOWN:
                default:
                executeUnknownCommand(input);
                break;
        }
    }

    return 0;
}
