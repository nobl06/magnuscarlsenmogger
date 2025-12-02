#include "positional.h"

namespace Positional {



// Pawn structure penalties
constexpr Score BACKWARD      = Score(6, 19);
constexpr Score DOUBLED       = Score(11, 51);
constexpr Score ISOLATED      = Score(1, 20);

// Pawn bonuses
constexpr int CONNECTED[8] = { 0, 3, 7, 7, 15, 54, 86, 0 };

// Passed pawn bonuses by rank
constexpr Score PASSED_RANK[8] = {
    Score(0, 0), Score(2, 38), Score(15, 36), Score(22, 50),
    Score(64, 81), Score(166, 184), Score(284, 269), Score(0, 0)
};

// Mobility bonuses [piece_type][num_squares]
constexpr Score MOBILITY_KNIGHT[9] = {
    Score(-62, -79), Score(-53, -57), Score(-12, -31), Score(-3, -17),
    Score(3, 7), Score(12, 13), Score(21, 16), Score(28, 21), Score(37, 26)
};

constexpr Score MOBILITY_BISHOP[14] = {
    Score(-47, -59), Score(-20, -25), Score(14, -8), Score(29, 12),
    Score(39, 21), Score(53, 40), Score(53, 56), Score(60, 58),
    Score(62, 65), Score(69, 72), Score(78, 78), Score(83, 87),
    Score(91, 88), Score(96, 98)
};

constexpr Score MOBILITY_ROOK[15] = {
    Score(-60, -82), Score(-24, -15), Score(0, 17), Score(3, 43),
    Score(4, 72), Score(14, 100), Score(20, 102), Score(30, 122),
    Score(41, 133), Score(41, 139), Score(41, 153), Score(45, 160),
    Score(57, 165), Score(58, 170), Score(67, 175)
};

constexpr Score MOBILITY_QUEEN[28] = {
    Score(-29, -49), Score(-16, -29), Score(-8, -8), Score(-8, 17),
    Score(18, 39), Score(25, 54), Score(23, 59), Score(37, 73),
    Score(41, 76), Score(54, 95), Score(65, 95), Score(68, 101),
    Score(69, 124), Score(70, 128), Score(70, 132), Score(70, 133),
    Score(71, 136), Score(72, 140), Score(74, 147), Score(76, 149),
    Score(90, 153), Score(104, 169), Score(105, 171), Score(106, 171),
    Score(112, 178), Score(114, 185), Score(114, 187), Score(119, 221)
};

// King safety
constexpr int KING_ATTACK_WEIGHTS[7] = { 0, 0, 76, 46, 45, 14, 0 };
constexpr int SAFE_CHECK[7][2] = {
    {0, 0}, {0, 0}, {805, 1292}, {650, 984}, {1071, 1886}, {730, 1128}, {0, 0}
};

// Piece bonuses
constexpr Score ROOK_ON_OPEN_FILE = Score(49, 26);
constexpr Score ROOK_ON_SEMIOPEN_FILE = Score(18, 8);
constexpr Score ROOK_ON_CLOSED_FILE = Score(10, 5);
constexpr Score BISHOP_PAIR = Score(30, 65);
constexpr Score KNIGHT_OUTPOST = Score(54, 34);
constexpr Score BISHOP_OUTPOST = Score(31, 25);
constexpr Score MINOR_BEHIND_PAWN = Score(18, 3);
constexpr Score ROOK_ON_KING_RING = Score(16, 0);
constexpr Score BISHOP_ON_KING_RING = Score(24, 0);

// Threat bonuses
constexpr Score THREAT_BY_MINOR[7] = {
    Score(0, 0), Score(6, 37), Score(64, 50), Score(82, 57),
    Score(103, 130), Score(81, 163), Score(0, 0)
};

constexpr Score THREAT_BY_ROOK[7] = {
    Score(0, 0), Score(3, 44), Score(36, 71), Score(44, 59),
    Score(0, 39), Score(60, 39), Score(0, 0)
};

constexpr Score THREAT_BY_KING = Score(24, 87);
constexpr Score HANGING = Score(72, 40);


// Utility Functions
inline int relativeRow(Color color, int square) {
    int row = Board::row(square);
    return color == WHITE ? row : 7 - row;
}


// Get king zone (king + adjacent squares)
uint64_t getKingZone(int kingSq, Color color) {
    uint64_t zone = Board::getKingAttacks(kingSq) | (1ULL << kingSq);
    return zone;
}

// Pawn Structure Evaluation

uint64_t getPassedPawns(const Board& board, Color color) {
    uint64_t pawns = board.bitboards[color][PAWN];
    uint64_t enemyPawns = board.bitboards[1 - color][PAWN];
    uint64_t passed = 0;
    
    while (pawns) {
        int sq = Board::popLsb(pawns);
        int column = Board::column(sq);
        int row = Board::row(sq);
        
        // Build span: all squares ahead in this column and adjacent columns
        uint64_t span = 0;
        if (color == WHITE) {
            for (int r = row + 1; r < 8; ++r)
                span |= Board::rowBB(r);
        } else {
            for (int r = row - 1; r >= 0; --r)
                span |= Board::rowBB(r);
        }
        span &= (Board::columnBB(column) | Board::adjacentColumnsBB(column));
        
        // If no enemy pawns in the span ahead, this pawn is passed
        if ((enemyPawns & span) == 0) {
            passed |= (1ULL << sq);
        }
    }
    
    return passed;
}

uint64_t getIsolatedPawns(const Board& board, Color color) {
    uint64_t pawns = board.bitboards[color][PAWN];
    uint64_t isolated = 0;
    
    while (pawns) {
        int sq = Board::popLsb(pawns);
        int column = Board::column(sq);
        
        uint64_t adjacentColumns = Board::adjacentColumnsBB(column);
        if ((board.bitboards[color][PAWN] & adjacentColumns) == 0) {
            isolated |= (1ULL << sq);
        }
    }
    
    return isolated;
}

uint64_t getDoubledPawns(const Board& board, Color color) {
    uint64_t pawns = board.bitboards[color][PAWN];
    uint64_t doubled = 0;
    
    for (int column = 0; column < 8; ++column) {
        uint64_t columnPawns = pawns & Board::columnBB(column);
        if (Board::popcount(columnPawns) > 1) {
            doubled |= columnPawns;
        }
    }
    
    return doubled;
}
// checks for backward pawns: pawns that cannot advance safely and has no support
uint64_t getBackwardPawns(const Board& board, Color color) {
    uint64_t pawns = board.bitboards[color][PAWN];
    uint64_t enemyPawns = board.bitboards[1 - color][PAWN];
    uint64_t backward = 0;
    
    while (pawns) {
        int sq = Board::popLsb(pawns);
        int column = Board::column(sq);
        int row = Board::row(sq);
        
        // Check if no friendly pawns behind in adjacent columns
        bool hasSupport = false;
        uint64_t adjacentColumns = Board::adjacentColumnsBB(column);
        
        if (color == WHITE) {
            for (int r = 0; r < row; ++r) {
                if (board.bitboards[color][PAWN] & adjacentColumns & Board::rowBB(r)) {
                    hasSupport = true;
                    break;
                }
            }
        } else {
            for (int r = 7; r > row; --r) {
                if (board.bitboards[color][PAWN] & adjacentColumns & Board::rowBB(r)) {
                    hasSupport = true;
                    break;
                }
            }
        }
        
        if (!hasSupport) {
            // Check if enemy pawns attack the push square
            int pushSq = color == WHITE ? sq + 8 : sq - 8;
            if (pushSq >= 0 && pushSq < 64) {
                uint64_t enemyAttacks = Board::getPawnAttacks(enemyPawns, (Color)(1 - color));
                if (enemyAttacks & (1ULL << pushSq)) {
                    backward |= (1ULL << sq);
                }
            }
        }
    }
    
    return backward;
}



std::pair<int, int> evaluatePawns(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    
    for (Color color : {WHITE, BLACK}) {
        int sign = (color == WHITE) ? 1 : -1;
        
        uint64_t pawns = board.bitboards[color][PAWN];
        uint64_t enemyPawns = board.bitboards[1 - color][PAWN];
        
        // Passed pawns
        uint64_t passed = getPassedPawns(board, color);
        uint64_t passedCopy = passed;
        while (passedCopy) {
            int sq = Board::popLsb(passedCopy);
            int row = relativeRow(color, sq);
            mgScore += sign * PASSED_RANK[row].mg;
            egScore += sign * PASSED_RANK[row ].eg;
        }
        
        // Isolated pawns
        uint64_t isolated = getIsolatedPawns(board, color);
        mgScore -= sign * Board::popcount(isolated) * ISOLATED.mg;
        egScore -= sign * Board::popcount(isolated) * ISOLATED.eg;
        
        // Doubled pawns
        uint64_t doubled = getDoubledPawns(board, color);
        if (doubled) {
            // Only penalize extras beyond 1 per column
            for (int column = 0; column < 8; ++column) {
                int columnCount = Board::popcount(doubled & Board::columnBB(column));
                if (columnCount > 1) {
                    mgScore -= sign * (columnCount - 1) * DOUBLED.mg;
                    egScore -= sign * (columnCount - 1) * DOUBLED.eg;
                }
            }
        }
        
        // Backward pawns
        uint64_t backward = getBackwardPawns(board, color);
        mgScore -= sign * Board::popcount(backward) * BACKWARD.mg;
        egScore -= sign * Board::popcount(backward) * BACKWARD.eg;
        
        // Connected pawns
        uint64_t pawnsCopy = pawns;
        while (pawnsCopy) {
            int sq = Board::popLsb(pawnsCopy);
            int column = Board::column(sq);
            int row = relativeRow(color, sq);
            
            // Check for neighboring pawns (same row)
            uint64_t neighboringPawns = pawns & Board::rowBB(Board::row(sq)) & Board::adjacentColumnsBB(column);
            
            // Check for support (behind)
            int behindRow = color == WHITE ? Board::row(sq) - 1 : Board::row(sq) + 1;
            uint64_t support = 0;
            if (behindRow >= 0 && behindRow < 8) {
                support = pawns & Board::rowBB(behindRow) & Board::adjacentColumnsBB(column);
            }
            
            if (neighboringPawns || support) {
                // We check if the pawn is opposed (enemy pawn directly ahead on same column)
                bool opposed = false;
                uint64_t aheadMask = 0;
                if (color == WHITE) {
                    for (int r = Board::row(sq) + 1; r < 8; ++r)
                        aheadMask |= Board::rowBB(r);
                } else {
                    for (int r = Board::row(sq) - 1; r >= 0; --r)
                        aheadMask |= Board::rowBB(r);
                }
                aheadMask &= Board::columnBB(column);
                opposed = (enemyPawns & aheadMask) != 0;
                
                int supportCount = Board::popcount(support);
                
                // Stockfish's formula for connected pawns
                int v = CONNECTED[row] * (2 + (neighboringPawns ? 1 : 0) - (opposed ? 1 : 0))
                        + 22 * supportCount;
                
                mgScore += sign * v;
                egScore += sign * v * (row - 2) / 4;
            }
        }
    }
    
    return {mgScore, egScore};
}


// ========================================
// Mobility Evaluation
// ========================================

std::pair<int, int> evaluateMobility(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    uint64_t occupied = board.getAllPieces();
    
    for (Color color : {WHITE, BLACK}) {
        int sign = (color == WHITE) ? 1 : -1;
        uint64_t ourPieces = (color == WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
        
        // Mobility area: not attacked by enemy pawns, not occupied by our pieces
        uint64_t mobilityArea = ~(Board::getPawnAttacks(board.bitboards[1 - color][PAWN], (Color)(1 - color)))
                                & ~ourPieces;
        
        // Knights
        uint64_t knights = board.bitboards[color][KNIGHT];
        while (knights) {
            int sq = Board::popLsb(knights);
            uint64_t attacks = Board::getKnightAttacks(sq);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 8);
            mgScore += sign * MOBILITY_KNIGHT[mobility].mg;
            egScore += sign * MOBILITY_KNIGHT[mobility].eg;
        }
        
        // Bishops
        uint64_t bishops = board.bitboards[color][BISHOP];
        while (bishops) {
            int sq = Board::popLsb(bishops);
            uint64_t attacks = Board::getBishopAttacks(sq, occupied);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 13);
            mgScore += sign * MOBILITY_BISHOP[mobility].mg;
            egScore += sign * MOBILITY_BISHOP[mobility].eg;
        }
        
        // Rooks
        uint64_t rooks = board.bitboards[color][ROOK];
        while (rooks) {
            int sq = Board::popLsb(rooks);
            uint64_t attacks = Board::getRookAttacks(sq, occupied);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 14);
            mgScore += sign * MOBILITY_ROOK[mobility].mg;
            egScore += sign * MOBILITY_ROOK[mobility].eg;
        }
        
        // Queens
        uint64_t queens = board.bitboards[color][QUEEN];
        while (queens) {
            int sq = Board::popLsb(queens);
            uint64_t attacks = Board::getQueenAttacks(sq, occupied);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 27);
            mgScore += sign * MOBILITY_QUEEN[mobility].mg;
            egScore += sign * MOBILITY_QUEEN[mobility].eg;
        }
    }
    
    return {mgScore, egScore};
}



// ========================================
// Final Evaluation Function
// ========================================

std::pair<int, int> evaluatePositional(const Board& board) {
    auto [pawnMg, pawnEg] = evaluatePawns(board);
    auto [mobilityMg, mobilityEg] = evaluateMobility(board);
    
    int mgTotal = pawnMg + mobilityMg;
    int egTotal = pawnEg + mobilityEg;
    
    return {mgTotal, egTotal};
}

}