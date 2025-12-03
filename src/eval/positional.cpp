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

// Pawn shelter strength by [distance_from_edge][row]
constexpr int SHELTER_STRENGTH[4][8] = {
    { -2, 85, 95, 53, 39, 23, 25, 0 },      // a/h files (edge)
    { -55, 64, 32, -55, -30, -11, -61, 0 }, // b/g files
    { -11, 75, 19, -6, 26, 9, -47, 0 },     // c/f files
    { -41, -11, -27, -58, -42, -66, -163, 0 } // d/e files (center)
};

// Enemy pawn storm penalty by [distance_from_edge][row]
constexpr int UNBLOCKED_STORM[4][8] = {
    { 94, -280, -170, 90, 59, 47, 53, 0 },
    { 43, -17, 128, 39, 26, -17, 15, 0 },
    { -9, 62, 170, 34, -5, -20, -11, 0 },
    { -27, -19, 106, 10, 2, -13, -24, 0 }
};

// Blocked storm penalty by [row] - when our pawn blocks enemy pawn
constexpr Score BLOCKED_STORM[8] = {
    Score(0, 0), Score(0, 0), Score(64, 75), Score(-3, 14),
    Score(-12, 19), Score(-7, 4), Score(-10, 5), Score(0, 0)
};

// King on file penalty [semi-open for us][semi-open for them]
constexpr Score KING_ON_FILE[2][2] = {
    { Score(-18, 11), Score(-6, -3) },
    { Score(0, 0), Score(5, -4) }
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
// King Safety Evaluation
// ========================================

std::pair<int, int> evaluateKingSafety(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    
    for (Color color : {WHITE, BLACK}) {
        int sign = (color == WHITE) ? 1 : -1;
        Color enemy = (Color)(1 - color);
        
        uint64_t king = board.bitboards[color][KING];
        if (!king) continue;
        
        int kingSq = Board::getLsb(king);
        uint64_t kingZone = getKingZone(kingSq, color);
        
        uint64_t enemyAttacks = board.getAttackedSquares(enemy);
        uint64_t attackedKingZone = kingZone & enemyAttacks;
        
        int kingDanger = 0;
        
        int attackerCount = 0;
        int attackerWeight = 0;
        
        uint64_t occupied = board.getAllPieces();
        
        // Check for potential attacks by enemy pieces
        uint64_t knights = board.bitboards[enemy][KNIGHT];
        uint64_t knightsCopy = knights;
        while (knightsCopy) {
            int sq = Board::popLsb(knightsCopy);
            if (Board::getKnightAttacks(sq) & kingZone) {
                attackerCount++;
                attackerWeight += KING_ATTACK_WEIGHTS[KNIGHT];
            }
        }
        
        uint64_t bishops = board.bitboards[enemy][BISHOP];
        uint64_t bishopsCopy = bishops;
        while (bishopsCopy) {
            int sq = Board::popLsb(bishopsCopy);
            if (Board::getBishopAttacks(sq, occupied) & kingZone) {
                attackerCount++;
                attackerWeight += KING_ATTACK_WEIGHTS[BISHOP];
            }
        }
        
        uint64_t rooks = board.bitboards[enemy][ROOK];
        uint64_t rooksCopy = rooks;
        while (rooksCopy) {
            int sq = Board::popLsb(rooksCopy);
            if (Board::getRookAttacks(sq, occupied) & kingZone) {
                attackerCount++;
                attackerWeight += KING_ATTACK_WEIGHTS[ROOK];
            }
        }
        
        uint64_t queens = board.bitboards[enemy][QUEEN];
        uint64_t queensCopy = queens;
        while (queensCopy) {
            int sq = Board::popLsb(queensCopy);
            if (Board::getQueenAttacks(sq, occupied) & kingZone) {
                attackerCount++;
                attackerWeight += KING_ATTACK_WEIGHTS[QUEEN];
            }
        }
        
        // Safe checks score if enemy can check and checking square is safe
        uint64_t ourDefense = board.getAttackedSquares(color);
        int safeCheckBonus = 0;
        
        //Check safe checks for all pieces types
        uint64_t knightCheckSquares = Board::getKnightAttacks(kingSq);
        uint64_t safeKnightChecks = knightCheckSquares & knights & ~ourDefense;
        if (safeKnightChecks) {
            bool multiple = Board::moreThanOne(safeKnightChecks);
            safeCheckBonus += SAFE_CHECK[KNIGHT][multiple ? 1 : 0];
        }
        
        uint64_t bishopCheckSquares = Board::getBishopAttacks(kingSq, occupied);
        uint64_t safeBishopChecks = bishopCheckSquares & bishops & ~ourDefense;
        if (safeBishopChecks) {
            bool multiple = Board::moreThanOne(safeBishopChecks);
            safeCheckBonus += SAFE_CHECK[BISHOP][multiple ? 1 : 0];
        }
        
        uint64_t rookCheckSquares = Board::getRookAttacks(kingSq, occupied);
        uint64_t safeRookChecks = rookCheckSquares & rooks & ~ourDefense;
        if (safeRookChecks) {
            bool multiple = Board::moreThanOne(safeRookChecks);
            safeCheckBonus += SAFE_CHECK[ROOK][multiple ? 1 : 0];
        }
        
        uint64_t queenCheckSquares = Board::getQueenAttacks(kingSq, occupied);
        uint64_t safeQueenChecks = queenCheckSquares & queens & ~ourDefense;
        if (safeQueenChecks) {
            bool multiple = Board::moreThanOne(safeQueenChecks);
            safeCheckBonus += SAFE_CHECK[QUEEN][multiple ? 1 : 0];
        }
        
        //Count enemy attacks on squares directly adjacent to king
        uint64_t kingAdjacent = Board::getKingAttacks(kingSq);
        int kingAttacksCount = Board::popcount(kingAdjacent & enemyAttacks);
        
        int defensiveBonus = 0;
        
        // No queen bonus
        if (board.bitboards[enemy][QUEEN] == 0) {
            defensiveBonus += 873;
        }
        
        // Knight+King defense: our knight and king defending same squares
        uint64_t ourKnights = board.bitboards[color][KNIGHT];
        uint64_t ourKnightAttacks = 0;
        uint64_t ourKnightsCopy = ourKnights;
        while (ourKnightsCopy) {
            int sq = Board::popLsb(ourKnightsCopy);
            ourKnightAttacks |= Board::getKnightAttacks(sq);
        }
        uint64_t ourKingAttacks = Board::getKingAttacks(kingSq);
        if (ourKnightAttacks & ourKingAttacks) {
            defensiveBonus += 100;
        }
        
        // King danger score
        if (attackerCount > 0) {
            kingDanger = attackerCount * attackerWeight;
            kingDanger += 183 * Board::popcount(attackedKingZone);
            kingDanger += safeCheckBonus;
            kingDanger += 69 * kingAttacksCount;
            kingDanger += 37;
            kingDanger -= defensiveBonus;
            
            // Scale danger
            if (kingDanger > 100) {
                int dangerMg = kingDanger * kingDanger / 4096;
                int dangerEg = kingDanger / 16;
                mgScore -= sign * dangerMg;
                egScore -= sign * dangerEg;
            }
        }
        
        // Pawn shelter and storm evaluation
        int kColumn = Board::column(kingSq);
        
        int shelterMg = 5;
        int shelterEg = 5;
        
        // Only consider pawns that are relevant (at or in front of my king)
        uint64_t relevantPawns = board.bitboards[color][PAWN] | board.bitboards[enemy][PAWN];
        relevantPawns &= ~Board::forwardRowsBB(enemy, kingSq);
        
        uint64_t ourPawns = relevantPawns & board.bitboards[color][PAWN];
        uint64_t enemyPawns = relevantPawns & board.bitboards[enemy][PAWN];
        
        // Filter our pawns: exclude those attacked by enemy pawns
        uint64_t enemyPawnAttacks = Board::getPawnAttacks(board.bitboards[enemy][PAWN], enemy);
        ourPawns &= ~enemyPawnAttacks;
        
        // Center file
        int centerColumn = std::max(1, std::min(6, kColumn));
        
        // Check the 3 columns around the center
        for (int column = centerColumn - 1; column <= centerColumn + 1; ++column) {
            int edgeDist = std::min(column, 7 - column);
            
            // Find our pawn on this file
            uint64_t ourColumnPawns = ourPawns & Board::columnBB(column);
            int ourRow = 0;
            if (ourColumnPawns) {
                // Get closest pawn to king
                int pawnSq = color == WHITE ? Board::getLsb(ourColumnPawns) : Board::getMsb(ourColumnPawns);
                ourRow = relativeRow(color, pawnSq);
                
                // Only count if pawn is in front of king
                int kingRelRow = relativeRow(color, kingSq);
                if (ourRow <= kingRelRow) {
                    ourRow = 0;
                }
            }
            
            // Find enemy pawn on this file
            uint64_t enemyColumnPawns = enemyPawns & Board::columnBB(column);
            int enemyRow = 0;
            if (enemyColumnPawns) {
                // Get enemy pawn most advanced toward us
                int pawnSq = enemy == WHITE ? Board::getMsb(enemyColumnPawns) : Board::getLsb(enemyColumnPawns);
                enemyRow = relativeRow(enemy, pawnSq);
            }
            
            shelterMg += SHELTER_STRENGTH[edgeDist][ourRow];
            
            // Apply storm penalty
            if (ourRow && (ourRow == enemyRow - 1)) {
                // Our pawn and enemy pawn are face-to-face (Blocked Storm)
                shelterMg -= BLOCKED_STORM[enemyRow].mg;
                shelterEg -= BLOCKED_STORM[enemyRow].eg;
            } else {
                // Unblocked storm
                shelterMg -= UNBLOCKED_STORM[edgeDist][enemyRow];
            }
        }
        
        // King on file penalty
        bool ourSemiOpen = Board::isOnSemiOpenFile(board, color, kColumn);
        bool enemySemiOpen = Board::isOnSemiOpenFile(board, enemy, kColumn);
        shelterMg -= KING_ON_FILE[ourSemiOpen][enemySemiOpen].mg;
        shelterEg -= KING_ON_FILE[ourSemiOpen][enemySemiOpen].eg;
        
        mgScore += sign * shelterMg;
        egScore += sign * shelterEg;
    }
    
    return {mgScore, egScore};
}






// ========================================
// Final Evaluation Function
// ========================================

std::pair<int, int> evaluatePositional(const Board& board) {
    auto [pawnMg, pawnEg] = evaluatePawns(board);
    auto [mobilityMg, mobilityEg] = evaluateMobility(board);
    auto [kingSafetyMg, kingSafetyEg] = evaluateKingSafety(board);

    int mgTotal = pawnMg + mobilityMg + kingSafetyMg;
    int egTotal = pawnEg + mobilityEg + kingSafetyEg;
    
    return {mgTotal, egTotal};
}

}