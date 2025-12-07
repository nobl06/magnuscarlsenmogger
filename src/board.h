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

// state to save/restore when making/unmaking moves
struct BoardState {
    PieceType capturedPiece;
    Color capturedColor;
    int enPassantTarget;
    bool whiteCanKingside;
    bool whiteCanQueenside;
    bool blackCanKingside;
    bool blackCanQueenside;
};

class Board {
  public:
    void clear();
    void initStartPosition();
    void print() const;
    PieceType pieceAt(int square) const;
    Color colorAt(int square) const;
    void update_move(Move m);
    void gamestate(const std::vector<std::string> &move_hist);
    
    // make/unmake functions
    BoardState makeMove(const Move& m);
    void unmakeMove(const Move& m, const BoardState& state);

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
    // Helper to get most significant bit position
    static int getMsb(std::uint64_t bb);
    // Helper to pop (extract and clear) least significant bit. Helps to not go through each square to find a specific piece
    static int popLsb(std::uint64_t &bb);

    // Bitboard utility functions
    static int popcount(uint64_t bb);
    static bool moreThanOne(uint64_t bb);
    static uint64_t shiftUp(uint64_t bb);
    static uint64_t shiftDown(uint64_t bb);
    static uint64_t shiftRight(uint64_t bb);
    static uint64_t shiftLeft(uint64_t bb);
    static uint64_t columnBB(int column);
    static uint64_t rowBB(int row);
    static uint64_t adjacentColumnsBB(int column);
    
    // Distance functions
    static int distance(int sq1, int sq2);
    static int columnDistance(int sq1, int sq2);
    
    // Attack generation
    static uint64_t getKnightAttacks(int square);
    static uint64_t getBishopAttacks(int square, uint64_t occupied);
    static uint64_t getRookAttacks(int square, uint64_t occupied);
    static uint64_t getQueenAttacks(int square, uint64_t occupied);
    static uint64_t getKingAttacks(int square);
    static uint64_t getPawnAttacks(uint64_t pawns, Color color);
    
    // Board utilities
    static uint64_t forwardRowsBB(Color color, int square);
    static bool isOnSemiOpenFile(const Board& board, Color color, int column);

    // bitboards[Color][PieceType]
    uint64_t bitboards[2][7];

    // Game state tracking
    bool whiteCanKingside;
    bool whiteCanQueenside;
    bool blackCanKingside;
    bool blackCanQueenside;
    int enPassantTarget; // -1 if none, otherwise square index (0-63)
    Color sideToMove;
    
    // Zobrist hash key
    uint64_t hashKey;
    
    // History for repetition detection
    std::vector<uint64_t> hashHistory;
    
    // Compute hash from scratch (for debugging)
    uint64_t computeHash() const;
    
    // Check for threefold repetition
    bool isThreefoldRepetition() const;
    
    // Get the ply since last irreversible move (for repetition detection)
    int getPlySinceIrreversible() const;

    // Attack and check detection
    bool isSquareAttackedBy(int square, Color attackerColor) const;
    bool isKingInCheck(Color kingColor) const;
    uint64_t getAttackedSquares(Color color) const;
};