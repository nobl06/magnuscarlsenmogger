#pragma once
#include <cstdint>

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
