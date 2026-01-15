/**
 * UCI (Universal Chess Interface) wrapper for MagnusCarlsenMogger
 * This allows the engine to be tested with cutechess-cli and other UCI tools
 * for Elo rating estimation.
 * 
 * The original main.cpp interface remains unchanged for teacher evaluation.
 */

#include "../src/board.h"
#include "../src/eval/evaluate.h"
#include "../src/eval/psqt.h"
#include "../src/gen.hpp"
#include "../src/move.h"
#include "../src/search.h"
#include "../src/zobrist.h"
#include "../src/tt.h"
#include "../src/magic.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

// External time limit from search.cpp
extern int time_limit_ms;

// Find a move in the legal moves list that matches the UCI string
Move findMoveFromString(Board &board, const std::string &moveStr) {
    MoveGenerator gen(board, board.sideToMove);
    Move pseudoLegal[220];
    size_t pseudoLegalCount = gen.generatePseudoLegalMoves(pseudoLegal);
    Move legalMoves[220];
    size_t legalCount = gen.filterLegalMoves(pseudoLegal, pseudoLegalCount, legalMoves);
    
    for (size_t i = 0; i < legalCount; i++) {
        const Move &m = legalMoves[i];
        std::string mStr = m.toString();
        // Handle promotion suffix (e.g., "e7e8q")
        if (m.promotion != PieceType::EMPTY) {
            char promoChar = ' ';
            switch (m.promotion) {
                case PieceType::QUEEN:  promoChar = 'q'; break;
                case PieceType::ROOK:   promoChar = 'r'; break;
                case PieceType::BISHOP: promoChar = 'b'; break;
                case PieceType::KNIGHT: promoChar = 'n'; break;
                default: break;
            }
            mStr += promoChar;
        }
        if (mStr == moveStr) {
            return m;
        }
    }
    
    // Fallback: use parseMove if no exact match (handles basic moves)
    return parseMove(moveStr);
}

// Handle "position" command
void handlePosition(Board &board, std::istringstream &is) {
    std::string token;
    is >> token;

    if (token == "startpos") {
        board = Board();
        board.initStartPosition(); // Must initialize to starting position!
        is >> token;     // Consume "moves" if present
    } else if (token == "fen") {
        // FEN support not implemented - fallback to startpos
        // Skip FEN tokens until "moves" or end of line
        while (is >> token && token != "moves") {
            // Skip FEN parts
        }
        board = Board();
        board.initStartPosition(); // Must initialize to starting position!
        // Note: If FEN support is needed, implement board.setFromFEN()
    }

    // Apply moves if present
    if (token == "moves") {
        while (is >> token) {
            Move m = findMoveFromString(board, token);
            board.update_move(m);
        }
    }
}

// Handle "go" command
void handleGo(Board &board, std::istringstream &is) {
    std::string token;
    int depth = 64;      // Default max depth
    int wtime = 0, btime = 0, winc = 0, binc = 0;
    int movestogo = 30;  // Default moves to go
    int movetime = 0;
    bool infinite = false;

    while (is >> token) {
        if (token == "depth") {
            is >> depth;
        } else if (token == "wtime") {
            is >> wtime;
        } else if (token == "btime") {
            is >> btime;
        } else if (token == "winc") {
            is >> winc;
        } else if (token == "binc") {
            is >> binc;
        } else if (token == "movestogo") {
            is >> movestogo;
        } else if (token == "movetime") {
            is >> movetime;
        } else if (token == "infinite") {
            infinite = true;
        }
    }

    // Time management: set time_limit_ms and let iterative deepening work
    int searchDepth = depth;
    
    if (movetime > 0) {
        // Fixed time per move - use it directly
        time_limit_ms = movetime - 50;  // Small buffer for safety
        searchDepth = 64;  // Let time control limit the search
    } else if (wtime > 0 || btime > 0) {
        // Time control - allocate time based on remaining time
        int ourTime = (board.sideToMove == Color::WHITE) ? wtime : btime;
        int ourInc = (board.sideToMove == Color::WHITE) ? winc : binc;
        
        // Simple allocation: use ~1/30th of remaining time + increment
        int allocatedTime = ourTime / movestogo + ourInc;
        
        // Set time limit with small buffer
        time_limit_ms = allocatedTime - 20;
        searchDepth = 64;  // Let time control limit the search
    } else if (infinite) {
        time_limit_ms = 1000000;  // Very long time for infinite
        searchDepth = 64;
    } else {
        // No time info - use reasonable default (5 seconds)
        time_limit_ms = 5000;
        searchDepth = 64;
    }

    // Search for best move using iterative deepening with time control
    Move bestMove = Search::findBestMove(board, searchDepth);
    
    // Format output
    std::string moveStr = bestMove.toString();
    if (bestMove.promotion != PieceType::EMPTY) {
        char promoChar = ' ';
        switch (bestMove.promotion) {
            case PieceType::QUEEN:  promoChar = 'q'; break;
            case PieceType::ROOK:   promoChar = 'r'; break;
            case PieceType::BISHOP: promoChar = 'b'; break;
            case PieceType::KNIGHT: promoChar = 'n'; break;
            default: break;
        }
        moveStr += promoChar;
    }
    
    std::cout << "bestmove " << moveStr << std::endl;
}

void uciLoop() {
    Board board;
    board.initStartPosition(); // Initialize to starting position
    std::string line, token;

    while (std::getline(std::cin, line)) {
        std::istringstream is(line);
        is >> token;

        if (token == "uci") {
            std::cout << "id name MagnusCarlsenMogger" << std::endl;
            std::cout << "id author CSE201_Team" << std::endl;
            // No options for now
            std::cout << "uciok" << std::endl;
        } 
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } 
        else if (token == "ucinewgame") {
            board = Board();
            board.initStartPosition();
            // Clear transposition table for new game
            TT::tt.clear();
        } 
        else if (token == "position") {
            handlePosition(board, is);
        } 
        else if (token == "go") {
            handleGo(board, is);
        } 
        else if (token == "quit") {
            break;
        }
        // Silently ignore unknown commands
    }
}

int main() {
    // Initialize magic bitboards
    Magic::init();
    
    // Verify magic bitboards are working correctly
    if (!Magic::verify()) {
        std::cerr << "WARNING: Magic bitboard verification failed! Results may be incorrect.\n";
    }
    
    // Initialize piece-square tables
    PSQT::init();
    
    // Initialize Zobrist hashing (required for TT)
    Zobrist::init();
    
    // Clear transposition table
    TT::tt.clear();
    
    // Disable output buffering for proper UCI communication
    std::cout.setf(std::ios::unitbuf);
    std::cin.tie(nullptr);
    
    uciLoop();
    return 0;
}

