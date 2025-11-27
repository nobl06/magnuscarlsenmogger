#pragma once
#include <cstdint>



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

class Board
{
public:
    void clear();
    void initStartPosition();
    static constexpr int sq(int file, int rank) { return rank * 8 + file; }
    static constexpr int file(int sq) { return sq % 8; }
    static constexpr int rank(int sq) { return sq / 8; }

    static constexpr uint64_t bit(int file, int rank) {
        return 1ULL << sq(file, rank);
    }
    

private:
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

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;

    Move() = default;
    
    Move(uint8_t f, uint8_t t) 
        : from(f), to(t) {}
    
    std::string toString() const;
};
