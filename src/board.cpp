#include "board.h"
#include <iostream>

void Board::clear()
{
    whitePawns   = 0ULL;
    whiteKnights = 0ULL;
    whiteBishops = 0ULL;
    whiteRooks   = 0ULL;
    whiteQueens  = 0ULL;
    whiteKing    = 0ULL;

    blackPawns   = 0ULL;
    blackKnights = 0ULL;
    blackBishops = 0ULL;
    blackRooks   = 0ULL;
    blackQueens  = 0ULL;
    blackKing    = 0ULL;
}

void Board::initStartPosition()
{
    clear();
    whitePawns   = 0b0000000000000000000000000000000000000000000000001111111100000000ULL;
    whiteRooks   = 0b0000000000000000000000000000000000000000000000000000000010000001ULL;
    whiteKnights = 0b0000000000000000000000000000000000000000000000000000000001000010ULL;
    whiteBishops = 0b0000000000000000000000000000000000000000000000000000000000100100ULL;
    whiteQueens  = 0b0000000000000000000000000000000000000000000000000000000000001000ULL;
    whiteKing    = 0b0000000000000000000000000000000000000000000000000000000000010000ULL;

    blackPawns   = 0b0000000011111111000000000000000000000000000000000000000000000000ULL;
    blackRooks   = 0b1000000100000000000000000000000000000000000000000000000000000000ULL;
    blackKnights = 0b0100001000000000000000000000000000000000000000000000000000000000ULL;
    blackBishops = 0b0010010000000000000000000000000000000000000000000000000000000000ULL;
    blackQueens  = 0b0000100000000000000000000000000000000000000000000000000000000000ULL;
    blackKing    = 0b0001000000000000000000000000000000000000000000000000000000000000ULL;
}

void Board::print() const {
    for (int rank = 7; rank >= 0; --rank)
    {
        std::cout << (rank + 1) << "  ";

        for (int letter = 0; letter < 8; ++letter)
        {
            int squareIndex = rank * 8 + letter;

            char piece = '.';

            std::uint64_t mask = 1ULL << squareIndex;

            if (whitePawns   & mask) piece = 'P';
            if (whiteKnights & mask) piece = 'N';
            if (whiteBishops & mask) piece = 'B';
            if (whiteRooks   & mask) piece = 'R';
            if (whiteQueens  & mask) piece = 'Q';
            if (whiteKing    & mask) piece = 'K';

            if (blackPawns   & mask) piece = 'p';
            if (blackKnights & mask) piece = 'n';
            if (blackBishops & mask) piece = 'b';
            if (blackRooks   & mask) piece = 'r';
            if (blackQueens  & mask) piece = 'q';
            if (blackKing    & mask) piece = 'k';

            std::cout << piece << " ";
        }

        std::cout << "\n";
    }

    std::cout << "   a b c d e f g h\n";
    }

    
