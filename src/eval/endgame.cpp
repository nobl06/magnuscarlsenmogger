#include "endgame.h"
#include <algorithm>
#include <cmath>

namespace Endgame {

// File bitboards for endgame calculations
constexpr uint64_t FILE_A_BB = 0x0101010101010101ULL;
constexpr uint64_t FILE_B_BB = 0x0202020202020202ULL;
constexpr uint64_t FILE_D_BB = 0x0808080808080808ULL;
constexpr uint64_t FILE_E_BB = 0x1010101010101010ULL;
constexpr uint64_t FILE_G_BB = 0x4040404040404040ULL;
constexpr uint64_t FILE_H_BB = 0x8080808080808080ULL;

// Dark and light squares
constexpr uint64_t DARK_SQUARES = 0xAA55AA55AA55AA55ULL;
constexpr uint64_t LIGHT_SQUARES = ~DARK_SQUARES;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Drive defending king to edge of board (values range 27-90)
int pushToEdge(int sq) {
    int rd = Board::edgeDistance(Board::row(sq));
    int fd = Board::edgeDistance(Board::column(sq));
    return 90 - (7 * fd * fd / 2 + 7 * rd * rd / 2);
}

// Drive king to A1/H8 corner (for KBNK endgame)
// Values range from 0 (on A8-H1 diagonal) to 7 (in A1/H8 corners)
int pushToCorner(int sq) {
    return std::abs(7 - Board::row(sq) - Board::column(sq));
}

// Drive two pieces close together
int pushClose(int sq1, int sq2) {
    return 140 - 20 * Board::distance(sq1, sq2);
}

// Drive two pieces apart
int pushAway(int sq1, int sq2) {
    return 120 - pushClose(sq1, sq2);
}

// Check if two squares are on opposite colors
static bool oppositeColors(int sq1, int sq2) {
    int s = sq1 ^ sq2;
    return ((s >> 3) ^ s) & 1;
}

// Get non-pawn material for a color
static int nonPawnMaterial(const Board& board, Color c) {
    return Board::popcount(board.bitboards[c][KNIGHT]) * KNIGHT_VALUE_MG
         + Board::popcount(board.bitboards[c][BISHOP]) * BISHOP_VALUE_MG
         + Board::popcount(board.bitboards[c][ROOK]) * ROOK_VALUE_MG
         + Board::popcount(board.bitboards[c][QUEEN]) * QUEEN_VALUE_MG;
}

// Normalize square: flip so strong side is white and pawn is on left half
static int normalize(const Board& board, Color strongSide, int sq) {
    // If strong side's pawn is on right half (files e-h), flip file
    uint64_t strongPawns = board.bitboards[strongSide][PAWN];
    if (strongPawns) {
        int pawnSq = Board::getLsb(strongPawns);
        if (Board::column(pawnSq) >= 4) {
            sq = Board::flipFile(sq);
        }
    }
    // If strong side is black, flip rank
    return strongSide == WHITE ? sq : Board::flipRank(sq);
}

// Get forward file bitboard (all squares ahead on same file)
static uint64_t forwardFileBB(Color c, int sq) {
    uint64_t fileBB = Board::columnBB(Board::column(sq));
    if (c == WHITE) {
        // All ranks above this square
        uint64_t mask = ~0ULL << (sq + 8);
        return fileBB & mask;
    } else {
        // All ranks below this square
        uint64_t mask = (1ULL << sq) - 1;
        return fileBB & mask;
    }
}

// Get passed pawn span (squares a pawn must pass through)
static uint64_t passedPawnSpan(Color c, int sq) {
    uint64_t file = Board::columnBB(Board::column(sq));
    uint64_t adjacent = Board::adjacentColumnsBB(Board::column(sq));
    uint64_t forward;
    if (c == WHITE) {
        forward = ~0ULL << (sq + 8);
    } else {
        forward = (1ULL << sq) - 1;
    }
    return (file | adjacent) & forward;
}

// Check if strong side can force mate (has Q, R, B+N, or two bishops of different colors)
static bool canForceMate(const Board& board, Color strongSide) {
    if (board.bitboards[strongSide][QUEEN]) return true;
    if (board.bitboards[strongSide][ROOK]) return true;
    
    int bishops = Board::popcount(board.bitboards[strongSide][BISHOP]);
    int knights = Board::popcount(board.bitboards[strongSide][KNIGHT]);
    
    // B+N can mate
    if (bishops >= 1 && knights >= 1) return true;
    
    // Two bishops on different colors can mate
    if (bishops >= 2) {
        uint64_t bishopBB = board.bitboards[strongSide][BISHOP];
        bool hasDark = (bishopBB & DARK_SQUARES) != 0;
        bool hasLight = (bishopBB & LIGHT_SQUARES) != 0;
        if (hasDark && hasLight) return true;
    }
    
    return false;
}

// ============================================================================
// EVALUATION FUNCTIONS
// ============================================================================

// KXK: King + overwhelming material vs lone King
int evaluateKXK(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // Start with material value
    int result = nonPawnMaterial(board, strongSide)
               + Board::popcount(board.bitboards[strongSide][PAWN]) * PAWN_VALUE_EG
               + pushToEdge(weakKing)
               + pushClose(strongKing, weakKing);
    
    // Add known win bonus if mate is forceable
    if (canForceMate(board, strongSide)) {
        result = std::min(result + VALUE_KNOWN_WIN, VALUE_TB_WIN_IN_MAX_PLY - 1);
    }
    
    return result;
}

// KBNK: King + Bishop + Knight vs King
// Must drive king to corner that bishop can attack
int evaluateKBNK(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // If bishop doesn't attack A1/H8 (light squares), flip weak king to drive to A8/H1
    int adjustedWeakKing = oppositeColors(strongBishop, 0) ? Board::flipFile(weakKing) : weakKing;
    
    int result = VALUE_KNOWN_WIN + 3520
               + pushClose(strongKing, weakKing)
               + 420 * pushToCorner(adjustedWeakKing);
    
    return result;
}

// KNNK: King + Two Knights vs King - Trivial draw
int evaluateKNNK(const Board& board, Color strongSide) {
    (void)board;
    (void)strongSide;
    return VALUE_DRAW;
}

// KRKP: King + Rook vs King + Pawn
int evaluateKRKP(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int strongRook = Board::getLsb(board.bitboards[strongSide][ROOK]);
    int weakPawn = Board::getLsb(board.bitboards[weakSide][PAWN]);
    
    int queeningSquare = Board::position(Board::column(weakPawn), 
                                         weakSide == WHITE ? 7 : 0);
    int result;
    
    // If strong king is in front of pawn, it's a win
    if (forwardFileBB(strongSide, strongKing) & (1ULL << weakPawn)) {
        result = ROOK_VALUE_EG - Board::distance(strongKing, weakPawn);
    }
    // If weak king is too far from pawn and rook, it's a win
    else if (Board::distance(weakKing, weakPawn) >= 3 + (board.sideToMove == weakSide ? 1 : 0)
          && Board::distance(weakKing, strongRook) >= 3) {
        result = ROOK_VALUE_EG - Board::distance(strongKing, weakPawn);
    }
    // If pawn is far advanced and defended by king, position is drawish
    else if (Board::relativeRank(strongSide, weakKing) <= 2
          && Board::distance(weakKing, weakPawn) == 1
          && Board::relativeRank(strongSide, strongKing) >= 3
          && Board::distance(strongKing, weakPawn) > 2 + (board.sideToMove == strongSide ? 1 : 0)) {
        result = 80 - 8 * Board::distance(strongKing, weakPawn);
    }
    // General case
    else {
        int push = Board::pawnPush(weakSide);
        result = 200 - 8 * (Board::distance(strongKing, weakPawn + push)
                         - Board::distance(weakKing, weakPawn + push)
                         - Board::distance(weakPawn, queeningSquare));
    }
    
    return result;
}

// KRKB: King + Rook vs King + Bishop - Very drawish
int evaluateKRKB(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    return pushToEdge(weakKing);
}

// KRKN: King + Rook vs King + Knight
int evaluateKRKN(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakKnight = Board::getLsb(board.bitboards[weakSide][KNIGHT]);
    return pushToEdge(weakKing) + pushAway(weakKing, weakKnight);
}

// KQKP: King + Queen vs King + Pawn
int evaluateKQKP(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakPawn = Board::getLsb(board.bitboards[weakSide][PAWN]);
    
    int result = pushClose(strongKing, weakKing);
    
    // If pawn not on 7th rank, or wrong file, or king not adjacent: winning
    int pawnRelRank = Board::relativeRank(weakSide, weakPawn);
    int pawnFile = Board::column(weakPawn);
    
    if (pawnRelRank != 6  // Not on 7th rank
        || Board::distance(weakKing, weakPawn) != 1  // King not adjacent
        || (pawnFile != 0 && pawnFile != 2 && pawnFile != 5 && pawnFile != 7)) {  // Not a/c/f/h file
        result += QUEEN_VALUE_EG - PAWN_VALUE_EG;
    }
    
    return result;
}

// KQKR: King + Queen vs King + Rook
int evaluateKQKR(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    return QUEEN_VALUE_EG - ROOK_VALUE_EG
         + pushToEdge(weakKing)
         + pushClose(strongKing, weakKing);
}

// KNNKP: King + Two Knights vs King + Pawn - Very drawish
int evaluateKNNKP(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakPawn = Board::getLsb(board.bitboards[weakSide][PAWN]);
    
    return PAWN_VALUE_EG
         + 2 * pushToEdge(weakKing)
         - 10 * Board::relativeRank(weakSide, weakPawn);
}

// ============================================================================
// SCALING FUNCTIONS
// ============================================================================

// KBPsK: King + Bishop + Pawns vs King
// Detects wrong-colored bishop with rook pawn draws
int scaleKBPsK(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    uint64_t strongPawns = board.bitboards[strongSide][PAWN];
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // All pawns on a-file or h-file?
    if (!(strongPawns & ~FILE_A_BB) || !(strongPawns & ~FILE_H_BB)) {
        int pawnFile = Board::column(Board::getLsb(strongPawns));
        int queeningSquare = Board::relativeSquare(strongSide, 
                                                   Board::position(pawnFile, 7));
        
        // Wrong bishop color + weak king near promotion = draw
        if (oppositeColors(queeningSquare, strongBishop)
            && Board::distance(queeningSquare, weakKing) <= 1) {
            return SCALE_FACTOR_DRAW;
        }
    }
    
    // Check for fortress on b/g files
    uint64_t allPawns = board.bitboards[WHITE][PAWN] | board.bitboards[BLACK][PAWN];
    if ((!(allPawns & ~FILE_B_BB) || !(allPawns & ~FILE_G_BB))
        && nonPawnMaterial(board, weakSide) == 0
        && Board::popcount(board.bitboards[weakSide][PAWN]) >= 1) {
        
        // Get weakSide's most advanced pawn
        uint64_t weakPawns = board.bitboards[weakSide][PAWN];
        int weakPawn = (strongSide == WHITE) ? Board::getMsb(weakPawns) : Board::getLsb(weakPawns);
        
        // Potential draw if weak pawn on 7th rank
        if (Board::relativeRank(strongSide, weakPawn) == 6) {
            int push = Board::pawnPush(weakSide);
            // If strong pawn blocks it
            if (strongPawns & (1ULL << (weakPawn + push))) {
                if (oppositeColors(strongBishop, weakPawn) || !Board::moreThanOne(strongPawns)) {
                    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
                    int strongKingDist = Board::distance(weakPawn, strongKing);
                    int weakKingDist = Board::distance(weakPawn, weakKing);
                    
                    if (Board::relativeRank(strongSide, weakKing) >= 6
                        && weakKingDist <= 2
                        && weakKingDist <= strongKingDist) {
                        return SCALE_FACTOR_DRAW;
                    }
                }
            }
        }
    }
    
    return SCALE_FACTOR_NONE;
}

// KQKRPs: King + Queen vs King + Rook + Pawns
// Detects fortress draws
int scaleKQKRPs(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakRook = Board::getLsb(board.bitboards[weakSide][ROOK]);
    
    // Fortress: weak king on back ranks, rook on 3rd rank defended by pawn
    if (Board::relativeRank(weakSide, weakKing) <= 1
        && Board::relativeRank(weakSide, strongKing) >= 3
        && Board::relativeRank(weakSide, weakRook) == 2) {
        
        // Check if rook is defended by pawn
        uint64_t weakPawns = board.bitboards[weakSide][PAWN];
        uint64_t kingAttacks = Board::getKingAttacks(weakKing);
        uint64_t pawnAttacks = Board::getPawnAttacks(1ULL << weakRook, strongSide);
        
        if ((weakPawns & kingAttacks & pawnAttacks)) {
            return SCALE_FACTOR_DRAW;
        }
    }
    
    return SCALE_FACTOR_NONE;
}

// KRPKR: King + Rook + Pawn vs King + Rook
// Most common rook endgame with many drawing patterns
int scaleKRPKR(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    
    // Normalize position
    int strongKing = normalize(board, strongSide, Board::getLsb(board.bitboards[strongSide][KING]));
    int strongRook = normalize(board, strongSide, Board::getLsb(board.bitboards[strongSide][ROOK]));
    int strongPawn = normalize(board, strongSide, Board::getLsb(board.bitboards[strongSide][PAWN]));
    int weakKing = normalize(board, strongSide, Board::getLsb(board.bitboards[weakSide][KING]));
    int weakRook = normalize(board, strongSide, Board::getLsb(board.bitboards[weakSide][ROOK]));
    
    int pawnFile = Board::column(strongPawn);
    int pawnRank = Board::row(strongPawn);
    int queeningSquare = Board::position(pawnFile, 7);
    int tempo = (board.sideToMove == strongSide) ? 1 : 0;
    
    // Third-rank defense: pawn not too advanced, weak king near queening square
    if (pawnRank <= 4
        && Board::distance(weakKing, queeningSquare) <= 1
        && strongKing <= 39  // <= H5
        && (Board::row(weakRook) == 5 || (pawnRank <= 2 && Board::row(strongRook) != 5))) {
        return SCALE_FACTOR_DRAW;
    }
    
    // Back-rank defense: pawn on 6th, weak king near queening square
    if (pawnRank == 5
        && Board::distance(weakKing, queeningSquare) <= 1
        && Board::row(strongKing) + tempo <= 5
        && (Board::row(weakRook) == 0 || (!tempo && Board::columnDistance(weakRook, strongPawn) >= 3))) {
        return SCALE_FACTOR_DRAW;
    }
    
    // Weak king on queening square, weak rook on back rank
    if (pawnRank >= 5
        && weakKing == queeningSquare
        && Board::row(weakRook) == 0
        && (!tempo || Board::distance(strongKing, strongPawn) >= 2)) {
        return SCALE_FACTOR_DRAW;
    }
    
    // A7 pawn + A8 rook specific case
    if (strongPawn == 48  // A7
        && strongRook == 56  // A8
        && (weakKing == 55 || weakKing == 62)  // H7 or G7
        && Board::column(weakRook) == 0
        && (Board::row(weakRook) <= 2 || Board::column(strongKing) >= 3 || Board::row(strongKing) <= 4)) {
        return SCALE_FACTOR_DRAW;
    }
    
    // Weak king blocks pawn, strong king far away
    if (pawnRank <= 4
        && weakKing == strongPawn + 8
        && Board::distance(strongKing, strongPawn) - tempo >= 2
        && Board::distance(strongKing, weakRook) - tempo >= 2) {
        return SCALE_FACTOR_DRAW;
    }
    
    // Winning: pawn on 7th supported by rook from behind
    if (pawnRank == 6
        && pawnFile != 0
        && Board::column(strongRook) == pawnFile
        && strongRook != queeningSquare
        && Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, queeningSquare) - 2 + tempo
        && Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, strongRook) + tempo) {
        return SCALE_FACTOR_MAX - 2 * Board::distance(strongKing, queeningSquare);
    }
    
    // Pawn further back with rook support
    if (pawnFile != 0
        && Board::column(strongRook) == pawnFile
        && strongRook < strongPawn
        && Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, queeningSquare) - 2 + tempo
        && Board::distance(strongKing, strongPawn + 8) < Board::distance(weakKing, strongPawn + 8) - 2 + tempo
        && (Board::distance(weakKing, strongRook) + tempo >= 3
            || (Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, strongRook) + tempo
                && Board::distance(strongKing, strongPawn + 8) < Board::distance(weakKing, strongPawn) + tempo))) {
        return SCALE_FACTOR_MAX - 8 * Board::distance(strongPawn, queeningSquare)
                                - 2 * Board::distance(strongKing, queeningSquare);
    }
    
    // Weak king somewhere in pawn's path
    if (pawnRank <= 3 && weakKing > strongPawn) {
        if (Board::column(weakKing) == Board::column(strongPawn)) {
            return 10;
        }
        if (Board::columnDistance(weakKing, strongPawn) == 1
            && Board::distance(strongKing, weakKing) > 2) {
            return 24 - 2 * Board::distance(strongKing, weakKing);
        }
    }
    
    return SCALE_FACTOR_NONE;
}

// KRPKB: King + Rook + Pawn vs King + Bishop
int scaleKRPKB(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    uint64_t pawnBB = board.bitboards[PAWN][0] | board.bitboards[PAWN][1];
    
    // Only applies to rook pawns (a-file or h-file)
    if (!(pawnBB & (FILE_A_BB | FILE_H_BB))) {
        return SCALE_FACTOR_NONE;
    }
    
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakBishop = Board::getLsb(board.bitboards[weakSide][BISHOP]);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int strongPawn = Board::getLsb(board.bitboards[strongSide][PAWN]);
    int pawnRank = Board::relativeRank(strongSide, strongPawn);
    int push = Board::pawnPush(strongSide);
    
    // Pawn on 5th rank, same color as bishop
    if (pawnRank == 4 && !oppositeColors(weakBishop, strongPawn)) {
        int d = Board::distance(strongPawn + 3 * push, weakKing);
        if (d <= 2 && !(d == 0 && weakKing == strongKing + 2 * push)) {
            return 24;
        }
        return 48;
    }
    
    // Pawn on 6th rank
    if (pawnRank == 5
        && Board::distance(strongPawn + 2 * push, weakKing) <= 1
        && (Board::getBishopAttacks(weakBishop, board.allPiecesBB) & (1ULL << (strongPawn + push)))
        && Board::columnDistance(weakBishop, strongPawn) >= 2) {
        return 8;
    }
    
    return SCALE_FACTOR_NONE;
}

// KRPPKRP: King + Rook + 2 Pawns vs King + Rook + Pawn
int scaleKRPPKRP(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    uint64_t strongPawns = board.bitboards[strongSide][PAWN];
    int strongPawn1 = Board::getLsb(strongPawns);
    int strongPawn2 = Board::getMsb(strongPawns);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // Check for passed pawns - if any exist, no scaling
    uint64_t weakPawns = board.bitboards[weakSide][PAWN];
    uint64_t weakPawnSpan1 = passedPawnSpan(weakSide, Board::getLsb(weakPawns));
    
    // Simple check: if strong pawns are blocked by weak pawns/king, drawish
    bool pawn1Passed = !(weakPawns & passedPawnSpan(strongSide, strongPawn1));
    bool pawn2Passed = !(weakPawns & passedPawnSpan(strongSide, strongPawn2));
    
    if (pawn1Passed || pawn2Passed) {
        return SCALE_FACTOR_NONE;
    }
    
    int pawnRank = std::max(Board::relativeRank(strongSide, strongPawn1),
                           Board::relativeRank(strongSide, strongPawn2));
    
    // Weak king between/in front of pawns
    if (Board::columnDistance(weakKing, strongPawn1) <= 1
        && Board::columnDistance(weakKing, strongPawn2) <= 1
        && Board::relativeRank(strongSide, weakKing) > pawnRank) {
        return 7 * pawnRank;
    }
    
    return SCALE_FACTOR_NONE;
}

// KPsK: King + Pawns vs King
// Detects blocked rook pawn draws
int scaleKPsK(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    uint64_t strongPawns = board.bitboards[strongSide][PAWN];
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // All pawns on a-file or h-file?
    if (!(strongPawns & ~FILE_A_BB) || !(strongPawns & ~FILE_H_BB)) {
        // Check if all pawns are blocked by weak king
        uint64_t span = passedPawnSpan(weakSide, weakKing);
        if (!(strongPawns & ~span)) {
            return SCALE_FACTOR_DRAW;
        }
    }
    
    return SCALE_FACTOR_NONE;
}

// KBPKB: King + Bishop + Pawn vs King + Bishop
int scaleKBPKB(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongPawn = Board::getLsb(board.bitboards[strongSide][PAWN]);
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakBishop = Board::getLsb(board.bitboards[weakSide][BISHOP]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // Case 1: Weak king blocks pawn path, wrong bishop color
    uint64_t forwardFile = forwardFileBB(strongSide, strongPawn);
    if ((forwardFile & (1ULL << weakKing))
        && (oppositeColors(weakKing, strongBishop)
            || Board::relativeRank(strongSide, weakKing) <= 5)) {
        return SCALE_FACTOR_DRAW;
    }
    
    // Case 2: Opposite colored bishops
    if (oppositeColors(strongBishop, weakBishop)) {
        return SCALE_FACTOR_DRAW;
    }
    
    return SCALE_FACTOR_NONE;
}

// KBPPKB: King + Bishop + 2 Pawns vs King + Bishop
int scaleKBPPKB(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakBishop = Board::getLsb(board.bitboards[weakSide][BISHOP]);
    
    // Must be opposite colored bishops
    if (!oppositeColors(strongBishop, weakBishop)) {
        return SCALE_FACTOR_NONE;
    }
    
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    uint64_t strongPawns = board.bitboards[strongSide][PAWN];
    int strongPawn1 = Board::getLsb(strongPawns);
    int strongPawn2 = Board::getMsb(strongPawns);
    
    int push = Board::pawnPush(strongSide);
    int pawn1Rank = Board::relativeRank(strongSide, strongPawn1);
    int pawn2Rank = Board::relativeRank(strongSide, strongPawn2);
    
    int blockSq1, blockSq2;
    if (pawn1Rank > pawn2Rank) {
        blockSq1 = strongPawn1 + push;
        blockSq2 = Board::position(Board::column(strongPawn2), Board::row(strongPawn1));
    } else {
        blockSq1 = strongPawn2 + push;
        blockSq2 = Board::position(Board::column(strongPawn1), Board::row(strongPawn2));
    }
    
    int fileDist = Board::columnDistance(strongPawn1, strongPawn2);
    
    switch (fileDist) {
    case 0:
        // Same file: draw if weak king controls blocking square
        if (Board::column(weakKing) == Board::column(blockSq1)
            && Board::relativeRank(strongSide, weakKing) >= Board::relativeRank(strongSide, blockSq1)
            && oppositeColors(weakKing, strongBishop)) {
            return SCALE_FACTOR_DRAW;
        }
        break;
        
    case 1:
        // Adjacent files
        if (weakKing == blockSq1
            && oppositeColors(weakKing, strongBishop)
            && (weakBishop == blockSq2
                || (Board::getBishopAttacks(blockSq2, board.allPiecesBB) & board.bitboards[weakSide][BISHOP])
                || std::abs(pawn1Rank - pawn2Rank) >= 2)) {
            return SCALE_FACTOR_DRAW;
        }
        if (weakKing == blockSq2
            && oppositeColors(weakKing, strongBishop)
            && (weakBishop == blockSq1
                || (Board::getBishopAttacks(blockSq1, board.allPiecesBB) & board.bitboards[weakSide][BISHOP]))) {
            return SCALE_FACTOR_DRAW;
        }
        break;
        
    default:
        // Pawns far apart: no scaling
        break;
    }
    
    return SCALE_FACTOR_NONE;
}

// KBPKN: King + Bishop + Pawn vs King + Knight
int scaleKBPKN(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongPawn = Board::getLsb(board.bitboards[strongSide][PAWN]);
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // Weak king on pawn's file, in front of pawn, wrong bishop color
    if (Board::column(weakKing) == Board::column(strongPawn)
        && Board::relativeRank(strongSide, strongPawn) < Board::relativeRank(strongSide, weakKing)
        && (oppositeColors(weakKing, strongBishop)
            || Board::relativeRank(strongSide, weakKing) <= 5)) {
        return SCALE_FACTOR_DRAW;
    }
    
    return SCALE_FACTOR_NONE;
}

// ============================================================================
// DETECTION FUNCTION
// ============================================================================

std::optional<EndgameInfo> detectEndgame(const Board& board) {
    // Count material for both sides
    int wN = Board::popcount(board.bitboards[WHITE][KNIGHT]);
    int wB = Board::popcount(board.bitboards[WHITE][BISHOP]);
    int wR = Board::popcount(board.bitboards[WHITE][ROOK]);
    int wQ = Board::popcount(board.bitboards[WHITE][QUEEN]);
    int wP = Board::popcount(board.bitboards[WHITE][PAWN]);
    
    int bN = Board::popcount(board.bitboards[BLACK][KNIGHT]);
    int bB = Board::popcount(board.bitboards[BLACK][BISHOP]);
    int bR = Board::popcount(board.bitboards[BLACK][ROOK]);
    int bQ = Board::popcount(board.bitboards[BLACK][QUEEN]);
    int bP = Board::popcount(board.bitboards[BLACK][PAWN]);
    
    int wTotal = wN + wB + wR + wQ + wP;
    int bTotal = bN + bB + bR + bQ + bP;
    
    // ========================================
    // EVALUATION ENDGAMES (return absolute score)
    // ========================================
    
    // KNNK: Two knights vs lone king (trivial draw)
    if (wN == 2 && wB == 0 && wR == 0 && wQ == 0 && wP == 0 && bTotal == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KNNK, true};
    }
    if (bN == 2 && bB == 0 && bR == 0 && bQ == 0 && bP == 0 && wTotal == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KNNK, true};
    }
    
    // KBNK: Bishop + Knight vs lone king
    if (wB == 1 && wN == 1 && wR == 0 && wQ == 0 && wP == 0 && bTotal == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KBNK, true};
    }
    if (bB == 1 && bN == 1 && bR == 0 && bQ == 0 && bP == 0 && wTotal == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KBNK, true};
    }
    
    // KQKR: Queen vs Rook
    if (wQ == 1 && wN == 0 && wB == 0 && wR == 0 && wP == 0 
        && bR == 1 && bN == 0 && bB == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KQKR, true};
    }
    if (bQ == 1 && bN == 0 && bB == 0 && bR == 0 && bP == 0 
        && wR == 1 && wN == 0 && wB == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KQKR, true};
    }
    
    // KQKP: Queen vs Pawn
    if (wQ == 1 && wN == 0 && wB == 0 && wR == 0 && wP == 0 
        && bP == 1 && bN == 0 && bB == 0 && bR == 0 && bQ == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KQKP, true};
    }
    if (bQ == 1 && bN == 0 && bB == 0 && bR == 0 && bP == 0 
        && wP == 1 && wN == 0 && wB == 0 && wR == 0 && wQ == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KQKP, true};
    }
    
    // KRKP: Rook vs Pawn
    if (wR == 1 && wN == 0 && wB == 0 && wQ == 0 && wP == 0 
        && bP == 1 && bN == 0 && bB == 0 && bR == 0 && bQ == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KRKP, true};
    }
    if (bR == 1 && bN == 0 && bB == 0 && bQ == 0 && bP == 0 
        && wP == 1 && wN == 0 && wB == 0 && wR == 0 && wQ == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KRKP, true};
    }
    
    // KRKB: Rook vs Bishop
    if (wR == 1 && wN == 0 && wB == 0 && wQ == 0 && wP == 0 
        && bB == 1 && bN == 0 && bR == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KRKB, true};
    }
    if (bR == 1 && bN == 0 && bB == 0 && bQ == 0 && bP == 0 
        && wB == 1 && wN == 0 && wR == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KRKB, true};
    }
    
    // KRKN: Rook vs Knight
    if (wR == 1 && wN == 0 && wB == 0 && wQ == 0 && wP == 0 
        && bN == 1 && bB == 0 && bR == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KRKN, true};
    }
    if (bR == 1 && bN == 0 && bB == 0 && bQ == 0 && bP == 0 
        && wN == 1 && wB == 0 && wR == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KRKN, true};
    }
    
    // KNNKP: Two Knights vs Pawn
    if (wN == 2 && wB == 0 && wR == 0 && wQ == 0 && wP == 0 
        && bP == 1 && bN == 0 && bB == 0 && bR == 0 && bQ == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KNNKP, true};
    }
    if (bN == 2 && bB == 0 && bR == 0 && bQ == 0 && bP == 0 
        && wP == 1 && wN == 0 && wB == 0 && wR == 0 && wQ == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KNNKP, true};
    }
    
    // KXK: Generic overwhelming material vs lone king
    if (bTotal == 0 && (wQ + wR + wB + wN) > 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KXK, true};
    }
    if (wTotal == 0 && (bQ + bR + bB + bN) > 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KXK, true};
    }
    
    // ========================================
    // SCALING ENDGAMES (return scale factor)
    // ========================================
    
    // KBPKB: Bishop + Pawn vs Bishop
    if (wB == 1 && wP == 1 && wN == 0 && wR == 0 && wQ == 0 
        && bB == 1 && bN == 0 && bR == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KBPKB, false};
    }
    if (bB == 1 && bP == 1 && bN == 0 && bR == 0 && bQ == 0 
        && wB == 1 && wN == 0 && wR == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KBPKB, false};
    }
    
    // KBPPKB: Bishop + 2 Pawns vs Bishop
    if (wB == 1 && wP == 2 && wN == 0 && wR == 0 && wQ == 0 
        && bB == 1 && bN == 0 && bR == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KBPPKB, false};
    }
    if (bB == 1 && bP == 2 && bN == 0 && bR == 0 && bQ == 0 
        && wB == 1 && wN == 0 && wR == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KBPPKB, false};
    }
    
    // KBPKN: Bishop + Pawn vs Knight
    if (wB == 1 && wP == 1 && wN == 0 && wR == 0 && wQ == 0 
        && bN == 1 && bB == 0 && bR == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KBPKN, false};
    }
    if (bB == 1 && bP == 1 && bN == 0 && bR == 0 && bQ == 0 
        && wN == 1 && wB == 0 && wR == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KBPKN, false};
    }
    
    // KRPKR: Rook + Pawn vs Rook
    if (wR == 1 && wP == 1 && wN == 0 && wB == 0 && wQ == 0 
        && bR == 1 && bN == 0 && bB == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KRPKR, false};
    }
    if (bR == 1 && bP == 1 && bN == 0 && bB == 0 && bQ == 0 
        && wR == 1 && wN == 0 && wB == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KRPKR, false};
    }
    
    // KRPKB: Rook + Pawn vs Bishop
    if (wR == 1 && wP == 1 && wN == 0 && wB == 0 && wQ == 0 
        && bB == 1 && bN == 0 && bR == 0 && bQ == 0 && bP == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KRPKB, false};
    }
    if (bR == 1 && bP == 1 && bN == 0 && bB == 0 && bQ == 0 
        && wB == 1 && wN == 0 && wR == 0 && wQ == 0 && wP == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KRPKB, false};
    }
    
    // KRPPKRP: Rook + 2 Pawns vs Rook + Pawn
    if (wR == 1 && wP == 2 && wN == 0 && wB == 0 && wQ == 0 
        && bR == 1 && bP == 1 && bN == 0 && bB == 0 && bQ == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KRPPKRP, false};
    }
    if (bR == 1 && bP == 2 && bN == 0 && bB == 0 && bQ == 0 
        && wR == 1 && wP == 1 && wN == 0 && wB == 0 && wQ == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KRPPKRP, false};
    }
    
    // KBPsK: Bishop + Pawns vs lone King (any number of pawns)
    if (wB == 1 && wP >= 1 && wN == 0 && wR == 0 && wQ == 0 && bTotal == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KBPsK, false};
    }
    if (bB == 1 && bP >= 1 && bN == 0 && bR == 0 && bQ == 0 && wTotal == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KBPsK, false};
    }
    
    // KPsK: Pawns vs lone King (any number of pawns, no pieces)
    if (wP >= 2 && wN == 0 && wB == 0 && wR == 0 && wQ == 0 && bTotal == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KPsK, false};
    }
    if (bP >= 2 && bN == 0 && bB == 0 && bR == 0 && bQ == 0 && wTotal == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KPsK, false};
    }
    
    // KQKRPs: Queen vs Rook + Pawns
    if (wQ == 1 && wN == 0 && wB == 0 && wR == 0 && wP == 0 
        && bR == 1 && bP >= 1 && bN == 0 && bB == 0 && bQ == 0) {
        return EndgameInfo{WHITE, BLACK, SCALE_KQKRPs, false};
    }
    if (bQ == 1 && bN == 0 && bB == 0 && bR == 0 && bP == 0 
        && wR == 1 && wP >= 1 && wN == 0 && wB == 0 && wQ == 0) {
        return EndgameInfo{BLACK, WHITE, SCALE_KQKRPs, false};
    }
    
    return std::nullopt;
}

// ============================================================================
// MAIN API FUNCTIONS
// ============================================================================

int evaluate(const Board& board, const EndgameInfo& info) {
    int value;
    
    switch (info.type) {
    case ENDGAME_KXK:
        value = evaluateKXK(board, info.strongSide);
        break;
    case ENDGAME_KBNK:
        value = evaluateKBNK(board, info.strongSide);
        break;
    case ENDGAME_KNNK:
        value = evaluateKNNK(board, info.strongSide);
        break;
    case ENDGAME_KRKP:
        value = evaluateKRKP(board, info.strongSide);
        break;
    case ENDGAME_KRKB:
        value = evaluateKRKB(board, info.strongSide);
        break;
    case ENDGAME_KRKN:
        value = evaluateKRKN(board, info.strongSide);
        break;
    case ENDGAME_KQKP:
        value = evaluateKQKP(board, info.strongSide);
        break;
    case ENDGAME_KQKR:
        value = evaluateKQKR(board, info.strongSide);
        break;
    case ENDGAME_KNNKP:
        value = evaluateKNNKP(board, info.strongSide);
        break;
    default:
        value = 0;
        break;
    }
    
    return value;
}

int getScaleFactor(const Board& board, const EndgameInfo& info) {
    switch (info.type) {
    case SCALE_KBPsK:
        return scaleKBPsK(board, info.strongSide);
    case SCALE_KQKRPs:
        return scaleKQKRPs(board, info.strongSide);
    case SCALE_KRPKR:
        return scaleKRPKR(board, info.strongSide);
    case SCALE_KRPKB:
        return scaleKRPKB(board, info.strongSide);
    case SCALE_KRPPKRP:
        return scaleKRPPKRP(board, info.strongSide);
    case SCALE_KPsK:
        return scaleKPsK(board, info.strongSide);
    case SCALE_KBPKB:
        return scaleKBPKB(board, info.strongSide);
    case SCALE_KBPPKB:
        return scaleKBPPKB(board, info.strongSide);
    case SCALE_KBPKN:
        return scaleKBPKN(board, info.strongSide);
    default:
        return SCALE_FACTOR_NONE;
    }
}

} // namespace Endgame
