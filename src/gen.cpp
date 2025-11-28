#include "gen.hpp"

void MoveGenerator::generatePawnMoves(std::vector<Move>& moves, int from) const {
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
                moves.emplace_back(from, toSq, PieceType::QUEEN);
                moves.emplace_back(from, toSq, PieceType::ROOK);
                moves.emplace_back(from, toSq, PieceType::BISHOP);
                moves.emplace_back(from, toSq, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, toSq);
                // If we are at the starting position we can do a double move
                if (row == startRank) {
                    int to2Sq = from + 2 * direction * 8;
                    std::uint64_t to2Bit = 1ULL << to2Sq;
                    if (!(allOccupied & to2Bit)) {
                        moves.emplace_back(from, to2Sq);
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
                moves.emplace_back(from, captureSq, PieceType::QUEEN);
                moves.emplace_back(from, captureSq, PieceType::ROOK);
                moves.emplace_back(from, captureSq, PieceType::BISHOP);
                moves.emplace_back(from, captureSq, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, captureSq);
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
                moves.emplace_back(from, captureSq, PieceType::QUEEN);
                moves.emplace_back(from, captureSq, PieceType::ROOK);
                moves.emplace_back(from, captureSq, PieceType::BISHOP);
                moves.emplace_back(from, captureSq, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, captureSq);
            }
        }
    }
}

std::vector<Move> MoveGenerator::generatePseudoLegalMoves() const {
    std::vector<Move> moves;
    
    // Generate pawn moves using efficient bitboard iteration
    std::uint64_t pawns = (color == Color::WHITE) ? board.whitePawns : board.blackPawns;
    while (pawns) {
        int sq = Board::popLsb(pawns);
        generatePawnMoves(moves, sq);
    }
    
    
    return moves;
}