#pragma once
#include "../board.h"
#include <cstdint>
#include <utility>

namespace PSQT {

// Score type combining middle game and endgame evaluations
struct Score {
    int16_t mg = 0;
    int16_t eg = 0;
    
    constexpr Score(int16_t middle, int16_t end) : mg(middle), eg(end) {}
    constexpr Score() = default;
};

// Piece-square table: [piece_type][color][square]
extern Score psqTable[7][2][64];
// Initialize the piece-square tables
void init();

// Get the piece-square bonus for a given piece, color, and square
inline Score getScore(PieceType piece, Color color, int square) {
    return psqTable[static_cast<int>(piece)][static_cast<int>(color)][square];
}

// Evaluate all piece-square bonuses for the current board position
std::pair<int, int> evaluatePSQT(const Board& board);

}

