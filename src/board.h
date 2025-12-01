#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Piece types
enum PieceType : uint8_t {
    EMPTY = 0,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

// Colors
enum Color : uint8_t {
    WHITE = 0,
    BLACK = 1
};

// Forward declare Move
struct Move;

class Board {
  public:
    void clear();
    void initStartPosition();
    void print() const;
    PieceType pieceAt(int square) const;
    Color colorAt(int square) const;
    void update_move(Move m);
    void gamestate(const std::vector<std::string> &move_hist);

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
    static int popLsb(std::uint64_t &bb);

    // bitboards[Color][PieceType]
    uint64_t bitboards[2][7];

    // Game state tracking
    bool whiteCanKingside;
    bool whiteCanQueenside;
    bool blackCanKingside;
    bool blackCanQueenside;
    int enPassantTarget; // -1 if none, otherwise square index (0-63)
    Color sideToMove;

    // Attack and check detection
    bool isSquareAttackedBy(int square, Color attackerColor) const;
    bool isKingInCheck(Color kingColor) const;
};
