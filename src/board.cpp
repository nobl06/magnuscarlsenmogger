#include "board.h"
#include <iostream>

void Board::clear() //clear function (no board)
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

void Board::initStartPosition() // initializing the piece position using a bitboard
{
    clear();
    for (int f = 0; f < 8; f++) {
        whitePawns |= bit(f, 1);
    }
    whiteRooks   = bit(0, 0) | bit(7, 0);
    whiteKnights = bit(1, 0) | bit(6, 0);
    whiteBishops = bit(2, 0) | bit(5, 0); 
    whiteKing    = bit(4, 0);

    for (int f = 0; f < 8; f++) {
        blackPawns |= bit(f, 6);
    }
    blackRooks   = bit(0, 7) | bit(7, 7);
    blackKnights = bit(1, 7) | bit(6, 7);
    blackBishops = bit(2, 7) | bit(5, 7);
    blackQueens  = bit(3, 7);
    blackKing    = bit(4, 7);
}

void Board::print() const { //printing the board with current positions
    for (int rank = 7; rank >= 0; --rank)
    {
        std::cout << (rank + 1) << "  ";

        for (int column = 0; column < 8; ++column)
        {
            int squareIndex = rank * 8 + column;

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


