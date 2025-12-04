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
constexpr Score THREAT_BY_PAWN_PUSH = Score(48, 39);
constexpr Score THREAT_BY_SAFE_PAWN = Score(167, 99);
constexpr Score HANGING = Score(72, 40);
constexpr Score WEAK_QUEEN_PROTECTION = Score(14, 0);
constexpr Score RESTRICTED_PIECE = Score(6, 7);
constexpr Score KNIGHT_ON_QUEEN = Score(16, 11);
constexpr Score SLIDER_ON_QUEEN = Score(62, 21);


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
    
    for (Color color : {WHITE, BLACK}) {
        int sign = (color == WHITE) ? 1 : -1;
        Color enemy = (Color)(1 - color);
        

        uint64_t allPieces = board.getAllPieces();
        uint64_t ourPawns = board.bitboards[color][PAWN];
        
        uint64_t blockedPawns;
        if (color == WHITE) {
            blockedPawns = ourPawns & Board::shiftDown(allPieces);
        } else {
            blockedPawns = ourPawns & Board::shiftUp(allPieces);
        }
        
        // Low rank pawns: on ranks 2-3 for White, 6-7 for Black
        uint64_t lowRanks = (color == WHITE) 
            ? (Board::rowBB(1) | Board::rowBB(2))
            : (Board::rowBB(6) | Board::rowBB(5));
        uint64_t lowRankPawns = ourPawns & lowRanks;
        
        // Undeveloped pawns = blocked OR on low ranks
        uint64_t undevelopedPawns = blockedPawns | lowRankPawns;
        
        // Mobility area: squares not attacked by enemy pawns, not occupied by:
        // - our king
        // - our queen  
        // - our developed pawns
        uint64_t enemyPawnAttacks = Board::getPawnAttacks(board.bitboards[enemy][PAWN], enemy);
        uint64_t mobilityArea = ~enemyPawnAttacks
                                & ~board.bitboards[color][KING]
                                & ~board.bitboards[color][QUEEN]
                                & ~(ourPawns & ~undevelopedPawns);
        
        // Knights - simple attacks
        uint64_t knights = board.bitboards[color][KNIGHT];
        while (knights) {
            int sq = Board::popLsb(knights);
            uint64_t attacks = Board::getKnightAttacks(sq);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 8);
            mgScore += sign * MOBILITY_KNIGHT[mobility].mg;
            egScore += sign * MOBILITY_KNIGHT[mobility].eg;
        }
        
        // Bishops - x-ray through queens for mobility calculation
        uint64_t bishops = board.bitboards[color][BISHOP];
        while (bishops) {
            int sq = Board::popLsb(bishops);
            // See through queens (potential mobility even when blocked)
            uint64_t bishopOccupancy = allPieces ^ board.bitboards[color][QUEEN];
            uint64_t attacks = Board::getBishopAttacks(sq, bishopOccupancy);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 13);
            mgScore += sign * MOBILITY_BISHOP[mobility].mg;
            egScore += sign * MOBILITY_BISHOP[mobility].eg;
        }
        
        // Rooks - x-ray through queens and other rooks
        uint64_t rooks = board.bitboards[color][ROOK];
        uint64_t rooksCopy = rooks;
        while (rooksCopy) {
            int sq = Board::popLsb(rooksCopy);
            // See through queens and other rooks (but not the current rook)
            uint64_t rookOccupancy = allPieces 
                                    ^ board.bitboards[color][QUEEN]
                                    ^ (rooks & ~(1ULL << sq));
            uint64_t attacks = Board::getRookAttacks(sq, rookOccupancy);
            int mobility = Board::popcount(attacks & mobilityArea);
            mobility = std::min(mobility, 14);
            mgScore += sign * MOBILITY_ROOK[mobility].mg;
            egScore += sign * MOBILITY_ROOK[mobility].eg;
        }
        
        // Queens - standard attacks
        uint64_t queens = board.bitboards[color][QUEEN];
        while (queens) {
            int sq = Board::popLsb(queens);
            uint64_t attacks = Board::getQueenAttacks(sq, allPieces);
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
        //only evaluate shelter if king has lost castling rights
        bool canCastleKingside = (color == WHITE) ? board.whiteCanKingside : board.blackCanKingside;
        bool canCastleQueenside = (color == WHITE) ? board.whiteCanQueenside : board.blackCanQueenside;
        
        if (!canCastleKingside && !canCastleQueenside) {
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
    }
    
    return {mgScore, egScore};
}


// ======================================== 
// Piece-Specific Evaluation
// ========================================

std::pair<int, int> evaluatePieces(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    uint64_t occupied = board.getAllPieces();
    
    for (Color color : {WHITE, BLACK}) {
        int sign = (color == WHITE) ? 1 : -1;
        
        
        // Rooks on open/semi-open/closed files
        uint64_t rooks = board.bitboards[color][ROOK];
        uint64_t rooksCopy = rooks;
        while (rooksCopy) {
            int sq = Board::popLsb(rooksCopy);
            int column = Board::column(sq);
            
            uint64_t ourPawnsOnColumn = board.bitboards[color][PAWN] & Board::columnBB(column);
            uint64_t enemyPawnsOnColumn = board.bitboards[1 - color][PAWN] & Board::columnBB(column);
            
            if (ourPawnsOnColumn == 0) {
                if (enemyPawnsOnColumn == 0) {
                    // Open file
                    mgScore += sign * ROOK_ON_OPEN_FILE.mg;
                    egScore += sign * ROOK_ON_OPEN_FILE.eg;
                } else {
                    // Semi-open file
                    mgScore += sign * ROOK_ON_SEMIOPEN_FILE.mg;
                    egScore += sign * ROOK_ON_SEMIOPEN_FILE.eg;
                }
            } else {
                if (enemyPawnsOnColumn != 0) {
                    // Closed file
                    uint64_t blockedPawns = ourPawnsOnColumn & Board::shiftDown(occupied);
                    if (color == BLACK) {
                        blockedPawns = ourPawnsOnColumn & Board::shiftUp(occupied);
                    }
                    
                    if (blockedPawns) {
                        mgScore -= sign * ROOK_ON_CLOSED_FILE.mg;
                        egScore -= sign * ROOK_ON_CLOSED_FILE.eg;
                    }
                }
            }
        }
        
        // Knight outposts and minor pieces
        uint64_t knights = board.bitboards[color][KNIGHT];
        uint64_t knightsCopy = knights;
        uint64_t ourPawns = board.bitboards[color][PAWN];
        uint64_t enemyPawns = board.bitboards[1 - color][PAWN];
        
        // Build outpost rows
        uint64_t outpostRows = (color == WHITE) 
            ? (Board::rowBB(3) | Board::rowBB(4) | Board::rowBB(5))
            : (Board::rowBB(4) | Board::rowBB(3) | Board::rowBB(2));
        
        // Squares defended by our pawns
        uint64_t pawnDefended = Board::getPawnAttacks(ourPawns, color);
        
        while (knightsCopy) {
            int sq = Board::popLsb(knightsCopy);
            int row = relativeRow(color, sq);
            int column = Board::column(sq);
            
            // Check for outpost
            if (outpostRows & (1ULL << sq)) {
                if (pawnDefended & (1ULL << sq)) {
                    // Check if safe from enemy pawns
                    uint64_t attackSpan = Board::forwardRowsBB(color, sq) 
                                        &  Board::adjacentColumnsBB(column);
                    
                    if ((enemyPawns & attackSpan) == 0) {
                        mgScore += sign * KNIGHT_OUTPOST.mg;
                        egScore += sign * KNIGHT_OUTPOST.eg;
                    }
                }
            }
            
            // Knight shielded by friendly pawn
            int frontSq = color == WHITE ? sq + 8 : sq - 8;
            if (frontSq >= 0 && frontSq < 64 && (ourPawns & (1ULL << frontSq))) {
                mgScore += sign * MINOR_BEHIND_PAWN.mg;
                egScore += sign * MINOR_BEHIND_PAWN.eg;
            }
        }
        
        // Bishop outposts
        uint64_t bishops = board.bitboards[color][BISHOP];
        uint64_t bishopsCopy = bishops;
        
        while (bishopsCopy) {
            int sq = Board::popLsb(bishopsCopy);
            int column = Board::column(sq);
            
            if (outpostRows & (1ULL << sq)) {
                if (pawnDefended & (1ULL << sq)) {
                    // Check if safe from enemy pawns
                    uint64_t attackSpan = Board::forwardRowsBB(color, sq) 
                                        & Board::adjacentColumnsBB(column);
                    
                    if ((enemyPawns & attackSpan) == 0) {
                        mgScore += sign * BISHOP_OUTPOST.mg;
                        egScore += sign * BISHOP_OUTPOST.eg;
                    }
                }
            }
        
            // Bishop shielded by friendly pawn
            int frontSq = color == WHITE ? sq + 8 : sq - 8;
            if (frontSq >= 0 && frontSq < 64 && (ourPawns & (1ULL << frontSq))) {
                mgScore += sign * MINOR_BEHIND_PAWN.mg;
                egScore += sign * MINOR_BEHIND_PAWN.eg;
            }
        }
    }
    
    return {mgScore, egScore};
}


// ========================================
// Threats Evaluation
// ========================================

std::pair<int, int> evaluateThreats(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    
    for (Color color : {WHITE, BLACK}) {
        int sign = (color == WHITE) ? 1 : -1;
        Color enemy = (Color)(1 - color);
        
        uint64_t occupied = board.getAllPieces();
        
        uint64_t enemyPieces = (enemy == WHITE) ? board.getAllWhitePieces() : board.getAllBlackPieces();
        uint64_t nonPawnEnemies = enemyPieces & ~board.bitboards[enemy][PAWN];
        
        // Piece attacks tracking
        uint64_t ourAttackedBy[7] = {0}; // [EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING]
        uint64_t enemyAttackedBy[7] = {0};
        uint64_t ourAttacks = 0;
        uint64_t enemyAttacks = 0;
        uint64_t ourAttacks2 = 0;
        uint64_t enemyAttacks2 = 0;
        
        ourAttackedBy[PAWN] = Board::getPawnAttacks(board.bitboards[color][PAWN], color);
        enemyAttackedBy[PAWN] = Board::getPawnAttacks(board.bitboards[enemy][PAWN], enemy);
        
        // Calculate our attacks by piece type (Knights through Kings)
        for (int pt = KNIGHT; pt <= KING; pt++) {
            uint64_t pieces = board.bitboards[color][pt];
            while (pieces) {
                int sq = Board::popLsb(pieces);
                uint64_t attacks = 0;
                
                if (pt == KNIGHT) attacks = Board::getKnightAttacks(sq);
                else if (pt == BISHOP) attacks = Board::getBishopAttacks(sq, occupied);
                else if (pt == ROOK) attacks = Board::getRookAttacks(sq, occupied);
                else if (pt == QUEEN) attacks = Board::getQueenAttacks(sq, occupied);
                else if (pt == KING) attacks = Board::getKingAttacks(sq);
                
                ourAttackedBy[pt] |= attacks;
            }
        }
        
        // Calculate enemy attacks by piece type (Knights through Kings)
        for (int pt = KNIGHT; pt <= KING; pt++) {
            uint64_t pieces = board.bitboards[enemy][pt];
            while (pieces) {
                int sq = Board::popLsb(pieces);
                uint64_t attacks = 0;
                
                if (pt == KNIGHT) attacks = Board::getKnightAttacks(sq);
                else if (pt == BISHOP) attacks = Board::getBishopAttacks(sq, occupied);
                else if (pt == ROOK) attacks = Board::getRookAttacks(sq, occupied);
                else if (pt == QUEEN) attacks = Board::getQueenAttacks(sq, occupied);
                else if (pt == KING) attacks = Board::getKingAttacks(sq);
                
                enemyAttackedBy[pt] |= attacks;
            }
        }
        
        // Compute all attacks and double attacks (attacks on a square by two or more pieces)
        for (int pt = PAWN; pt <= KING; pt++) {
            ourAttacks2 |= (ourAttacks & ourAttackedBy[pt]);
            ourAttacks |= ourAttackedBy[pt];
            
            enemyAttacks2 |= (enemyAttacks & enemyAttackedBy[pt]);
            enemyAttacks |= enemyAttackedBy[pt];
        }
        
        // pawn attacks
        uint64_t ourPawnAttacks = ourAttackedBy[PAWN];
        uint64_t enemyPawnAttacks = enemyAttackedBy[PAWN];
        
        uint64_t stronglyProtected = enemyPawnAttacks | (enemyAttacks2 & ~ourAttacks2);
        
        // Strongly protected ennemi pieces
        uint64_t defended = nonPawnEnemies & stronglyProtected;
        
        // Enemies not strongly protected and under our attack
        uint64_t weak = enemyPieces & ~stronglyProtected & ourAttacks;
        
        // Bonus according to the kind of attacking pieces
        if (defended | weak) {
            // Threats by minor pieces
            uint64_t minorAttacks = ourAttackedBy[KNIGHT] | ourAttackedBy[BISHOP];
            uint64_t threatenedByMinor = (defended | weak) & minorAttacks;
            while (threatenedByMinor) {
                int sq = Board::popLsb(threatenedByMinor);
                PieceType piece = board.pieceAt(sq);
                if (piece != EMPTY) {
                    mgScore += sign * THREAT_BY_MINOR[piece].mg;
                    egScore += sign * THREAT_BY_MINOR[piece].eg;
                }
            }
            
            // Threats by rooks (only weak ennemies attacked by our rooks)
            uint64_t threatenedByRook = weak & ourAttackedBy[ROOK];
            while (threatenedByRook) {
                int sq = Board::popLsb(threatenedByRook);
                PieceType piece = board.pieceAt(sq);
                if (piece != EMPTY) {
                    mgScore += sign * THREAT_BY_ROOK[piece].mg;
                    egScore += sign * THREAT_BY_ROOK[piece].eg;
                }
            }
            
            // Threats by king
            if (weak & ourAttackedBy[KING]) {
                mgScore += sign * THREAT_BY_KING.mg;
                egScore += sign * THREAT_BY_KING.eg;
            }
            
            // Hanging pieces
            // its an ennemy piece that is not strongly protected and we attack it. 
            // And it is either not defended by enemy at all or its a non pawn ennemy and we attack it twice.
            uint64_t bitboardHanging = ~enemyAttacks | (nonPawnEnemies & ourAttacks2);
            int hangingCount = Board::popcount(weak & bitboardHanging);
            mgScore += sign * hangingCount * HANGING.mg;
            egScore += sign * hangingCount * HANGING.eg;
            
            // Additional bonus if weak piece is only protected by a queen
            int weakQueenCount = Board::popcount(weak & enemyAttackedBy[QUEEN]);
            mgScore += sign * weakQueenCount * WEAK_QUEEN_PROTECTION.mg;
            egScore += sign * weakQueenCount * WEAK_QUEEN_PROTECTION.eg;
        }
        
        // Squares that both sides attack, but they're not strongly protected by them
        uint64_t restricted = enemyAttacks & ~stronglyProtected & ourAttacks;
        int restrictedCount = Board::popcount(restricted);
        mgScore += sign * restrictedCount * RESTRICTED_PIECE.mg;
        egScore += sign * restrictedCount * RESTRICTED_PIECE.eg;
        
        // Protected or unattacked squares
        uint64_t safe = ~enemyAttacks | ourAttacks;
        

        uint64_t safePawns = board.bitboards[color][PAWN] & safe;
        uint64_t safePawnAttacks = Board::getPawnAttacks(safePawns, color);
        uint64_t threatenedBySafePawn = safePawnAttacks & nonPawnEnemies;
        
        // Threats by safe pawns
        int safePawnThreats = Board::popcount(threatenedBySafePawn);
        mgScore += sign * safePawnThreats * THREAT_BY_SAFE_PAWN.mg;
        egScore += sign * safePawnThreats * THREAT_BY_SAFE_PAWN.eg;
        
        // Find squares where our pawns can push on the next move
        uint64_t pawnPushes = 0;
        if (color == WHITE) {
            pawnPushes = Board::shiftUp(board.bitboards[WHITE][PAWN]) & ~occupied;
            uint64_t startingPawns3 = board.bitboards[WHITE][PAWN] & Board::rowBB(1);
            pawnPushes |= Board::shiftUp(Board::shiftUp(startingPawns3)) & ~occupied & ~Board::shiftUp(occupied); // double push
        } else {
            pawnPushes = Board::shiftDown(board.bitboards[BLACK][PAWN]) & ~occupied;
            uint64_t startingPawns6 = board.bitboards[BLACK][PAWN] & Board::rowBB(6);
            pawnPushes |= Board::shiftDown(Board::shiftDown(startingPawns6)) & ~occupied & ~Board::shiftDown(occupied);
        }
        
        // Keep only the squares safe
        pawnPushes &= ~enemyPawnAttacks & safe;
        
        // Bonus for safe pawn threats on the next move
        uint64_t pawnPushThreats = Board::getPawnAttacks(pawnPushes, color) & nonPawnEnemies;
        int pushThreats = Board::popcount(pawnPushThreats);
        mgScore += sign * pushThreats * THREAT_BY_PAWN_PUSH.mg;
        egScore += sign * pushThreats * THREAT_BY_PAWN_PUSH.eg;
        
        // Bonus for threats on the next moves against enemy queen
        uint64_t enemyQueen = board.bitboards[enemy][QUEEN];
        if (Board::popcount(enemyQueen) == 1) {
            int queenSq = Board::getLsb(enemyQueen);
            bool queenImbalance = (Board::popcount(board.bitboards[WHITE][QUEEN]) + 
                                   Board::popcount(board.bitboards[BLACK][QUEEN])) == 1;
            
            // Safe squares for attacks (mobility area, not pawns, not strongly protected)
            uint64_t safeMobilityArea = safe & ~board.bitboards[color][PAWN] & ~stronglyProtected;
            
            // Knight attacks on queen
            uint64_t knightAttacksOnQueen = Board::getKnightAttacks(queenSq);
            uint64_t ourKnights = board.bitboards[color][KNIGHT];
            while (ourKnights) {
                int sq = Board::popLsb(ourKnights);
                uint64_t kAttacks = Board::getKnightAttacks(sq);
                int knightThreats = Board::popcount(kAttacks & knightAttacksOnQueen & safeMobilityArea);
                mgScore += sign * knightThreats * (1 + queenImbalance) * KNIGHT_ON_QUEEN.mg;
                egScore += sign * knightThreats * (1 + queenImbalance) * KNIGHT_ON_QUEEN.eg;
            }
            
            // Slider attacks on queen
            uint64_t bishopAttacksOnQueen = Board::getBishopAttacks(queenSq, occupied);
            uint64_t rookAttacksOnQueen = Board::getRookAttacks(queenSq, occupied);
            
            uint64_t ourBishops = board.bitboards[color][BISHOP];
            while (ourBishops) {
                int sq = Board::popLsb(ourBishops);
                uint64_t bAttacks = Board::getBishopAttacks(sq, occupied);
                uint64_t sliderThreats = bAttacks & bishopAttacksOnQueen & safeMobilityArea & ourAttacks2;
                int sliderCount = Board::popcount(sliderThreats);
                mgScore += sign * sliderCount * (1 + queenImbalance) * SLIDER_ON_QUEEN.mg;
                egScore += sign * sliderCount * (1 + queenImbalance) * SLIDER_ON_QUEEN.eg;
            }
            
            uint64_t ourRooks = board.bitboards[color][ROOK];
            while (ourRooks) {
                int sq = Board::popLsb(ourRooks);
                uint64_t rAttacks = Board::getRookAttacks(sq, occupied);
                uint64_t sliderThreats = rAttacks & rookAttacksOnQueen & safeMobilityArea & ourAttacks2;
                int sliderCount = Board::popcount(sliderThreats);
                mgScore += sign * sliderCount * (1 + queenImbalance) * SLIDER_ON_QUEEN.mg;
                egScore += sign * sliderCount * (1 + queenImbalance) * SLIDER_ON_QUEEN.eg;
            }
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
    auto [kingSafetyMg, kingSafetyEg] = evaluateKingSafety(board);
    auto [piecesMg, piecesEg] = evaluatePieces(board);
    auto [threatsMg, threatsEg] = evaluateThreats(board);

    int mgTotal = pawnMg + mobilityMg + kingSafetyMg + piecesMg + threatsMg;
    int egTotal = pawnEg + mobilityEg + kingSafetyEg + piecesEg + threatsEg;
    
    return {mgTotal, egTotal};
}

}