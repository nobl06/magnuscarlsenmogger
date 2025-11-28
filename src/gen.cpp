#include "gen.hpp"
// PAWNS
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

// KNIGHTS ------------------------

void MoveGenerator::generateKnightMoves(std::vector<Move>& moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    const int dCol[8] = { 1,  2,  2,  1, -1, -2, -2, -1 };
    const int dRow[8] = { 2,  1, -1, -2, -2, -1,  1,  2 };

    for (int i = 0; i < 8; ++i) {
        int nc = col + dCol[i];
        int nr = row + dRow[i];
        if (nc < 0 || nc > 7 || nr < 0 || nr > 7) continue;

        int to = Board::position(nc, nr);

        // skip if our own piece is there
        if (board.isSquareOccupiedByColor(to, color)) continue;

        moves.emplace_back(from, to);
    }
}

//  BISHOPS ------------------------

void MoveGenerator::generateBishopMoves(std::vector<Move>& moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // 4 diagonal directions
    const int dCol[4] = {  1,  1, -1, -1 };
    const int dRow[4] = {  1, -1,  1, -1 };

    for (int dir = 0; dir < 4; ++dir) {
        int nc = col + dCol[dir];
        int nr = row + dRow[dir];

        while (nc >= 0 && nc < 8 && nr >= 0 && nr < 8) {
            int to = Board::position(nc, nr);

            if (board.isSquareOccupiedByColor(to, color)) {
                // our own piece blocks further moves
                break;
            }

            moves.emplace_back(from, to);

            if (!board.isSquareEmpty(to)) {
                // captured enemy → stop in this direction
                break;
            }

            nc += dCol[dir];
            nr += dRow[dir];
        }
    }
}

//  ROOKS ------------------------

void MoveGenerator::generateRookMoves(std::vector<Move>& moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // 4 straight directions: up, down, right, left
    const int dCol[4] = {  0,  0,  1, -1 };
    const int dRow[4] = {  1, -1,  0,  0 };

    for (int dir = 0; dir < 4; ++dir) {
        int nc = col + dCol[dir];
        int nr = row + dRow[dir];

        while (nc >= 0 && nc < 8 && nr >= 0 && nr < 8) {
            int to = Board::position(nc, nr);

            if (board.isSquareOccupiedByColor(to, color)) {
                // Our own piece blocks
                break;
            }

            moves.emplace_back(from, to);

            if (!board.isSquareEmpty(to)) {
                // Captured enemy → stop along this line
                break;
            }

            nc += dCol[dir];
            nr += dRow[dir];
        }
    }
}

//  QUEENS ------------------------

void MoveGenerator::generateQueenMoves(std::vector<Move>& moves, int from) const {
    generateBishopMoves(moves, from);
    generateRookMoves(moves, from);  // queen moves = bishop + rook hence
}

// KING ------------------------

void MoveGenerator::generateKingMoves(std::vector<Move>& moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // Adjacent squares 
    const int dCol[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int dRow[8] = { -1,-1,-1,  0, 0,  1, 1, 1 };

    for (int i = 0; i < 8; ++i) {
        int nc = col + dCol[i];
        int nr = row + dRow[i];
        if (nc < 0 || nc > 7 || nr < 0 || nr > 7) continue;

        int to = Board::position(nc, nr);

        if (board.isSquareOccupiedByColor(to, color)) continue;

        moves.emplace_back(from, to);
    }

    // we dont yet check if the king is moving into check
}.  // castling still not implemented



std::vector<Move> MoveGenerator::generatePseudoLegalMoves() const {
    std::vector<Move> moves;
    
    // Generate pawn moves using efficient bitboard iteration
    std::uint64_t pawns = (color == Color::WHITE) ? board.whitePawns : board.blackPawns;
    while (pawns) {
        int sq = Board::popLsb(pawns);
        generatePawnMoves(moves, sq);
    }

    // Knights move generator
    std::uint64_t knights = (color == Color::WHITE) ? board.whiteKnights : board.blackKnights;
    while (knights) {
        int sq = Board::popLsb(knights);
        generateKnightMoves(moves, sq);
    }

    // Bishops
    std::uint64_t bishops = (color == Color::WHITE) ? board.whiteBishops : board.blackBishops;
    while (bishops) {
        int sq = Board::popLsb(bishops);
        generateBishopMoves(moves, sq);
    }

    // Rooks
    std::uint64_t rooks = (color == Color::WHITE) ? board.whiteRooks : board.blackRooks;
    while (rooks) {
        int sq = Board::popLsb(rooks);
        generateRookMoves(moves, sq);
    }

    // Queens 
    std::uint64_t queens = (color == Color::WHITE) ? board.whiteQueens : board.blackQueens;
    while (queens) {
        int sq = Board::popLsb(queens);
        generateQueenMoves(moves, sq);
    }

    // even though king is single piece, but we can still use popLsbp
    std::uint64_t king = (color == Color::WHITE) ? board.whiteKing : board.blackKing;
    if (king) {
        int sq = Board::popLsb(king);
        generateKingMoves(moves, sq);
    }

    return moves;
}


