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
#include <iostream>
#include <vector>
#include <algorithm>

std::vector<std::string> parseArgs(const std::string& input) {
    std::vector<std::string> args;
    std::string arg;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escape_next = false;

    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];

        if (escape_next) {
            if (in_double_quote) {
                if (c == '"' || c == '\\') {
                    arg += c;  
                } else {
                    arg += '\\';  
                    arg += c;
                }
            } else {
                arg += c;
            }
            escape_next = false;
            continue;
        }

        if (c == '\\') {
            if (in_single_quote) {
                arg += '\\';  
            } else {
                escape_next = true;
            }
            continue;
        }

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
            continue;
        }

        arg += c;
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

    // Check for output redirection (> and 1> for stdout, 2> for stderr)
    size_t stdout_redirect_pos = std::string::npos;
    size_t stderr_redirect_pos = std::string::npos;
    std::string stdout_redirect_symbol;
    std::string stderr_redirect_symbol;
    
    // Check for stdout redirection (> and 1>)
    std::vector<std::pair<std::string, size_t>> stdout_redirects = {
        {" 1> ", trimmed_arg.find(" 1> ")},
        {"1> ", trimmed_arg.find("1> ")},
        {" 1>", trimmed_arg.find(" 1>")},
        {"1>", trimmed_arg.find("1>")},
        {" > ", trimmed_arg.find(" > ")},
        {"> ", trimmed_arg.find("> ")},
        {" >", trimmed_arg.find(" >")},
        {">", trimmed_arg.find(">")}
    };
    
    // Check for stderr redirection (2>)
    std::vector<std::pair<std::string, size_t>> stderr_redirects = {
        {" 2> ", trimmed_arg.find(" 2> ")},
        {"2> ", trimmed_arg.find("2> ")},
        {" 2>", trimmed_arg.find(" 2>")},
        {"2>", trimmed_arg.find("2>")}
    };
    
    // Find stdout redirections
    for (const auto& [symbol, pos] : stdout_redirects) {
        if (pos != std::string::npos && (stdout_redirect_pos == std::string::npos || pos < stdout_redirect_pos)) {
            stdout_redirect_pos = pos;
            stdout_redirect_symbol = symbol;
        }
    }
    
    // Find stderr redirections
    for (const auto& [symbol, pos] : stderr_redirects) {
        if (pos != std::string::npos && (stderr_redirect_pos == std::string::npos || pos < stderr_redirect_pos)) {
            stderr_redirect_pos = pos;
            stderr_redirect_symbol = symbol;
        }
    }
    
    // Handle stderr redirection (2>) - for echo, this should just create an empty file
    if (stderr_redirect_pos != std::string::npos && 
        (stdout_redirect_pos == std::string::npos || stderr_redirect_pos < stdout_redirect_pos)) {
        
        // Extract the stderr output file
        std::string input_part = trimmed_arg.substr(0, stderr_redirect_pos);
        std::string stderr_output_part = trimmed_arg.substr(stderr_redirect_pos + stderr_redirect_symbol.length());
        
        input_part = removeExtraSpaces(input_part);
        stderr_output_part = removeExtraSpaces(stderr_output_part);
        
        if (stderr_output_part.empty()) {
            std::cout << "echo: missing output file" << std::endl;
            return;
        }
        
        // Create empty stderr redirect file
        namespace fs = std::filesystem;
        fs::path stderr_outFile;
        
        if (fs::path(stderr_output_part).is_absolute()) {
            stderr_outFile = stderr_output_part;
        } else {
            stderr_outFile = fs::current_path() / stderr_output_part;
        }
        
        fs::create_directories(stderr_outFile.parent_path());
        
        std::ofstream stderr_file(stderr_outFile);
        if (!stderr_file) {
            std::cerr << "echo: Failed to open file " << stderr_output_part << " for writing\n";
            return;
        }
        stderr_file.close(); // Create empty file
        
        // Continue with normal echo output to stdout using the remaining input
        std::vector<std::string> tokens = parseArgs(input_part);
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << tokens[i];
            if (i != tokens.size() - 1) std::cout << " ";
        }
        std::cout << std::endl;
        return;
    }
    
    // Handle stdout redirection (> and 1>)
    if (stdout_redirect_pos != std::string::npos) {
        std::string input_part = trimmed_arg.substr(0, stdout_redirect_pos);
        std::string output_part = trimmed_arg.substr(stdout_redirect_pos + stdout_redirect_symbol.length());
        
        input_part = removeExtraSpaces(input_part);
        output_part = removeExtraSpaces(output_part);
        
        if (output_part.empty()) {
            std::cout << "echo: missing output file" << std::endl;
            return;
        }
        
        // Parse the input text
        std::vector<std::string> tokens = parseArgs(input_part);
        
        // Build the output content
        std::string content;
        for (size_t i = 0; i < tokens.size(); ++i) {
            content += tokens[i];
            if (i != tokens.size() - 1) content += " ";
        }
        
        // Write to file
        namespace fs = std::filesystem;
        fs::path outFile;
        
        if (fs::path(output_part).is_absolute()) {
            outFile = output_part;
        } else {
            outFile = fs::current_path() / output_part;
        }
        
        // Create directories if they don't exist
        fs::create_directories(outFile.parent_path());
        
        std::ofstream file(outFile);
        if (!file) {
            std::cerr << "echo: Failed to open file " << output_part << " for writing\n";
            return;
        }
        
        file << content << std::endl;
        file.close();
        
        // Don't output anything to stdout when redirecting
        return;
    } else {
        // No redirection, output to stdout
        std::vector<std::string> tokens = parseArgs(trimmed_arg);
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << tokens[i];
            if (i != tokens.size() - 1) std::cout << " ";
        }
        std::cout << std::endl;
    }
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
    std::string trimmed_arg = removeExtraSpaces(arg);
    
    size_t stdout_redirect_pos = std::string::npos;
    size_t stderr_redirect_pos = std::string::npos;
    std::string stdout_redirect_symbol;
    std::string stderr_redirect_symbol;
    
    // Check for stdout redirection
    std::vector<std::pair<std::string, size_t>> stdout_redirects = {
        {" 1> ", trimmed_arg.find(" 1> ")},
        {"1> ", trimmed_arg.find("1> ")},
        {" 1>", trimmed_arg.find(" 1>")},
        {"1>", trimmed_arg.find("1>")},
        {" > ", trimmed_arg.find(" > ")},
        {"> ", trimmed_arg.find("> ")},
        {" >", trimmed_arg.find(" >")},
        {">", trimmed_arg.find(">")}
    };
    
    std::vector<std::pair<std::string, size_t>> stderr_redirects = {
        {" 2> ", trimmed_arg.find(" 2> ")},
        {"2> ", trimmed_arg.find("2> ")},
        {" 2>", trimmed_arg.find(" 2>")},
        {"2>", trimmed_arg.find("2>")}
    };
    
    for (const auto& [symbol, pos] : stdout_redirects) {
        if (pos != std::string::npos && (stdout_redirect_pos == std::string::npos || pos < stdout_redirect_pos)) {
            stdout_redirect_pos = pos;
            stdout_redirect_symbol = symbol;
        }
    }
    
    for (const auto& [symbol, pos] : stderr_redirects) {
        if (pos != std::string::npos && (stderr_redirect_pos == std::string::npos || pos < stderr_redirect_pos)) {
            stderr_redirect_pos = pos;
            stderr_redirect_symbol = symbol;
        }
    }
    
    std::string input_part = trimmed_arg;
    std::string stdout_output_file;
    std::string stderr_output_file;
    bool has_stdout_redirection = false;
    bool has_stderr_redirection = false;
    
    if (stderr_redirect_pos != std::string::npos && 
        (stdout_redirect_pos == std::string::npos || stderr_redirect_pos < stdout_redirect_pos)) {
        
        input_part = trimmed_arg.substr(0, stderr_redirect_pos);
        std::string remaining_part = trimmed_arg.substr(stderr_redirect_pos + stderr_redirect_symbol.length());
        
        input_part = removeExtraSpaces(input_part);
        remaining_part = removeExtraSpaces(remaining_part);
        
        size_t next_stdout_pos = std::string::npos;
        std::string next_stdout_symbol;
        
        for (const auto& [symbol, pos] : stdout_redirects) {
            size_t actual_pos = remaining_part.find(symbol);
            if (actual_pos != std::string::npos && (next_stdout_pos == std::string::npos || actual_pos < next_stdout_pos)) {
                next_stdout_pos = actual_pos;
                next_stdout_symbol = symbol;
            }
        }
        
        if (next_stdout_pos != std::string::npos) {
            stderr_output_file = removeExtraSpaces(remaining_part.substr(0, next_stdout_pos));
            stdout_output_file = removeExtraSpaces(remaining_part.substr(next_stdout_pos + next_stdout_symbol.length()));
            has_stdout_redirection = true;
        } else {
            stderr_output_file = remaining_part;
        }
        has_stderr_redirection = true;
    }

    else if (stdout_redirect_pos != std::string::npos) {
        input_part = trimmed_arg.substr(0, stdout_redirect_pos);
        std::string remaining_part = trimmed_arg.substr(stdout_redirect_pos + stdout_redirect_symbol.length());
        
        input_part = removeExtraSpaces(input_part);
        remaining_part = removeExtraSpaces(remaining_part);
        
        size_t next_stderr_pos = std::string::npos;
        std::string next_stderr_symbol;
        
        for (const auto& [symbol, pos] : stderr_redirects) {
            size_t actual_pos = remaining_part.find(symbol);
            if (actual_pos != std::string::npos && (next_stderr_pos == std::string::npos || actual_pos < next_stderr_pos)) {
                next_stderr_pos = actual_pos;
                next_stderr_symbol = symbol;
            }
        }
        
        if (next_stderr_pos != std::string::npos) {
            stdout_output_file = removeExtraSpaces(remaining_part.substr(0, next_stderr_pos));
            stderr_output_file = removeExtraSpaces(remaining_part.substr(next_stderr_pos + next_stderr_symbol.length()));
            has_stderr_redirection = true;
        } else {
            stdout_output_file = remaining_part;
        }
        has_stdout_redirection = true;
    }
    
    if ((has_stdout_redirection && stdout_output_file.empty()) || 
        (has_stderr_redirection && stderr_output_file.empty())) {
        std::cout << "cat: missing output file" << std::endl;
        return;
    }
    
    std::vector<std::string> files = parseArgs(input_part);
    
    std::ofstream stdout_file_stream;
    std::ofstream stderr_file_stream;
    std::ostream* output_stream = &std::cout;
    std::ostream* error_stream = &std::cerr;
    
    if (has_stdout_redirection) {
        namespace fs = std::filesystem;
        fs::path outFile;
        
        if (fs::path(stdout_output_file).is_absolute()) {
            outFile = stdout_output_file;
        } else {
            outFile = fs::current_path() / stdout_output_file;
        }
        
        fs::create_directories(outFile.parent_path());
        
        stdout_file_stream.open(outFile);
        if (!stdout_file_stream) {
            std::cerr << "cat: Failed to open file " << stdout_output_file << " for writing\n";
            return;
        }
        output_stream = &stdout_file_stream;
    }
    
    if (has_stderr_redirection) {
        namespace fs = std::filesystem;
        fs::path stderr_outFile;
        
        if (fs::path(stderr_output_file).is_absolute()) {
            stderr_outFile = stderr_output_file;
        } else {
            stderr_outFile = fs::current_path() / stderr_output_file;
        }
        
        fs::create_directories(stderr_outFile.parent_path());
        
        stderr_file_stream.open(stderr_outFile);
        if (!stderr_file_stream) {
            std::cerr << "cat: Failed to open file " << stderr_output_file << " for writing\n";
            return;
        }
        error_stream = &stderr_file_stream;
    }
    
    bool first_file = true;
    bool has_error = false;
    bool has_content = false;
    
    for (const auto& filename : files) {
        std::ifstream input_file(filename);
        if (!input_file.is_open()) {
            *error_stream << "cat: " << filename << ": No such file or directory" << std::endl;
            has_error = true;
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(input_file)), 
                        std::istreambuf_iterator<char>());
        
        *output_stream << content;
        has_content = true;
        
        input_file.close();
        first_file = false;
    }
    
    if (has_stdout_redirection) {
        stdout_file_stream.close();
    }
    
    if (has_stderr_redirection) {
        stderr_file_stream.close();
    }
}

void executeLs(std::string& arg) {
    namespace fs = std::filesystem;
    
    arg = removeExtraSpaces(arg);
    
    size_t stderr_redirect_pos = std::string::npos;
    std::string stderr_output_file;
    bool has_stderr_redirect = false;
    
    std::vector<std::pair<std::string, size_t>> stderr_redirects = {
        {" 2> ", arg.find(" 2> ")},
        {"2> ", arg.find("2> ")},
        {" 2>", arg.find(" 2>")},
        {"2>", arg.find("2>")}
    };
    
    for (const auto& [symbol, pos] : stderr_redirects) {
        if (pos != std::string::npos && (stderr_redirect_pos == std::string::npos || pos < stderr_redirect_pos)) {
            stderr_redirect_pos = pos;
            std::string stderr_part = arg.substr(pos + symbol.length());
            stderr_output_file = removeExtraSpaces(stderr_part);
            has_stderr_redirect = true;
            arg = arg.substr(0, pos);
            break;
        }
    }
    
    std::vector<std::string> parts = split(arg, '>');
    
    std::string inputArgs;
    std::string outputPath;
    
    if (parts.size() == 1) {
        inputArgs = parts[0];
    } else if (parts.size() == 2) {
        inputArgs = removeExtraSpaces(parts[0]);
        outputPath = removeExtraSpaces(parts[1]);
    } else {
        if (has_stderr_redirect) {
            fs::path stderr_outFile;
            if (fs::path(stderr_output_file).is_absolute()) {
                stderr_outFile = stderr_output_file;
            } else {
                stderr_outFile = fs::current_path() / stderr_output_file;
            }
            fs::create_directories(stderr_outFile.parent_path());
            std::ofstream stderr_file(stderr_outFile);
            if (stderr_file) {
                stderr_file << "ls: too many arguments" << std::endl;
                stderr_file.close();
            }
        } else {
            std::cerr << "ls: too many arguments" << std::endl;
        }
        return;
    }
    
    std::vector<std::string> tokens = parseArgs(inputArgs);
    
    bool listVertical = false;
    std::string targetPath = "."; 
    
    for (const auto& token : tokens) {
        if (token == "-1") {
            listVertical = true;
        } else if (token != "ls") {
            targetPath = token;
        }
    }
    
    fs::path dirPath;
    if (fs::path(targetPath).is_absolute()) {
        dirPath = targetPath;
    } else {
        dirPath = fs::current_path() / targetPath;
    }
    
    std::ofstream stderr_file;
    std::ostream* error_stream = &std::cerr;
    
    if (has_stderr_redirect) {
        fs::path stderr_outFile;
        if (fs::path(stderr_output_file).is_absolute()) {
            stderr_outFile = stderr_output_file;
        } else {
            stderr_outFile = fs::current_path() / stderr_output_file;
        }
        fs::create_directories(stderr_outFile.parent_path());
        stderr_file.open(stderr_outFile);
        if (stderr_file) {
            error_stream = &stderr_file;
        }
    }
    
    if (!fs::exists(dirPath)) {
        *error_stream << "ls: " << targetPath << ": No such file or directory" << std::endl;
        if (has_stderr_redirect) {
            stderr_file.close();
        }
        return;
    }
    
    std::vector<std::string> entries;
    try {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            entries.push_back(entry.path().filename().string());
        }
    } catch (const std::exception& e) {
        *error_stream << "ls: " << targetPath << ": Permission denied" << std::endl;
        if (has_stderr_redirect) {
            stderr_file.close();
        }
        return;
    }
    
    std::sort(entries.begin(), entries.end());
    
    if (outputPath.empty()) {
        if (listVertical) {
            for (const auto& entry : entries) {
                std::cout << entry << "\n";
            }
        } else {
            for (size_t i = 0; i < entries.size(); ++i) {
                std::cout << entries[i];
                if (i != entries.size() - 1) std::cout << "  ";
            }
            if (!entries.empty()) std::cout << "\n";
        }
    } else {
        fs::path outFile;
        if (fs::path(outputPath).is_absolute()) {
            outFile = outputPath;
        } else {
            outFile = fs::current_path() / outputPath;
        }
        
        fs::create_directories(outFile.parent_path());
        
        std::ofstream file(outFile);
        if (!file) {
            *error_stream << "ls: Failed to open file " << outputPath << " for writing\n";
            if (has_stderr_redirect) {
                stderr_file.close();
            }
            return;
        }
        
        for (const auto& entry : entries) {
            file << entry << "\n";
        }
        
        file.close();
    }
    
    if (has_stderr_redirect) {
        stderr_file.close();
    }
}
