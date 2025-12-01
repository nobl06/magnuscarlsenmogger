#include "board.h"
#include "move.h"
#include <cstdlib>
#include <iostream>

void Board::clear() // clear function (no board)
{
    // Clear all bitboards
    for (int c = 0; c < 2; ++c) {
        for (int p = 0; p < 7; ++p) {
            bitboards[c][p] = 0ULL;
        }
    }

    // Initialize game state
    whiteCanKingside = false;
    whiteCanQueenside = false;
    blackCanKingside = false;
    blackCanQueenside = false;
    enPassantTarget = -1;
    sideToMove = Color::WHITE;
}

void Board::initStartPosition() // initializing the piece position using a bitboard
{
    clear();

    // White pieces
    for (int f = 0; f < 8; f++) {
        bitboards[WHITE][PAWN] |= bit(f, 1);
    }

    bitboards[WHITE][ROOK] = bit(0, 0) | bit(7, 0);
    bitboards[WHITE][KNIGHT] = bit(1, 0) | bit(6, 0);
    bitboards[WHITE][BISHOP] = bit(2, 0) | bit(5, 0);
    bitboards[WHITE][QUEEN] = bit(3, 0);
    bitboards[WHITE][KING] = bit(4, 0);

    // Black pieces
    for (int f = 0; f < 8; f++) {
        bitboards[BLACK][PAWN] |= bit(f, 6);
    }

    bitboards[BLACK][ROOK] = bit(0, 7) | bit(7, 7);
    bitboards[BLACK][KNIGHT] = bit(1, 7) | bit(6, 7);
    bitboards[BLACK][BISHOP] = bit(2, 7) | bit(5, 7);
    bitboards[BLACK][QUEEN] = bit(3, 7);
    bitboards[BLACK][KING] = bit(4, 7);

    // Set initial game state
    whiteCanKingside = true;
    whiteCanQueenside = true;
    blackCanKingside = true;
    blackCanQueenside = true;
    enPassantTarget = -1;
    sideToMove = Color::WHITE;
}

void Board::print() const { // printing the board with current positions
    for (int row = 7; row >= 0; --row) {
        std::cout << (row + 1) << "  ";

        for (int column = 0; column < 8; ++column) {
            int squareIndex = row * 8 + column;

            char piece = '.';

            std::uint64_t mask = 1ULL << squareIndex;

            // White pieces
            for (int pt = PAWN; pt <= KING; ++pt)
                if (bitboards[WHITE][pt] & mask) {
                    switch (pt) {
                    case PAWN:
                        piece = 'P';
                        break;
                    case KNIGHT:
                        piece = 'N';
                        break;
                    case BISHOP:
                        piece = 'B';
                        break;
                    case ROOK:
                        piece = 'R';
                        break;
                    case QUEEN:
                        piece = 'Q';
                        break;
                    case KING:
                        piece = 'K';
                        break;
                    }
                }

            // Black pieces
            for (int pt = PAWN; pt <= KING; ++pt)
                if (bitboards[BLACK][pt] & mask) {
                    switch (pt) {
                    case PAWN:
                        piece = 'p';
                        break;
                    case KNIGHT:
                        piece = 'n';
                        break;
                    case BISHOP:
                        piece = 'b';
                        break;
                    case ROOK:
                        piece = 'r';
                        break;
                    case QUEEN:
                        piece = 'q';
                        break;
                    case KING:
                        piece = 'k';
                        break;
                    }
                }

            std::cout << piece << " ";
        }

        std::cout << "\n";
    }

    std::cout << "   a b c d e f g h\n\n";
}

PieceType Board::pieceAt(int square) const {
    std::uint64_t mask = 1ULL << square;

    for (int pt = PAWN; pt <= KING; ++pt) {
        if (bitboards[WHITE][pt] & mask) return (PieceType)pt;
        if (bitboards[BLACK][pt] & mask) return (PieceType)pt;
    }

    return PieceType::EMPTY;
}

Color Board::colorAt(int square) const {
    std::uint64_t mask = 1ULL << square;

    if (getAllWhitePieces() & mask)
        return Color::WHITE;

    if (getAllBlackPieces() & mask)
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

    // Clear en passant target from previous move
    int oldEnPassant = enPassantTarget;
    enPassantTarget = -1;

    // Handle castling move (king moving 2 squares)
    if (fpt == PieceType::KING && std::abs(m.to - m.from) == 2) {
        int row = Board::row(m.from);
        int fromCol = Board::column(m.from);
        int toCol = Board::column(m.to);

        // Kingside castling (king moves right 2 squares)
        if (toCol > fromCol) {
            int rookFrom = position(7, row); // Rook on h-file
            int rookTo = position(5, row);   // Rook moves to f-file
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;

            bitboards[fc][ROOK] &= ~rookMaskFrom;
            bitboards[fc][ROOK] |= rookMaskTo;
        }
        // Queenside castling (king moves left 2 squares)
        else {
            int rookFrom = position(0, row); // Rook on a-file
            int rookTo = position(3, row);   // Rook moves to d-file
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;

            bitboards[fc][ROOK] &= ~rookMaskFrom;
            bitboards[fc][ROOK] |= rookMaskTo;
        }
    }

    // Handle en passant capture
    if (fpt == PieceType::PAWN && m.to == oldEnPassant) {
        // Remove the captured pawn (which is not on the 'to' square but behind it)
        int capturedPawnSquare = m.to + (fc == Color::WHITE ? -8 : 8);
        std::uint64_t capturedMask = 1ULL << capturedPawnSquare;
        bitboards[fc == WHITE ? BLACK : WHITE][PAWN] &= ~capturedMask;
    }

    // Set en passant target if pawn moved 2 squares
    if (fpt == PieceType::PAWN && std::abs(static_cast<int>(m.to) - static_cast<int>(m.from)) == 16) {
        enPassantTarget = m.from + (fc == Color::WHITE ? 8 : -8);
    }

    // Removing piece in To square (if there is one)
    for (int c = 0; c < 2; ++c)
        for (int pt = PAWN; pt <= KING; ++pt)
            bitboards[c][pt] &= ~maskTo;

    // Removing piece in From square (there should be one since the move is legal)
    bitboards[fc][fpt] &= ~maskFrom;

    // Moving piece to To square
    bitboards[fc][finaltype] |= maskTo;

    // Update castling rights
    if (fpt == PieceType::KING) {
        if (fc == Color::WHITE) {
            whiteCanKingside = false;
            whiteCanQueenside = false;
        } else {
            blackCanKingside = false;
            blackCanQueenside = false;
        }
    }

    // If rook moves from starting position, remove that castling right
    if (fpt == PieceType::ROOK) {
        if (fc == Color::WHITE) {
            if (m.from == position(0, 0)) whiteCanQueenside = false;
            if (m.from == position(7, 0)) whiteCanKingside = false;
        } else {
            if (m.from == position(0, 7)) blackCanQueenside = false;
            if (m.from == position(7, 7)) blackCanKingside = false;
        }
    }

    // If rook is captured on starting square, remove that castling right
    if (tpt == PieceType::ROOK) {
        if (m.to == position(0, 0)) whiteCanQueenside = false;
        if (m.to == position(7, 0)) whiteCanKingside = false;
        if (m.to == position(0, 7)) blackCanQueenside = false;
        if (m.to == position(7, 7)) blackCanKingside = false;
    }

    // Toggle side to move
    sideToMove = (sideToMove == Color::WHITE) ? Color::BLACK : Color::WHITE;
}

std::uint64_t Board::getAllWhitePieces() const {
    uint64_t bb = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt)
        bb |= bitboards[WHITE][pt];
    return bb;
}

std::uint64_t Board::getAllBlackPieces() const {
    uint64_t bb = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt)
        bb |= bitboards[BLACK][pt];
    return bb;
}

std::uint64_t Board::getAllPieces() const {
    return getAllWhitePieces() | getAllBlackPieces();
}

bool Board::isSquareEmpty(int pos) const {
    std::uint64_t mask = 1ULL << pos;
    return (getAllPieces() & mask) == 0;
}

bool Board::isSquareOccupiedByColor(int pos, Color color) const {
    std::uint64_t mask = 1ULL << pos;
    if (color == Color::WHITE) {
        return (getAllWhitePieces() & mask) != 0;
    } else {
        return (getAllBlackPieces() & mask) != 0;
    }
}

int Board::getLsb(std::uint64_t bb) {
    // Simple portable version - count trailing zeros
    int count = 0;
    while ((bb & 1) == 0) {
        bb >>= 1;
        count++;
    }
    return count;
}

int Board::popLsb(std::uint64_t &bb) {
    int pos = getLsb(bb);
    bb &= bb - 1; // Clear the least significant bit
    return pos;
}

void Board::gamestate(const std::vector<std::string> &move_hist) {
    initStartPosition(); // start from initial position

    // Apply every move from the history
    for (const std::string &mv : move_hist) {
        if (mv.size() < 4) continue; // Apply every move from history
                                     // skipping invalid lines
        Move m = parseMove(mv);
        update_move(m);
    }
}

bool Board::isSquareAttackedBy(int square, Color attackerColor) const {
    int targetCol = column(square);
    int targetRow = row(square);

    uint64_t attackerPawns = bitboards[attackerColor][PAWN];
    uint64_t attackerKnights = bitboards[attackerColor][KNIGHT];
    uint64_t attackerBishops = bitboards[attackerColor][BISHOP];
    uint64_t attackerRooks = bitboards[attackerColor][ROOK];
    uint64_t attackerQueens = bitboards[attackerColor][QUEEN];
    uint64_t attackerKing = bitboards[attackerColor][KING];

    // Check for pawn attacks
    int pawnDir = (attackerColor == Color::WHITE) ? 1 : -1;
    if (targetRow - pawnDir >= 0 && targetRow - pawnDir < 8) {
        // Check left diagonal
        if (targetCol > 0) {
            int pawnSq = position(targetCol - 1, targetRow - pawnDir);
            if (attackerPawns & (1ULL << pawnSq)) return true;
        }
        // Check right diagonal
        if (targetCol < 7) {
            int pawnSq = position(targetCol + 1, targetRow - pawnDir);
            if (attackerPawns & (1ULL << pawnSq)) return true;
        }
    }

    // checks for any possible knight attacks
    const int knightDCol[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    const int knightDRow[8] = {2, 1, -1, -2, -2, -1, 1, 2};
    for (int i = 0; i < 8; ++i) {
        int newcol = targetCol + knightDCol[i];
        int newrow = targetRow + knightDRow[i];
        if (newcol >= 0 && newcol < 8 && newrow >= 0 && newrow < 8) {
            int knightSq = position(newcol, newrow);
            if (attackerKnights & (1ULL << knightSq)) return true;
        }
    }

    // Checks for any possible king attacks
    const int kingDCol[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int kingDRow[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (int i = 0; i < 8; ++i) {
        int newcol = targetCol + kingDCol[i];
        int newrow = targetRow + kingDRow[i];
        if (newcol >= 0 && newcol < 8 && newrow >= 0 && newrow < 8) {
            int kingSq = position(newcol, newrow);
            if (attackerKing & (1ULL << kingSq)) return true;
        }
    }

    std::uint64_t allOccupied = getAllPieces();

    // Check for bishop/queen diagonal attacks
    std::uint64_t diagonalAttackers = attackerBishops | attackerQueens;
    const int diagDCol[4] = {1, 1, -1, -1};
    const int diagDRow[4] = {1, -1, 1, -1};
    for (int dir = 0; dir < 4; ++dir) {
        int newcol = targetCol + diagDCol[dir];
        int newrow = targetRow + diagDRow[dir];
        while (newcol >= 0 && newcol < 8 && newrow >= 0 && newrow < 8) {
            int sq = position(newcol, newrow);
            std::uint64_t sqBit = 1ULL << sq;

            if (diagonalAttackers & sqBit) return true;
            if (allOccupied & sqBit) break;

            newcol += diagDCol[dir];
            newrow += diagDRow[dir];
        }
    }

    // Check for rook/queen straight attacks
    std::uint64_t straightAttackers = attackerRooks | attackerQueens;
    const int straightDCol[4] = {0, 0, 1, -1};
    const int straightDRow[4] = {1, -1, 0, 0};
    for (int dir = 0; dir < 4; ++dir) {
        int nc = targetCol + straightDCol[dir];
        int nr = targetRow + straightDRow[dir];
        while (nc >= 0 && nc < 8 && nr >= 0 && nr < 8) {
            int sq = position(nc, nr);
            std::uint64_t sqBit = 1ULL << sq;

            if (straightAttackers & sqBit) return true;
            if (allOccupied & sqBit) break; // Blocked

            nc += straightDCol[dir];
            nr += straightDRow[dir];
        }
    }

    return false;
}

bool Board::isKingInCheck(Color kingColor) const {
    // Find the king position
    std::uint64_t king = bitboards[kingColor][KING];

    int kingSq = getLsb(king);

    // Check if the king square is attacked by the opponent
    Color opponent = (kingColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    return isSquareAttackedBy(kingSq, opponent);
}
