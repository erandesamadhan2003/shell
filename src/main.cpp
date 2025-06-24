#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <readline/readline.h>
#include <readline/history.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include <iomanip>
#include <dirent.h>
#include "./helper/helper_utils.h"
#include "./commands/command_utils.h"

std::vector<std::string> commandList = {
    "echo", "exit", "type", "pwd", "cd", "cat", "ls", "wc"
};

std::vector<std::string> getExecutablesInPath() {
    std::vector<std::string> executables;
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return executables;

    std::stringstream ss(pathEnv);
    std::string dir;

    while (std::getline(ss, dir, ':')) {
        DIR* dp = opendir(dir.c_str());
        if (!dp) continue;

        dirent* entry;
        while ((entry = readdir(dp)) != nullptr) {
            std::string fileName(entry->d_name);
            
            // Skip . and .. entries
            if (fileName == "." || fileName == "..") continue;
            
            std::string fullPath = dir + "/" + fileName;

            // Check if file is executable
            if (access(fullPath.c_str(), X_OK) == 0) {
                executables.push_back(fileName);
            }
        }

        closedir(dp);
    }

    return executables;
}

// Global variables for completion state
static std::vector<std::string> allCommands;
static std::vector<std::string> matches;
static size_t match_index = 0;

char* commandGenerator(const char* text, int state) {
    if (state == 0) {
        // Initialize on first call
        match_index = 0;
        matches.clear();
        
        // Get all available commands
        allCommands = commandList;
        std::vector<std::string> pathCommands = getExecutablesInPath();
        allCommands.insert(allCommands.end(), pathCommands.begin(), pathCommands.end());
        
        // Remove duplicates and sort
        std::sort(allCommands.begin(), allCommands.end());
        allCommands.erase(std::unique(allCommands.begin(), allCommands.end()), allCommands.end());
        
        // Find all matches for the given text
        std::string prefix(text);
        for (const auto& cmd : allCommands) {
            if (cmd.substr(0, prefix.length()) == prefix) {
                matches.push_back(cmd);
            }
        }
    }
    
    // Return next match
    if (match_index < matches.size()) {
        return strdup(matches[match_index++].c_str());
    }
    
    return nullptr;
}

// Custom completion function that handles partial completion
char** commandCompletion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    
    // Get all matches
    char** matches = rl_completion_matches(text, commandGenerator);
    
    if (matches && matches[0] && matches[1]) {
        // Multiple matches found - find common prefix
        std::string first(matches[1]);
        std::string common = first;
        
        for (int i = 2; matches[i] != nullptr; i++) {
            std::string current(matches[i]);
            
            // Find common prefix between 'common' and 'current'
            size_t j = 0;
            while (j < common.length() && j < current.length() && 
                   common[j] == current[j]) {
                j++;
            }
            common = common.substr(0, j);
        }
        
        // If we found a longer common prefix than what was typed, use it
        if (common.length() > strlen(text)) {
            free(matches[0]);
            matches[0] = strdup(common.c_str());
        }
    }
    
    return matches;
}

void executeTwoCommandPipeline(const std::string& cmd1, const std::string& cmd2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        return;
    }

    if (pid1 == 0) {
        // First child - execute first command
        close(pipefd[0]); // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]);

        // Parse and execute first command
        std::string arg;
        CommandType cmdType = getCommandType(cmd1, arg);
        
        switch (cmdType) {
            case CMD_CAT:
                executeCat(arg);
                break;
            case CMD_ECHO:
                executeEcho(arg);
                break;
            case CMD_LS:
                executeLs(arg);
                break;
            default:
                executeUnknownCommand(cmd1);
                break;
        }
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        return;
    }

    if (pid2 == 0) {
        // Second child - execute second command
        close(pipefd[1]); // Close write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(pipefd[0]);

        // Parse and execute second command
        std::string arg;
        CommandType cmdType = getCommandType(cmd2, arg);
        
        switch (cmdType) {
            case CMD_WC:
                executeWc(arg);
                break;
            case CMD_CAT:
                executeCat(arg);
                break;
            default:
                executeUnknownCommand(cmd2);
                break;
        }
        exit(0);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
}

// Simple function to check if input contains a pipe
bool hasPipe(const std::string& input) {
    return input.find('|') != std::string::npos;
}

// Function to split string and trim whitespace
std::vector<std::string> splitAndTrim(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        // Trim whitespace
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        } else if (start != std::string::npos) {
            result.push_back(item.substr(start));
        }
    }
    
    return result;
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Set up tab completion
    rl_attempted_completion_function = commandCompletion;

    while (true) {
        char* input_cstr = readline("$ ");
        if (!input_cstr) break;

        std::string input(input_cstr);
        free(input_cstr);

        if (!input.empty()) {
            add_history(input.c_str());
        }

        // Handle exit command
        if (input == "exit") {
            return 0;
        }

        // Check for pipeline
        if (hasPipe(input)) {
            std::vector<std::string> commands = splitAndTrim(input, '|');
            if (commands.size() == 2) {
                executeTwoCommandPipeline(commands[0], commands[1]);
            } else {
                std::cout << "Only two-command pipelines supported currently" << std::endl;
            }
        } else {
            // Single command execution
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
                case CMD_WC:
                    executeWc(arg);
                    break;
                case CMD_UNKNOWN:
                    executeUnknownCommand(input);
                    break;
                default:
                    std::cout << "Unknown command: " << input << std::endl;
                    break;
            }
        }
    }

    return 0;
}