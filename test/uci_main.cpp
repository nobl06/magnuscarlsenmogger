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
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

// Find a move in the legal moves list that matches the UCI string
Move findMoveFromString(Board &board, const std::string &moveStr) {
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);
    
    for (const Move &m : legalMoves) {
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
    int depth = 64;      // Default max depth (will be capped)
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

    // Simple time management
    // For now, use a fixed depth based on available time
    int searchDepth = depth;
    
    if (movetime > 0) {
        // Fixed time per move - use reasonable depth
        if (movetime < 1000) searchDepth = std::min(searchDepth, 4);
        else if (movetime < 5000) searchDepth = std::min(searchDepth, 6);
        else if (movetime < 15000) searchDepth = std::min(searchDepth, 8);
        else searchDepth = std::min(searchDepth, 10);
    } else if (wtime > 0 || btime > 0) {
        // Time control - allocate time based on remaining time
        int ourTime = (board.sideToMove == Color::WHITE) ? wtime : btime;
        int ourInc = (board.sideToMove == Color::WHITE) ? winc : binc;
        
        // Simple allocation: use ~1/30th of remaining time + increment
        int allocatedTime = ourTime / movestogo + ourInc;
        
        if (allocatedTime < 500) searchDepth = std::min(searchDepth, 3);
        else if (allocatedTime < 1000) searchDepth = std::min(searchDepth, 4);
        else if (allocatedTime < 2000) searchDepth = std::min(searchDepth, 5);
        else if (allocatedTime < 5000) searchDepth = std::min(searchDepth, 6);
        else if (allocatedTime < 10000) searchDepth = std::min(searchDepth, 7);
        else searchDepth = std::min(searchDepth, 8);
    } else if (!infinite) {
        // No time info - use reasonable default
        searchDepth = std::min(searchDepth, 6);
    }

    // Search for best move
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
            // Could clear transposition tables here if implemented
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
    // Initialize piece-square tables
    PSQT::init();
    
    // Disable output buffering for proper UCI communication
    std::cout.setf(std::ios::unitbuf);
    std::cin.tie(nullptr);
    
    uciLoop();
    return 0;
}

