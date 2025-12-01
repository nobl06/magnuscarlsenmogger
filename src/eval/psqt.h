#pragma once
#include "../board.h"
#include "defs.h"
#include <cstdint>
#include <utility>

namespace PSQT {

using Score = Eval::Score;

// Piece-square table: [piece_type][color][square]
extern Score psqTable[7][2][64];
void init();

// Get the piece-square bonus for a given piece, color, and square
inline Score getScore(PieceType piece, Color color, int square) {
    return psqTable[piece][color][square];
}

// Evaluate all piece-square bonuses for the current board position
std::pair<int, int> evaluatePSQT(const Board& board);

}

