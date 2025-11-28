#pragma once
#include <cstdint>
#include <string>

// Piece types
enum class PieceType : uint8_t {
    EMPTY = 0,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

// Colors
enum class Color : uint8_t {
    WHITE = 0,
    BLACK = 1
};

class Board {
  public:
    void clear();
    void initStartPosition();
    void print() const;

    static constexpr int position(int column, int row) { return row * 8 + column; }
    static constexpr int column(int position) { return position % 8; }
    static constexpr int row(int position) { return position / 8; }

    static constexpr std::uint64_t bit(int column, int row) {
        return 1ULL << position(column, row);
    }

    // Bitboard utility functions
    std::uint64_t getAllWhitePieces() const;
    std::uint64_t getAllBlackPieces() const;
    std::uint64_t getAllPieces() const;
    
    bool isSquareEmpty(int pos) const;
    bool isSquareOccupiedByColor(int pos, Color color) const;
    
    // Helper to get least significant bit position
    static int getLsb(std::uint64_t bb);
    // Helper to pop (extract and clear) least significant bit. Helps to not go through each square to find a specific piece
    static int popLsb(std::uint64_t& bb);

    std::uint64_t whitePawns;
    std::uint64_t whiteKnights;
    std::uint64_t whiteBishops;
    std::uint64_t whiteRooks;
    std::uint64_t whiteQueens;
    std::uint64_t whiteKing;

    std::uint64_t blackPawns;
    std::uint64_t blackKnights;
    std::uint64_t blackBishops;
    std::uint64_t blackRooks;
    std::uint64_t blackQueens;
    std::uint64_t blackKing;
};
