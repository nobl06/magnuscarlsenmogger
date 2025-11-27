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

class Board
{
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
