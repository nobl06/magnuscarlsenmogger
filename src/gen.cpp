#include "gen.hpp"

// PAWNS ------------------------
void MoveGenerator::generatePawnMoves(Move moves[220], size_t& moveCount, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // Determine direction and starting rank based on color
    int direction = (color == Color::WHITE) ? 1 : -1;
    int startRank = (color == Color::WHITE) ? 1 : 6;
    int promotionRank = (color == Color::WHITE) ? 7 : 0;

    std::uint64_t allOccupied = board.getAllPieces();
    std::uint64_t enemyOccupied = (color == Color::WHITE) ? board.getAllBlackPieces() : board.getAllWhitePieces();

    // Every basic forward move
    int toSq = from + direction * 8;
    if (row + direction >= 0 && row + direction < 8) {
        std::uint64_t toBit = 1ULL << toSq;

        if (!(allOccupied & toBit)) {
            if (row + direction == promotionRank) {
                moves[moveCount++] = Move(from, toSq, PieceType::QUEEN);
                moves[moveCount++] = Move(from, toSq, PieceType::ROOK);
                moves[moveCount++] = Move(from, toSq, PieceType::BISHOP);
                moves[moveCount++] = Move(from, toSq, PieceType::KNIGHT);
            } else {
                moves[moveCount++] = Move(from, toSq);
                // If we are at the starting position we can do a double move
                if (row == startRank) {
                    int to2Sq = from + 2 * direction * 8;
                    std::uint64_t to2Bit = 1ULL << to2Sq;
                    if (!(allOccupied & to2Bit)) {
                        moves[moveCount++] = Move(from, to2Sq);
                    }
                }
            }
        }
    }

    // all the captures that can be done to the left
    if (col > 0 && row + direction >= 0 && row + direction < 8) {
        int captureSq = from + direction * 8 - 1;
        std::uint64_t captureBit = 1ULL << captureSq;

        if (enemyOccupied & captureBit) {
            int captureRank = Board::row(captureSq);

            if (captureRank == promotionRank) {
                moves[moveCount++] = Move(from, captureSq, PieceType::QUEEN);
                moves[moveCount++] = Move(from, captureSq, PieceType::ROOK);
                moves[moveCount++] = Move(from, captureSq, PieceType::BISHOP);
                moves[moveCount++] = Move(from, captureSq, PieceType::KNIGHT);
            } else {
                moves[moveCount++] = Move(from, captureSq);
            }
        }
    }

    // all the captures that can be done to the right
    if (col < 7 && row + direction >= 0 && row + direction < 8) {
        int captureSq = from + direction * 8 + 1;
        std::uint64_t captureBit = 1ULL << captureSq;

        if (enemyOccupied & captureBit) {
            int captureRank = Board::row(captureSq);

            if (captureRank == promotionRank) {
                moves[moveCount++] = Move(from, captureSq, PieceType::QUEEN);
                moves[moveCount++] = Move(from, captureSq, PieceType::ROOK);
                moves[moveCount++] = Move(from, captureSq, PieceType::BISHOP);
                moves[moveCount++] = Move(from, captureSq, PieceType::KNIGHT);
            } else {
                moves[moveCount++] = Move(from, captureSq);
            }
        }
    }

    // Adds en passant captures if possible
    if (board.enPassantTarget != -1) {
        int epSquare = board.enPassantTarget;
        int epCol = Board::column(epSquare);
        int epRow = Board::row(epSquare);

        // Check if we can capture en passant to the left
        if (col > 0 && epCol == col - 1 && epRow == row + direction) {
            moves[moveCount++] = Move(from, epSquare);
        }

        // Check if we can capture en passant to the right
        if (col < 7 && epCol == col + 1 && epRow == row + direction) {
            moves[moveCount++] = Move(from, epSquare);
        }
    }
}

// KNIGHTS ------------------------

void MoveGenerator::generateKnightMoves(Move moves[220], size_t& moveCount, int from) const {
    uint64_t attacks = Board::getKnightAttacks(from);
    uint64_t ownPieces = (color == Color::WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
    
    // Remove squares occupied by our own pieces
    attacks &= ~ownPieces;
    
    // Generate move for each attack square
    while (attacks) {
        int to = Board::popLsb(attacks);
        moves[moveCount++] = Move(from, to);
    }
}

//  BISHOPS ------------------------

void MoveGenerator::generateBishopMoves(Move moves[220], size_t& moveCount, int from) const {
    uint64_t occupied = board.getAllPieces();
    uint64_t attacks = Board::getBishopAttacks(from, occupied);
    uint64_t ownPieces = (color == Color::WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
    
    // Remove squares occupied by our own pieces
    attacks &= ~ownPieces;
    
    // Generate move for each attack square
    while (attacks) {
        int to = Board::popLsb(attacks);
        moves[moveCount++] = Move(from, to);
    }
}

//  ROOKS ------------------------

void MoveGenerator::generateRookMoves(Move moves[220], size_t& moveCount, int from) const {
    uint64_t occupied = board.getAllPieces();
    uint64_t attacks = Board::getRookAttacks(from, occupied);
    uint64_t ownPieces = (color == Color::WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
    
    // Remove squares occupied by our own pieces
    attacks &= ~ownPieces;
    
    // Generate move for each attack square
    while (attacks) {
        int to = Board::popLsb(attacks);
        moves[moveCount++] = Move(from, to);
    }
}

//  QUEENS ------------------------

void MoveGenerator::generateQueenMoves(Move moves[220], size_t& moveCount, int from) const {
    uint64_t occupied = board.getAllPieces();
    uint64_t attacks = Board::getBishopAttacks(from, occupied) | Board::getRookAttacks(from, occupied);
    uint64_t ownPieces = (color == Color::WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
    
    attacks &= ~ownPieces;
    
    while (attacks) {
        int to = Board::popLsb(attacks);
        moves[moveCount++] = Move(from, to);
    }
}

// KING ------------------------

void MoveGenerator::generateKingMoves(Move moves[220], size_t& moveCount, int from) const {
    // Regular king moves
    uint64_t attacks = Board::getKingAttacks(from);
    uint64_t ownPieces = (color == Color::WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
    
    // Remove squares occupied by our own pieces
    attacks &= ~ownPieces;
    
    // Generate move for each attack square
    while (attacks) {
        int to = Board::popLsb(attacks);
        moves[moveCount++] = Move(from, to);
    }

    // Castling
    std::uint64_t allOccupied = board.getAllPieces();

    if (color == Color::WHITE) {
        // White kingside castling: e1 to g1
        if (board.whiteCanKingside && from == Board::position(4, 0)) {
            int f1 = Board::position(5, 0);
            int g1 = Board::position(6, 0);
            // Check that f1 and g1 are empty
            if (!((allOccupied >> f1) & 1) && !((allOccupied >> g1) & 1)) {
                moves[moveCount++] = Move(from, g1);
            }
        }

        // White queenside castling: e1 to c1
        if (board.whiteCanQueenside && from == Board::position(4, 0)) {
            int d1 = Board::position(3, 0);
            int c1 = Board::position(2, 0);
            int b1 = Board::position(1, 0);
            // Check that b1, c1, d1 are empty
            if (!((allOccupied >> d1) & 1) && !((allOccupied >> c1) & 1) && !((allOccupied >> b1) & 1)) {
                moves[moveCount++] = Move(from, c1);
            }
        }
    } else {
        // Black kingside castling: e8 to g8
        if (board.blackCanKingside && from == Board::position(4, 7)) {
            int f8 = Board::position(5, 7);
            int g8 = Board::position(6, 7);
            if (!((allOccupied >> f8) & 1) && !((allOccupied >> g8) & 1)) {
                moves[moveCount++] = Move(from, g8);
            }
        }

        // Black queenside castling: e8 to c8
        if (board.blackCanQueenside && from == Board::position(4, 7)) {
            int d8 = Board::position(3, 7);
            int c8 = Board::position(2, 7);
            int b8 = Board::position(1, 7);
            if (!((allOccupied >> d8) & 1) && !((allOccupied >> c8) & 1) && !((allOccupied >> b8) & 1)) {
                moves[moveCount++] = Move(from, c8);
            }
        }
    }
}

size_t MoveGenerator::generatePseudoLegalMoves(Move moves[220]) const {
    size_t moveCount = 0;

    int c = color; // WHITE = 0, BLACK = 1

    // Pawns
    uint64_t pawns = board.bitboards[c][PAWN];
    while (pawns) {
        int sq = Board::popLsb(pawns);
        generatePawnMoves(moves, moveCount, sq);
    }

    // Knights
    uint64_t knights = board.bitboards[c][KNIGHT];
    while (knights) {
        int sq = Board::popLsb(knights);
        generateKnightMoves(moves, moveCount, sq);
    }

    // Bishops
    uint64_t bishops = board.bitboards[c][BISHOP];
    while (bishops) {
        int sq = Board::popLsb(bishops);
        generateBishopMoves(moves, moveCount, sq);
    }

    // Rooks
    uint64_t rooks = board.bitboards[c][ROOK];
    while (rooks) {
        int sq = Board::popLsb(rooks);
        generateRookMoves(moves, moveCount, sq);
    }

    // Queens
    uint64_t queens = board.bitboards[c][QUEEN];
    while (queens) {
        int sq = Board::popLsb(queens);
        generateQueenMoves(moves, moveCount, sq);
    }

    // King
    uint64_t king = board.bitboards[c][KING];
    if (king) {
        int sq = Board::popLsb(king);
        generateKingMoves(moves, moveCount, sq);
    }

    return moveCount;
}

size_t MoveGenerator::filterLegalMoves(const Move pseudoLegalMoves[220], size_t pseudoLegalCount, Move legalMoves[220]) {
    size_t legalCount = 0;
    Color ourColor = color;
    Color opponent = (ourColor == Color::WHITE) ? Color::BLACK : Color::WHITE;

    for (size_t i = 0; i < pseudoLegalCount; i++) {
        const Move& move = pseudoLegalMoves[i];
        PieceType movedPiece = board.pieceAt(move.from);
        
        // Handle castling specially - need to check if king passes through check
        if (movedPiece == PieceType::KING && std::abs(static_cast<int>(move.to) - static_cast<int>(move.from)) == 2) {
            // King can't castle out of check
            if (board.isKingInCheck(ourColor)) {
                continue;
            }
            
            // Check if king passes through attacked square
            int fromCol = Board::column(move.from);
            int toCol = Board::column(move.to);
            int row = Board::row(move.from);
            int middleCol = (fromCol + toCol) / 2;
            int middleSq = Board::position(middleCol, row);
            
            if (board.isSquareAttackedBy(middleSq, opponent)) {
                continue;
            }
        }

        // Use makeMove/unmakeMove to check legality
        BoardState state = board.makeMove(move);
        bool legal = !board.isKingInCheck(ourColor);
        board.unmakeMove(move, state);
        
        if (legal) {
            legalMoves[legalCount++] = move;
        }
    }

    return legalCount;
}
