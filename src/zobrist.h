#pragma once

#include <cstdint>
#include "board.h"

namespace Zobrist {
    // Zobrist keys for piece positions: [color][pieceType][square]
    extern uint64_t pieceKeys[2][7][64];
    
    // Zobrist key for side to move (XOR when black to move)
    extern uint64_t sideKey;
    
    // Zobrist keys for castling rights (4 bits: WK, WQ, BK, BQ)
    extern uint64_t castlingKeys[16];
    
    // Zobrist keys for en passant file (0-7, file a-h)
    extern uint64_t enPassantKeys[8];
    
    // Initialize all Zobrist keys with random numbers
    void init();
    
    // Compute hash from scratch (for debugging/initialization)
    uint64_t computeHash(const Board& board);
    
    // Helper to get castling rights index
    inline int getCastlingIndex(bool wk, bool wq, bool bk, bool bq) {
        return (wk ? 8 : 0) | (wq ? 4 : 0) | (bk ? 2 : 0) | (bq ? 1 : 0);
    }
}

