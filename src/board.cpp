#include "board.h"
#include "move.h"
#include <iostream>

void Board::clear() // clear function (no board)
{
    whitePawns = 0ULL;
    whiteKnights = 0ULL;
    whiteBishops = 0ULL;
    whiteRooks = 0ULL;
    whiteQueens = 0ULL;
    whiteKing = 0ULL;

    blackPawns = 0ULL;
    blackKnights = 0ULL;
    blackBishops = 0ULL;
    blackRooks = 0ULL;
    blackQueens = 0ULL;
    blackKing = 0ULL;
}

void Board::initStartPosition() // initializing the piece position using a bitboard
{
    clear();
    for (int f = 0; f < 8; f++) {
        whitePawns |= bit(f, 1);
    }
    whiteRooks = bit(0, 0) | bit(7, 0);
    whiteKnights = bit(1, 0) | bit(6, 0);
    whiteBishops = bit(2, 0) | bit(5, 0);
    whiteQueens = bit(3, 0);
    whiteKing = bit(4, 0);

    for (int f = 0; f < 8; f++) {
        blackPawns |= bit(f, 6);
    }
    blackRooks = bit(0, 7) | bit(7, 7);
    blackKnights = bit(1, 7) | bit(6, 7);
    blackBishops = bit(2, 7) | bit(5, 7);
    blackQueens = bit(3, 7);
    blackKing = bit(4, 7);
}

void Board::print() const { // printing the board with current positions
    for (int row = 7; row >= 0; --row) {
        std::cout << (row + 1) << "  ";

        for (int column = 0; column < 8; ++column) {
            int squareIndex = row * 8 + column;

            char piece = '.';

            std::uint64_t mask = 1ULL << squareIndex;

            if (whitePawns & mask) piece = 'P';
            if (whiteKnights & mask) piece = 'N';
            if (whiteBishops & mask) piece = 'B';
            if (whiteRooks & mask) piece = 'R';
            if (whiteQueens & mask) piece = 'Q';
            if (whiteKing & mask) piece = 'K';

            if (blackPawns & mask) piece = 'p';
            if (blackKnights & mask) piece = 'n';
            if (blackBishops & mask) piece = 'b';
            if (blackRooks & mask) piece = 'r';
            if (blackQueens & mask) piece = 'q';
            if (blackKing & mask) piece = 'k';

            std::cout << piece << " ";
        }

        std::cout << "\n";
    }

    std::cout << "   a b c d e f g h\n\n";
}

PieceType Board::pieceAt(int square) const {
    std::uint64_t mask = 1ULL << square;

    // White pieces
    if (whitePawns & mask) return PieceType::PAWN;
    if (whiteKnights & mask) return PieceType::KNIGHT;
    if (whiteBishops & mask) return PieceType::BISHOP;
    if (whiteRooks & mask) return PieceType::ROOK;
    if (whiteQueens & mask) return PieceType::QUEEN;
    if (whiteKing & mask) return PieceType::KING;

    // Black pieces
    if (blackPawns & mask) return PieceType::PAWN;
    if (blackKnights & mask) return PieceType::KNIGHT;
    if (blackBishops & mask) return PieceType::BISHOP;
    if (blackRooks & mask) return PieceType::ROOK;
    if (blackQueens & mask) return PieceType::QUEEN;
    if (blackKing & mask) return PieceType::KING;

    return PieceType::EMPTY;
}

Color Board::colorAt(int square) const {
    std::uint64_t mask = 1ULL << square;

    if ((whitePawns | whiteKnights | whiteBishops |
         whiteRooks | whiteQueens | whiteKing) &
        mask)
        return Color::WHITE;

    if ((blackPawns | blackKnights | blackBishops |
         blackRooks | blackQueens | blackKing) &
        mask)
        return Color::BLACK;

    return Color::WHITE;
}

void Board::update_move(Move m) {
    PieceType fpt = pieceAt(m.from);
    Color fc = colorAt(m.from);
    PieceType tpt = pieceAt(m.to);
    Color tc = colorAt(m.to);
    PieceType finaltype = fpt;
    if (m.promotion != PieceType::EMPTY) {
        finaltype = m.promotion;
    }

    std::uint64_t maskFrom = 1ULL << m.from;
    std::uint64_t maskTo = 1ULL << m.to;

    // Removing piece in To square (if there is one)
    whitePawns &= ~maskTo;
    ;
    whiteKnights &= ~maskTo;
    whiteBishops &= ~maskTo;
    whiteRooks &= ~maskTo;
    whiteQueens &= ~maskTo;
    whiteKing &= ~maskTo;

    blackPawns &= ~maskTo;
    blackKnights &= ~maskTo;
    blackBishops &= ~maskTo;
    blackRooks &= ~maskTo;
    blackQueens &= ~maskTo;
    blackKing &= ~maskTo;

    // Removing piece in From square (there should be one since the move is legal)
    if (fc == Color::WHITE) {
        if (fpt == PieceType::PAWN) {
            whitePawns &= ~maskFrom;
        }
        if (fpt == PieceType::KNIGHT) {
            whiteKnights &= ~maskFrom;
        }
        if (fpt == PieceType::BISHOP) {
            whiteBishops &= ~maskFrom;
        }
        if (fpt == PieceType::ROOK) {
            whiteRooks &= ~maskFrom;
        }
        if (fpt == PieceType::QUEEN) {
            whiteQueens &= ~maskFrom;
        }
        if (fpt == PieceType::KING) {
            whiteKing &= ~maskFrom;
        }
    }

    else {
        if (fpt == PieceType::PAWN) {
            blackPawns &= ~maskFrom;
        }
        if (fpt == PieceType::KNIGHT) {
            blackKnights &= ~maskFrom;
        }
        if (fpt == PieceType::BISHOP) {
            blackBishops &= ~maskFrom;
        }
        if (fpt == PieceType::ROOK) {
            blackRooks &= ~maskFrom;
        }
        if (fpt == PieceType::QUEEN) {
            blackQueens &= ~maskFrom;
        }
        if (fpt == PieceType::KING) {
            blackKing &= ~maskFrom;
        }
    }

    // Moving piece to To square
    if (fc == Color::WHITE) {
        if (finaltype == PieceType::PAWN) {
            whitePawns |= maskTo;
        }
        if (finaltype == PieceType::KNIGHT) {
            whiteKnights |= maskTo;
        }
        if (finaltype == PieceType::BISHOP) {
            whiteBishops |= maskTo;
        }
        if (finaltype == PieceType::ROOK) {
            whiteRooks |= maskTo;
        }
        if (finaltype == PieceType::QUEEN) {
            whiteQueens |= maskTo;
        }
        if (finaltype == PieceType::KING) {
            whiteKing |= maskTo;
        }
    }

    else {
        if (finaltype == PieceType::PAWN) {
            blackPawns |= maskTo;
        }
        if (finaltype == PieceType::KNIGHT) {
            blackKnights |= maskTo;
        }
        if (finaltype == PieceType::BISHOP) {
            blackBishops |= maskTo;
        }
        if (finaltype == PieceType::ROOK) {
            blackRooks |= maskTo;
        }
        if (finaltype == PieceType::QUEEN) {
            blackQueens |= maskTo;
        }
        if (finaltype == PieceType::KING) {
            blackKing |= maskTo;
        }
    }
}
