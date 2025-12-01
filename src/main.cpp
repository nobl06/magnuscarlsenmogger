#include "board.h"
#include "debugger.h"
#include "evaluate.h"
#include "gen.hpp"
#include "move.h"
#include "eval/psqt.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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
    // Initialize piece-square tables
    PSQT::init();
    
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

    // Printing evaluation of position
    std::cout << "Evaluation = " << evaluate(board, board.sideToMove) << "\n";

    // Generate moves for the side to move
    MoveGenerator generator(board, board.sideToMove);
    std::vector<Move> pseudoLegalMoves = generator.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = generator.filterLegalMoves(pseudoLegalMoves);

    // Choose a move
    Move chosenMove = chooseMove(legalMoves);

    write_out(outputfile, chosenMove.toString());

    print_file(outputfile);

    return 0;
}
