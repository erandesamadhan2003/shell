#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <readline/readline.h>
#include <readline/history.h>

#include "./helper/helper_utils.h"
#include "./commands/command_utils.h"

std::vector<std::string> commandList = {
    "echo", "exit", "type", "pwd", "cd", "cat", "ls"
};

char* commandGenerator(const char* text, int state) {
    static size_t list_index;
    static size_t len;
    
    if (state == 0) {
        list_index = 0;
        len = strlen(text);
    }

    while (list_index < commandList.size()) {
        const std::string& cmd = commandList[list_index++];
        if (cmd.compare(0, len, text) == 0) {
            return strdup(cmd.c_str());
        }
    }
    return nullptr;
}

// Wrapper for readline completion
char** commandCompletion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, commandGenerator);
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Set the autocompletion function
    rl_attempted_completion_function = commandCompletion;

    while (true) {
        char* input_cstr = readline("$ ");
        if (!input_cstr) break;

        std::string input(input_cstr);
        free(input_cstr); // clean up

        if (!input.empty()) add_history(input.c_str());

        std::string arg;
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
            case CMD_CD:
                executeCd(arg);
                break;
            case CMD_CAT:
                executeCat(arg);
                break;
            case CMD_LS:
                executeLs(arg);
                break;
            case CMD_UNKNOWN:
            default:
                executeUnknownCommand(input);
                break;
        }
    }

    return 0;
}
