#include <iostream>
#include <string>
// using namespace std;
int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true) {
    std::cout << "$ ";

    std::string input;
    getline(std::cin, input);

    if (input == "exit 0") return 0;
    
    if (input.find("echo") != std::string::npos) {
      std::string echo = "echo ";
      std::cout << input.substr(echo.length()) << std::endl;
    } else {
      std::cout << input << ": command not found" << std::endl;
    }

  }

  return 0;
}
