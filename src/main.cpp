#include "board.h"
#include "debugger.h"
#include "eval/evaluate.h"
#include "eval/psqt.h"
#include "gen.hpp"
#include "magic.h"
#include "move.h"
#include "search.h"
#include "zobrist.h"
#include "tt.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// External time limit from search.cpp
extern int time_limit_ms;



std::vector<std::string> read_file(std::string file) {
    std::ifstream f(file); // open file
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(line); // adding each line of file to vector
    }
    return lines;
}

void write_out(std::string out, std::string move) {
    std::ofstream file(out);

    if (!file.is_open()) {
        std::cerr << "Could not open file " << out << "\n";
        return;
    }

    file << move << "\n"; // writing move in out

    file.close();
}

bool is_white(const std::vector<std::string> &move_hist) {
    return move_hist.size() % 2 == 0;
}

int main(int argc, char *argv[]) {
    // Start timer
    auto total_start = std::chrono::steady_clock::now();

    // Initialize piece-square tables
    PSQT::init();
    
    // Initialize Zobrist hashing
    Zobrist::init();

    // Initialize magic bitboards
    Magic::init();
    
    // Initialize transposition table (already initialized globally, but clear it)
    TT::tt.clear();

    std::string inputfile;
    std::string outputfile;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-H") {
            inputfile = argv[i + 1];
        }
        if (std::string(argv[i]) == "-m") {
            outputfile = argv[i + 1];
        }
    }

    if (inputfile.empty()) {
        std::cerr << "No input file provided (-H <file>)\n";
        return 1;
    }

    std::vector<std::string> move_hist = read_file(inputfile);

    print_vector(move_hist);

    Board board;
    board.gamestate(move_hist);
    board.print();

    // printing evaluation of position
    std::cout << "Evaluation = " << Evaluation::evaluate(board) << "\n";

    // search for best move
    const int MAXDEPTH = 64;
    time_limit_ms = 9000;  
    Move chosenMove = Search::findBestMove(board, MAXDEPTH);

    auto total_end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        total_end - total_start)
                        .count();

    std::cout << "Search depth: " << Search::stats.depthReached << "\n";
    std::cout << "Nodes searched: " << Search::stats.nodes << "\n";
    std::cout << "Best move: " << chosenMove.toString() << "\n";
    std::cout << "Total time: " << total_ms << " ms\n";

    write_out(outputfile, chosenMove.toString());

    print_file(outputfile);

    return 0;
}

//