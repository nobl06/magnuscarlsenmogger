#include "zobrist.h"
#include <random>

namespace Zobrist {
    // Define the arrays
    uint64_t pieceKeys[2][7][64];
    uint64_t sideKey;
    uint64_t castlingKeys[16];
    uint64_t enPassantKeys[8];
    
    // Use a fixed seed for reproducibility (which can be changed)
    constexpr uint64_t SEED = 0x123456789ABCDEFULL;
    
    void init() {
        // Initialize random number generator with fixed seed
        std::mt19937_64 rng(SEED);
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
        
        // Initialize piece keys
        for (int color = 0; color < 2; ++color) {
            for (int piece = PAWN; piece <= KING; ++piece) {
                for (int square = 0; square < 64; ++square) {
                    pieceKeys[color][piece][square] = dist(rng);
                }
            }
        }
        
        // Initialize side to move key
        sideKey = dist(rng);
        
        // Initialize castling keys (16 possible combinations)
        for (int i = 0; i < 16; ++i) {
            castlingKeys[i] = dist(rng);
        }
        
        // Initialize en passant keys (8 files)
        for (int file = 0; file < 8; ++file) {
            enPassantKeys[file] = dist(rng);
        }
    }
    
    uint64_t computeHash(const Board& board) {
        uint64_t hash = 0ULL;
        
        // Hash all pieces
        for (int color = 0; color < 2; ++color) {
            for (int pieceType = PAWN; pieceType <= KING; ++pieceType) {
                uint64_t pieces = board.bitboards[color][pieceType];
                while (pieces) {
                    int square = Board::popLsb(pieces);
                    hash ^= pieceKeys[color][pieceType][square];
                }
            }
        }
        
        // Hash side to move (XOR if black)
        if (board.sideToMove == BLACK) {
            hash ^= sideKey;
        }
        
        // Hash castling rights
        int castlingIndex = getCastlingIndex(
            board.whiteCanKingside,
            board.whiteCanQueenside,
            board.blackCanKingside,
            board.blackCanQueenside
        );
        hash ^= castlingKeys[castlingIndex];
        
        // Hash en passant file (if any)
        if (board.enPassantTarget != -1) {
            int epFile = Board::column(board.enPassantTarget);
            hash ^= enPassantKeys[epFile];
        }
        
        return hash;
    }
}

