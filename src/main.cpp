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
    
    std::cout << input << ": command not found" << std::endl;
  }

  return 0;
}
