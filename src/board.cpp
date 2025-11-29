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

    // Clear en passant target from previous move
    int oldEnPassant = enPassantTarget;
    enPassantTarget = -1;

    // Handle castling move (king moving 2 squares)
    if (fpt == PieceType::KING && std::abs(static_cast<int>(m.to) - static_cast<int>(m.from)) == 2) {
        int row = Board::row(m.from);
        int fromCol = Board::column(m.from);
        int toCol = Board::column(m.to);
        
        // Kingside castling (king moves right 2 squares)
        if (toCol > fromCol) {
            int rookFrom = position(7, row);  // Rook on h-file
            int rookTo = position(5, row);    // Rook moves to f-file
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;
            
            if (fc == Color::WHITE) {
                whiteRooks &= ~rookMaskFrom;
                whiteRooks |= rookMaskTo;
            } else {
                blackRooks &= ~rookMaskFrom;
                blackRooks |= rookMaskTo;
            }
        }
        // Queenside castling (king moves left 2 squares)
        else {
            int rookFrom = position(0, row);  // Rook on a-file
            int rookTo = position(3, row);    // Rook moves to d-file
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;
            
            if (fc == Color::WHITE) {
                whiteRooks &= ~rookMaskFrom;
                whiteRooks |= rookMaskTo;
            } else {
                blackRooks &= ~rookMaskFrom;
                blackRooks |= rookMaskTo;
            }
        }
    }

    // Handle en passant capture
    if (fpt == PieceType::PAWN && m.to == oldEnPassant) {
        // Remove the captured pawn (which is not on the 'to' square but behind it)
        int capturedPawnSquare = m.to + (fc == Color::WHITE ? -8 : 8);
        std::uint64_t capturedMask = 1ULL << capturedPawnSquare;
        if (fc == Color::WHITE) {
            blackPawns &= ~capturedMask;
        } else {
            whitePawns &= ~capturedMask;
        }
    }

    // Set en passant target if pawn moved 2 squares
    if (fpt == PieceType::PAWN && std::abs(static_cast<int>(m.to) - static_cast<int>(m.from)) == 16) {
        enPassantTarget = m.from + (fc == Color::WHITE ? 8 : -8);
    }

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
    return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
}

std::uint64_t Board::getAllBlackPieces() const {
    return blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
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

void Board::gamestate(const std::vector<std::string>& move_hist) {
    initStartPosition(); //start from initial position

    // Apply every move from the history
    for (const std::string& mv : move_hist) {
        if (mv.size() < 4) continue;  // Apply every move from history 
                                    // skipping invalid lines

        Move m = parseMove(mv);
        update_move(m);
    }
}

bool Board::isSquareAttackedBy(int square, Color attackerColor) const {
    int targetCol = column(square);
    int targetRow = row(square);
    
    std::uint64_t attackerPawns, attackerKnights, attackerBishops;
    std::uint64_t attackerRooks, attackerQueens, attackerKing;
    
    if (attackerColor == Color::WHITE) {
        attackerPawns = whitePawns;
        attackerKnights = whiteKnights;
        attackerBishops = whiteBishops;
        attackerRooks = whiteRooks;
        attackerQueens = whiteQueens;
        attackerKing = whiteKing;
    } else {
        attackerPawns = blackPawns;
        attackerKnights = blackKnights;
        attackerBishops = blackBishops;
        attackerRooks = blackRooks;
        attackerQueens = blackQueens;
        attackerKing = blackKing;
    }
    
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
    const int knightDCol[8] = { 1,  2,  2,  1, -1, -2, -2, -1 };
    const int knightDRow[8] = { 2,  1, -1, -2, -2, -1,  1,  2 };
    for (int i = 0; i < 8; ++i) {
        int newcol = targetCol + knightDCol[i];
        int newrow = targetRow + knightDRow[i];
        if (newcol >= 0 && newcol < 8 && newrow >= 0 && newrow < 8) {
            int knightSq = position(newcol, newrow);
            if (attackerKnights & (1ULL << knightSq)) return true;
        }
    }
    
    // Checks for any possible king attacks
    const int kingDCol[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int kingDRow[8] = { -1,-1,-1,  0, 0,  1, 1, 1 };
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
    const int diagDCol[4] = {  1,  1, -1, -1 };
    const int diagDRow[4] = {  1, -1,  1, -1 };
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
    const int straightDCol[4] = {  0,  0,  1, -1 };
    const int straightDRow[4] = {  1, -1,  0,  0 };
    for (int dir = 0; dir < 4; ++dir) {
        int nc = targetCol + straightDCol[dir];
        int nr = targetRow + straightDRow[dir];
        while (nc >= 0 && nc < 8 && nr >= 0 && nr < 8) {
            int sq = position(nc, nr);
            std::uint64_t sqBit = 1ULL << sq;
            
            if (straightAttackers & sqBit) return true;
            if (allOccupied & sqBit) break;  // Blocked
            
            nc += straightDCol[dir];
            nr += straightDRow[dir];
        }
    }
    
    return false;
}

bool Board::isKingInCheck(Color kingColor) const {
    // Find the king position
    std::uint64_t king = (kingColor == Color::WHITE) ? whiteKing : blackKing;
    
    int kingSq = getLsb(king);
    
    // Check if the king square is attacked by the opponent
    Color opponent = (kingColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    return isSquareAttackedBy(kingSq, opponent);
}
