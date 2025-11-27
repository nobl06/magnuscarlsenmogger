#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> read_file(std::string file) {
    std::ifstream f(file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(line);
    }
    return lines;
}

int main(int argc, char *argv[]) {
    std::string inputfile;
    std::string outputfile;

    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "-H") {
            inputfile = argv[i + 1]; // FIXED
        }
        if (std::string(argv[i]) == "-m") {
            outputfile = argv[i + 1]; // FIXED
        }
    }

    if (inputfile.empty()) {
        std::cerr << "No input file provided (-H <file>)\n";
        return 1;
    }

    std::vector<std::string> move_hist = read_file(inputfile);

    for (auto &m : move_hist) {
        std::cout << m << "\n";
    }

    return 0;
}
