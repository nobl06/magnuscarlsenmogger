#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void print_file(std::string file) {
    std::ifstream f(file);
    std::string line;
    while (std::getline(f, line)) {
        std::cout << line << "\n";
    }
}

void print_vector(const std::vector<std::string> &v) {
    for (const std::string &s : v) {
        std::cout << s << "\n";
    }
}