#pragma once
#include <cstdint>

// This file contains the shared evaluation definitions and types for the evaluation functions

namespace Eval {

// Score combining midgame and endgame values
struct Score {
    int16_t mg = 0;
    int16_t eg = 0;
    
    constexpr Score(int16_t middle, int16_t end) : mg(middle), eg(end) {}
    constexpr Score() = default;
};

// Stockfish piece values
constexpr int PAWN_VALUE_MG   = 126;
constexpr int PAWN_VALUE_EG   = 208;
constexpr int KNIGHT_VALUE_MG = 781;
constexpr int KNIGHT_VALUE_EG = 854;
constexpr int BISHOP_VALUE_MG = 825;
constexpr int BISHOP_VALUE_EG = 915;
constexpr int ROOK_VALUE_MG   = 1276;
constexpr int ROOK_VALUE_EG   = 1380;
constexpr int QUEEN_VALUE_MG  = 2538;
constexpr int QUEEN_VALUE_EG  = 2682;

}

