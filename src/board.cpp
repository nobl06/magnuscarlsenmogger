#include "board.h"
#include "move.h"
#include "zobrist.h"
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

    //Clear cached bitboards
    whitePiecesBB = 0ULL;
    blackPiecesBB = 0ULL;
    allPiecesBB = 0ULL;

    // Initialize game state
    whiteCanKingside = false;
    whiteCanQueenside = false;
    blackCanKingside = false;
    blackCanQueenside = false;
    enPassantTarget = -1;
    sideToMove = Color::WHITE;
    
    // Initialize hash
    hashKey = 0ULL;
    hashHistory.clear();
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
    
    whitePiecesBB = 0ULL;
    blackPiecesBB = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt) {
        whitePiecesBB |= bitboards[WHITE][pt];
        blackPiecesBB |= bitboards[BLACK][pt];
    }
    allPiecesBB = whitePiecesBB | blackPiecesBB;
    
    // Compute initial hash
    hashKey = Zobrist::computeHash(*this);
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

    return Color::NO_COLOR;
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

    // Update cached bitboards
    updateCachedBitboards();

    // Update hash
    hashKey = Zobrist::computeHash(*this);
}

uint64_t Board::computeHash() const {
    return Zobrist::computeHash(*this);
}

bool Board::isThreefoldRepetition() const {
    // Count occurrences of current position
    int count = 0;
    
    // Search backwards through history
    // We can only have repetition after reversible moves
    // (moves that don't capture, move pawns, or change castling rights)
    for (size_t i = hashHistory.size(); i > 0; --i) {
        if (hashHistory[i - 1] == hashKey) {
            count++;
            if (count >= 2) {  // Current position + 2 previous = 3 times
                return true;
            }
        }
        
        // Optimization: we can stop at the last irreversible move
        // For now, we check all history (we'll optimize this later)
    }
    
    return false;
}

int Board::getPlySinceIrreversible() const {
    // For now, return history size (can be optimized with halfmove counter)
    return static_cast<int>(hashHistory.size());
}

BoardState Board::makeMove(const Move& m) {
    // Save state for unmake
    BoardState state;
    state.capturedPiece = pieceAt(m.to);
    state.capturedColor = colorAt(m.to);
    state.enPassantTarget = enPassantTarget;
    state.whiteCanKingside = whiteCanKingside;
    state.whiteCanQueenside = whiteCanQueenside;
    state.blackCanKingside = blackCanKingside;
    state.blackCanQueenside = blackCanQueenside;
    
    // Push current hash to history before making the move
    hashHistory.push_back(hashKey);
    
    // Get move info
    PieceType fpt = pieceAt(m.from);
    Color fc = colorAt(m.from);
    PieceType finaltype = (m.promotion != PieceType::EMPTY) ? m.promotion : fpt;
    
    std::uint64_t maskFrom = 1ULL << m.from;
    std::uint64_t maskTo = 1ULL << m.to;
    
    // XOR out old castling rights
    int oldCastlingIndex = Zobrist::getCastlingIndex(
        whiteCanKingside, whiteCanQueenside, blackCanKingside, blackCanQueenside
    );
    hashKey ^= Zobrist::castlingKeys[oldCastlingIndex];
    
    // XOR out old en passant
    if (enPassantTarget != -1) {
        int oldEpFile = column(enPassantTarget);
        hashKey ^= Zobrist::enPassantKeys[oldEpFile];
    }
    
    // Clear en passant target
    int oldEnPassant = enPassantTarget;
    enPassantTarget = -1;
    
    // XOR out piece from source square
    hashKey ^= Zobrist::pieceKeys[fc][fpt][m.from];
    
    // Handle captures (XOR out captured piece)
    if (state.capturedPiece != PieceType::EMPTY) {
        hashKey ^= Zobrist::pieceKeys[state.capturedColor][state.capturedPiece][m.to];
    }
    
    // Handle castling move (king moving 2 squares)
    if (fpt == PieceType::KING && std::abs(m.to - m.from) == 2) {
        int row = Board::row(m.from);
        int fromCol = Board::column(m.from);
        int toCol = Board::column(m.to);
        
        // kingside castling
        if (toCol > fromCol) {
            int rookFrom = position(7, row);
            int rookTo = position(5, row);
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;
            
            // XOR out rook from old square, XOR in to new square
            hashKey ^= Zobrist::pieceKeys[fc][ROOK][rookFrom];
            hashKey ^= Zobrist::pieceKeys[fc][ROOK][rookTo];
            
            bitboards[fc][ROOK] &= ~rookMaskFrom;
            bitboards[fc][ROOK] |= rookMaskTo;
        }

        // queenside castling
        else {
            int rookFrom = position(0, row);
            int rookTo = position(3, row);
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;
            
            // XOR out rook from old square, XOR in to new square
            hashKey ^= Zobrist::pieceKeys[fc][ROOK][rookFrom];
            hashKey ^= Zobrist::pieceKeys[fc][ROOK][rookTo];
            
            bitboards[fc][ROOK] &= ~rookMaskFrom;
            bitboards[fc][ROOK] |= rookMaskTo;
        }
    }
    
    // en passant capture
    if (fpt == PieceType::PAWN && m.to == oldEnPassant) {
        int capturedPawnSquare = m.to + (fc == Color::WHITE ? -8 : 8);
        std::uint64_t capturedMask = 1ULL << capturedPawnSquare;
        
        // XOR out the captured pawn
        Color enemyColor = (fc == WHITE) ? BLACK : WHITE;
        hashKey ^= Zobrist::pieceKeys[enemyColor][PAWN][capturedPawnSquare];
        
        bitboards[enemyColor][PAWN] &= ~capturedMask;
    }
    
    // set en passant target if pawn moved 2 squares
    if (fpt == PieceType::PAWN && std::abs(static_cast<int>(m.to) - static_cast<int>(m.from)) == 16) {
        enPassantTarget = m.from + (fc == Color::WHITE ? 8 : -8);
    }
    
    // remove piece at destination (capture)
    if (state.capturedPiece != PieceType::EMPTY) {
        bitboards[state.capturedColor][state.capturedPiece] &= ~maskTo;
    }
    
    // remove piece from source
    bitboards[fc][fpt] &= ~maskFrom;
    
    // place piece at destination (XOR in the piece)
    bitboards[fc][finaltype] |= maskTo;
    hashKey ^= Zobrist::pieceKeys[fc][finaltype][m.to];
    
    // Handle promotion (we already XORed out the pawn, now XOR in the promoted piece)
    // (already handled above with finaltype)
    
    // update castling rights
    if (fpt == PieceType::KING) {
        if (fc == Color::WHITE) {
            whiteCanKingside = false;
            whiteCanQueenside = false;
        } else {
            blackCanKingside = false;
            blackCanQueenside = false;
        }
    }
    
    // if rook moves from starting position
    if (fpt == PieceType::ROOK) {
        if (fc == Color::WHITE) {
            if (m.from == position(0, 0)) whiteCanQueenside = false;
            if (m.from == position(7, 0)) whiteCanKingside = false;
        } else {
            if (m.from == position(0, 7)) blackCanQueenside = false;
            if (m.from == position(7, 7)) blackCanKingside = false;
        }
    }
    
    // if rook is captured on starting square
    if (state.capturedPiece == PieceType::ROOK) {
        if (m.to == position(0, 0)) whiteCanQueenside = false;
        if (m.to == position(7, 0)) whiteCanKingside = false;
        if (m.to == position(0, 7)) blackCanQueenside = false;
        if (m.to == position(7, 7)) blackCanKingside = false;
    }
    
    // XOR in new castling rights
    int newCastlingIndex = Zobrist::getCastlingIndex(
        whiteCanKingside, whiteCanQueenside, blackCanKingside, blackCanQueenside
    );
    hashKey ^= Zobrist::castlingKeys[newCastlingIndex];
    
    // XOR in new en passant
    if (enPassantTarget != -1) {
        int newEpFile = column(enPassantTarget);
        hashKey ^= Zobrist::enPassantKeys[newEpFile];
    }
    
    // toggle side to move
    sideToMove = (sideToMove == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    // XOR side to move
    hashKey ^= Zobrist::sideKey;
    

    // Update cached bitboards
    updateCachedBitboards();

    return state;
}

void Board::unmakeMove(const Move& m, const BoardState& state) {
    // Pop hash from history
    if (!hashHistory.empty()) {
        hashKey = hashHistory.back();
        hashHistory.pop_back();
    }
    
    // toggle side to move back
    sideToMove = (sideToMove == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    Color fc = sideToMove;  // now it's back to the original side
    PieceType fpt = pieceAt(m.to);  // the piece is now at 'to'
    PieceType originalPiece = (m.promotion != PieceType::EMPTY) ? PAWN : fpt;
    
    std::uint64_t maskFrom = 1ULL << m.from;
    std::uint64_t maskTo = 1ULL << m.to;
    
    // undo castling rook move
    if (originalPiece == PieceType::KING && std::abs(m.to - m.from) == 2) {
        int row = Board::row(m.from);
        int fromCol = Board::column(m.from);
        int toCol = Board::column(m.to);
        
        // kingside castling
        if (toCol > fromCol) {
            int rookFrom = position(7, row);
            int rookTo = position(5, row);
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;
            
            bitboards[fc][ROOK] &= ~rookMaskTo;
            bitboards[fc][ROOK] |= rookMaskFrom;
        }
        // queenside castling
        else {
            int rookFrom = position(0, row);
            int rookTo = position(3, row);
            std::uint64_t rookMaskFrom = 1ULL << rookFrom;
            std::uint64_t rookMaskTo = 1ULL << rookTo;
            
            bitboards[fc][ROOK] &= ~rookMaskTo;
            bitboards[fc][ROOK] |= rookMaskFrom;
        }
    }
    
    // undo en passant capture
    if (originalPiece == PieceType::PAWN && m.to == state.enPassantTarget) {
        int capturedPawnSquare = m.to + (fc == Color::WHITE ? -8 : 8);
        std::uint64_t capturedMask = 1ULL << capturedPawnSquare;
        bitboards[fc == WHITE ? BLACK : WHITE][PAWN] |= capturedMask;
    }
    
    // remove piece from destination
    bitboards[fc][fpt] &= ~maskTo;
    
    // restore piece at source
    bitboards[fc][originalPiece] |= maskFrom;
    
    // restore captured piece
    if (state.capturedPiece != PieceType::EMPTY) {
        bitboards[state.capturedColor][state.capturedPiece] |= maskTo;
    }
    
    // restore state
    enPassantTarget = state.enPassantTarget;
    whiteCanKingside = state.whiteCanKingside;
    whiteCanQueenside = state.whiteCanQueenside;
    blackCanKingside = state.blackCanKingside;
    blackCanQueenside = state.blackCanQueenside;

    // Update cached bitboards
    updateCachedBitboards();
}

std::uint64_t Board::getAllWhitePieces() const {
    return whitePiecesBB;
}

std::uint64_t Board::getAllBlackPieces() const {
    return blackPiecesBB;
}

std::uint64_t Board::getAllPieces() const {
    return allPiecesBB;
}

void Board::updateCachedBitboards() {
    whitePiecesBB = 0ULL;
    blackPiecesBB = 0ULL;
    for (int pt = PAWN; pt <= KING; ++pt) {
        whitePiecesBB |= bitboards[WHITE][pt];
        blackPiecesBB |= bitboards[BLACK][pt];
    }
    allPiecesBB = whitePiecesBB | blackPiecesBB;
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
    #if defined(__GNUC__) || defined(__clang__)
        // Use builtin function for better performance
        return __builtin_ctzll(bb);
    #else
        // Fallback: count trailing zeros manually
        int count = 0;
        while ((bb & 1) == 0) {
            bb >>= 1;
            count++;
        }
        return count;
    #endif
}

int Board::getMsb(std::uint64_t bb) {
    #if defined(__GNUC__) || defined(__clang__)
        // Use builtin function for better performance
        if (bb == 0) return -1;
        return 63 - __builtin_clzll(bb);
    #else
        // Fallback: find most significant bit manually
        if (bb == 0) return -1;
        int msb = 0;
        while (bb >>= 1) {
            msb++;
        }
        return msb;
    #endif
}

int Board::popLsb(std::uint64_t &bb) {
    #if defined(__GNUC__) || defined(__clang__)
        // Use builtin function for better performance
        int pos = __builtin_ctzll(bb);
        bb &= bb - 1; // Clear the least significant bit
        return pos;
    #else
        // Fallback: use getLsb
        int pos = getLsb(bb);
        bb &= bb - 1; // Clear the least significant bit
        return pos;
    #endif
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

// Bitboard Utility Functions

int Board::popcount(uint64_t bb) {
    #if defined(__GNUC__) || defined(__clang__)
        // Use builtin function for better performance
        return __builtin_popcountll(bb);
    #else
        // Fallback: SWAR algorithm (parallel bit counting)
        bb = bb - ((bb >> 1) & 0x5555555555555555ULL);
        bb = (bb & 0x3333333333333333ULL) + ((bb >> 2) & 0x3333333333333333ULL);
        bb = (bb + (bb >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
        return (bb * 0x0101010101010101ULL) >> 56;
    #endif
}

bool Board::moreThanOne(uint64_t bb) {
    return bb & (bb - 1);
}

uint64_t Board::shiftUp(uint64_t bb) {
    return bb << 8;
}

uint64_t Board::shiftDown(uint64_t bb) {
    return bb >> 8;
}
//THe FEFE is a mask to prevent wrapping around the edges of the board
uint64_t Board::shiftRight(uint64_t bb) {
    return (bb & 0xFEFEFEFEFEFEFEFEULL) << 1;
}
//Same here with 7F7F... = 01111111 01111111 01111111 .. in binary
uint64_t Board::shiftLeft(uint64_t bb) {
    return (bb & 0x7F7F7F7F7F7F7F7FULL) >> 1;
}
// 010101.. = 00000001 00000001 00000001 .. in binary
uint64_t Board::columnBB(int column) {
    return 0x0101010101010101ULL << column;
}

uint64_t Board::rowBB(int row) {
    return 0xFFULL << (row * 8);
}

uint64_t Board::adjacentColumnsBB(int column) {
    uint64_t result = 0;
    if (column > 0) result |= columnBB(column - 1);
    if (column < 7) result |= columnBB(column + 1);
    return result;
}

// Distance Functions

int Board::distance(int sq1, int sq2) {
    int col1 = column(sq1);
    int row1 = row(sq1);
    int col2 = column(sq2);
    int row2 = row(sq2);
    return std::max(std::abs(col1 - col2), std::abs(row1 - row2));
}

int Board::columnDistance(int sq1, int sq2) {
    return std::abs(column(sq1) - column(sq2));
}

// Attack Generation Functions

uint64_t Board::getKnightAttacks(int square) {
    uint64_t attacks = 0;
    int col = column(square);
    int r = row(square);
    
    const int offsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    
    for (auto [dc, dr] : offsets) {
        int newCol = col + dc;
        int newRow = r + dr;
        if (newCol >= 0 && newCol < 8 && newRow >= 0 && newRow < 8) {
            attacks |= (1ULL << position(newCol, newRow));
        }
    }
    
    return attacks;
}

uint64_t Board::getBishopAttacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int col = column(square);
    int r = row(square);
    
    const int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    
    for (auto [dc, dr] : directions) {
        int c = col + dc;
        int rr = r + dr;
        while (c >= 0 && c < 8 && rr >= 0 && rr < 8) {
            int sq = position(c, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
            c += dc;
            rr += dr;
        }
    }
    
    return attacks;
}

uint64_t Board::getRookAttacks(int square, uint64_t occupied) {
    uint64_t attacks = 0;
    int col = column(square);
    int r = row(square);
    
    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    
    for (auto [dc, dr] : directions) {
        int c = col + dc;
        int rr = r + dr;
        while (c >= 0 && c < 8 && rr >= 0 && rr < 8) {
            int sq = position(c, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
            c += dc;
            rr += dr;
        }
    }
    
    return attacks;
}

uint64_t Board::getQueenAttacks(int square, uint64_t occupied) {
    return getBishopAttacks(square, occupied) | getRookAttacks(square, occupied);
}

uint64_t Board::getKingAttacks(int square) {
    uint64_t attacks = 0;
    int col = column(square);
    int r = row(square);
    
    for (int dc = -1; dc <= 1; ++dc) {
        for (int dr = -1; dr <= 1; ++dr) {
            if (dc == 0 && dr == 0) continue;
            int newCol = col + dc;
            int newRow = r + dr;
            if (newCol >= 0 && newCol < 8 && newRow >= 0 && newRow < 8) {
                attacks |= (1ULL << position(newCol, newRow));
            }
        }
    }
    
    return attacks;
}

uint64_t Board::getPawnAttacks(uint64_t pawns, Color color) {
    if (color == WHITE) {
        return shiftUp(shiftRight(pawns)) | shiftUp(shiftLeft(pawns));
    } else {
        return shiftDown(shiftRight(pawns)) | shiftDown(shiftLeft(pawns));
    }
}

// Get all squares attacked by a given color
uint64_t Board::getAttackedSquares(Color color) const {
    uint64_t attacks = 0;
    uint64_t occupied = getAllPieces();
    
    // Pawn attacks
    attacks |= getPawnAttacks(bitboards[color][PAWN], color);
    
    // Knight attacks
    uint64_t knights = bitboards[color][KNIGHT];
    while (knights) {
        int sq = popLsb(knights);
        attacks |= getKnightAttacks(sq);
    }
    
    // Bishop attacks
    uint64_t bishops = bitboards[color][BISHOP];
    while (bishops) {
        int sq = popLsb(bishops);
        attacks |= getBishopAttacks(sq, occupied);
    }
    
    // Rook attacks
    uint64_t rooks = bitboards[color][ROOK];
    while (rooks) {
        int sq = popLsb(rooks);
        attacks |= getRookAttacks(sq, occupied);
    }
    
    // Queen attacks
    uint64_t queens = bitboards[color][QUEEN];
    while (queens) {
        int sq = popLsb(queens);
        attacks |= getQueenAttacks(sq, occupied);
    }
    
    // King attacks
    uint64_t king = bitboards[color][KING];
    if (king) {
        int sq = getLsb(king);
        attacks |= getKingAttacks(sq);
    }
    
    return attacks;
}

// Get forward rows from a given square (from color's perspective)
uint64_t Board::forwardRowsBB(Color color, int square) {
    int row = Board::row(square);
    if (color == WHITE) {
        // For White, forward is toward row 7
        uint64_t forwardRows = 0ULL;
        for (int r = row + 1; r <= 7; ++r) {
            forwardRows |= Board::rowBB(r);
        }
        return forwardRows;
    } else {
        // For Black, forward is toward row 0
        uint64_t forwardRows = 0ULL;
        for (int r = 0; r < row; ++r) {
            forwardRows |= Board::rowBB(r);
        }
        return forwardRows;
    }
}

// Check if a color has no pawns on a given column (semi-open file)
bool Board::isOnSemiOpenFile(const Board& board, Color color, int column) {
    uint64_t pawns = board.bitboards[color][PAWN];
    return (pawns & Board::columnBB(column)) == 0;
}