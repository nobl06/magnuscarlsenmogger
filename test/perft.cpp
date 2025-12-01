#include "../src/board.h"
#include "../src/gen.hpp"
#include "../src/move.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// Perft function: counts all positions at a given depth
uint64_t perft(Board& board, int depth) {
    if (depth == 0) {
        return 1;  // Leaf node
    }
    
    MoveGenerator generator(board, board.sideToMove);
    std::vector<Move> pseudoLegal = generator.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = generator.filterLegalMoves(pseudoLegal);
    
    if (depth == 1) {
        return legalMoves.size();  // Optimization: don't need to make moves at depth 1
    }
    
    uint64_t nodes = 0;
    for (const Move& move : legalMoves) {
        Board copy = board;
        copy.update_move(move);
        nodes += perft(copy, depth - 1);
    }
    
    return nodes;
}

// Perft with move breakdown (useful for debugging)
void perftDivide(Board& board, int depth) {
    MoveGenerator generator(board, board.sideToMove);
    std::vector<Move> pseudoLegal = generator.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = generator.filterLegalMoves(pseudoLegal);
    
    uint64_t totalNodes = 0;
    
    std::cout << "\nPerft Divide (depth " << depth << "):\n";
    std::cout << "--------------------------------\n";
    
    for (const Move& move : legalMoves) {
        Board copy = board;
        copy.update_move(move);
        uint64_t nodes = perft(copy, depth - 1);
        totalNodes += nodes;
        
        std::cout << move.toString() << ": " << nodes << "\n";
    }
    
    std::cout << "--------------------------------\n";
    std::cout << "Total nodes: " << totalNodes << "\n\n";
}

// Test structure
struct PerftTest {
    std::string name;
    std::string fen;  // For now, we'll use description since we don't have FEN parser
    void (*setupBoard)(Board&);
    int depth;
    uint64_t expectedNodes;
};

// Board setup functions
void setupStartingPosition(Board& board) {
    board.initStartPosition();
}

void setupKiwipete(Board& board) {
    // This would need FEN parser, skip for now
    // FEN: r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
    board.clear();
    // We'll implement this later when we add FEN support
}

// Run a single perft test
bool runPerftTest(const PerftTest& test) {
    Board board;
    test.setupBoard(board);
    
    std::cout << "Testing: " << test.name << " (depth " << test.depth << ")\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes = perft(board, test.depth);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double nps = (nodes * 1000.0) / (duration.count() + 1);  // Nodes per second
    
    bool passed = (nodes == test.expectedNodes);
    
    std::cout << "  Expected: " << test.expectedNodes << "\n";
    std::cout << "  Got:      " << nodes << "\n";
    std::cout << "  Time:     " << duration.count() << " ms\n";
    std::cout << "  Speed:    " << std::fixed << std::setprecision(0) << nps << " nodes/sec\n";
    std::cout << "  Result:   " << (passed ? "✓ PASS" : "✗ FAIL") << "\n\n";
    
    return passed;
}

int main(int argc, char* argv[]) {
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "  Magnus Chess Engine - Perft Test Suite\n";
    std::cout << "═══════════════════════════════════════════\n\n";
    
    // Define test suite with known correct perft values
    std::vector<PerftTest> tests = {
        {"Starting Position", "", setupStartingPosition, 1, 20},
        {"Starting Position", "", setupStartingPosition, 2, 400},
        {"Starting Position", "", setupStartingPosition, 3, 8902},
        {"Starting Position", "", setupStartingPosition, 4, 197281},
        {"Starting Position", "", setupStartingPosition, 5, 4865609},
        // We can add depth 6 later when it's faster
        // {"Starting Position", "", setupStartingPosition, 6, 119060324},
    };
    
    // Check if user wants divide mode
    if (argc > 1 && std::string(argv[1]) == "divide") {
        int depth = 3;
        if (argc > 2) {
            depth = std::stoi(argv[2]);
        }
        
        Board board;
        board.initStartPosition();
        std::cout << "Running perft divide from starting position:\n";
        board.print();
        perftDivide(board, depth);
        return 0;
    }
    
    // Run all tests
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        if (runPerftTest(test)) {
            passed++;
        } else {
            failed++;
        }
    }
    
    // Summary
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "  Test Summary\n";
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "  Total:  " << (passed + failed) << "\n";
    std::cout << "  Passed: " << passed << " ✓\n";
    std::cout << "  Failed: " << failed << (failed > 0 ? " ✗" : "") << "\n";
    std::cout << "═══════════════════════════════════════════\n";
    
    if (failed == 0) {
        std::cout << "\n🎉 All tests passed! Move generation is correct!\n";
        return 0;
    } else {
        std::cout << "\n⚠️  Some tests failed. Check move generation logic.\n";
        return 1;
    }
}

