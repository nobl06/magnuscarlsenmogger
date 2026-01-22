#include "endgame.h"
#include "../gen.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>

namespace Endgame {

// ============================================================================
// CONSTANTS
// ============================================================================

// Dark and light squares (Stockfish: DarkSquares)
constexpr uint64_t DarkSquares = 0xAA55AA55AA55AA55ULL;

// File bitboards
constexpr uint64_t FileABB = 0x0101010101010101ULL;
constexpr uint64_t FileBBB = 0x0202020202020202ULL;
constexpr uint64_t FileDBB = 0x0808080808080808ULL;
constexpr uint64_t FileEBB = 0x1010101010101010ULL;
constexpr uint64_t FileGBB = 0x4040404040404040ULL;
constexpr uint64_t FileHBB = 0x8080808080808080ULL;

// File constants
constexpr int FILE_A = 0;
constexpr int FILE_B = 1;
constexpr int FILE_D = 3;
constexpr int FILE_E = 4;
constexpr int FILE_G = 6;
constexpr int FILE_H = 7;

// Rank constants
constexpr int RANK_1 = 0;
constexpr int RANK_2 = 1;
constexpr int RANK_3 = 2;
constexpr int RANK_4 = 3;
constexpr int RANK_5 = 4;
constexpr int RANK_6 = 5;
constexpr int RANK_7 = 6;
constexpr int RANK_8 = 7;

// Square constants
constexpr int SQ_A7 = 6 * 8 + 0;  // 48
constexpr int SQ_A8 = 7 * 8 + 0;  // 56
constexpr int SQ_G7 = 6 * 8 + 6;  // 54
constexpr int SQ_H5 = 4 * 8 + 7;  // 39
constexpr int SQ_H7 = 6 * 8 + 7;  // 55

// Direction constant
constexpr int NORTH = 8;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Used to drive the king towards the edge of the board
// in KX vs K and KQ vs KR endgames.
// Values range from 27 (center squares) to 90 (in the corners)
int push_to_edge(int sq) {
    int rd = Board::rankOrFileEdgeDistance(Board::row(sq));
    int fd = Board::rankOrFileEdgeDistance(Board::column(sq));
    return 90 - (7 * fd * fd / 2 + 7 * rd * rd / 2);
}

// Used to drive the king towards A1H8 corners in KBN vs K endgames.
// Values range from 0 on A8H1 diagonal to 7 in A1H8 corners
int push_to_corner(int sq) {
    return std::abs(7 - Board::row(sq) - Board::column(sq));
}

// Drive two pieces close together
int push_close(int sq1, int sq2) {
    return 140 - 20 * Board::distance(sq1, sq2);
}

// Drive two pieces apart
int push_away(int sq1, int sq2) {
    return 120 - push_close(sq1, sq2);
}

// Check if two squares are on opposite colors
static bool oppositeColors(int sq1, int sq2) {
    int s = sq1 ^ sq2;
    return ((s >> 3) ^ s) & 1;
}

// Get forward file bitboard (all squares ahead on same file)
static uint64_t forward_file_bb(Color c, int sq) {
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

// Get passed pawn span (squares a pawn must pass through, including adjacent files)
static uint64_t passed_pawn_span(Color c, int sq) {
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

// Map the square as if strongSide is white and strongSide's only pawn
// is on the left half of the board (files A-D).
static int normalize(const Board& board, Color strongSide, int sq) {
    // If strong side's pawn is on right half (files e-h), flip file
    uint64_t strongPawns = board.bitboards[strongSide][PAWN];
    if (strongPawns) {
        int pawnSq = Board::getLsb(strongPawns);
        if (Board::column(pawnSq) >= FILE_E) {
            sq = Board::flipFile(sq);
        }
    }
    // If strong side is black, flip rank
    return strongSide == WHITE ? sq : Board::flipRank(sq);
}

// ============================================================================
// EVALUATION FUNCTIONS
// ============================================================================

/// Mate with KX vs K. This function is used to evaluate positions with
/// king and plenty of material vs a lone king. It simply gives the
/// attacking side a bonus for driving the defending king towards the edge
/// of the board, and for keeping the distance between the two kings small.
int evaluateKXK(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    
    // Stalemate detection with lone king
    if (board.sideToMove == weakSide) {
        Board& mutableBoard = const_cast<Board&>(board);
        MoveGenerator gen(mutableBoard, weakSide);
        Move pseudoLegal[220];
        size_t pseudoLegalCount = gen.generatePseudoLegalMoves(pseudoLegal);
        Move legalMoves[220];
        size_t legalCount = gen.filterLegalMoves(pseudoLegal, pseudoLegalCount, legalMoves);
        if (legalCount == 0) {
            return VALUE_DRAW;
        }
    }
    
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // Calculate non-pawn material
    int npm = Board::popcount(board.bitboards[strongSide][KNIGHT]) * KnightValueMg
            + Board::popcount(board.bitboards[strongSide][BISHOP]) * BishopValueMg
            + Board::popcount(board.bitboards[strongSide][ROOK]) * RookValueMg
            + Board::popcount(board.bitboards[strongSide][QUEEN]) * QueenValueMg;
    
    int result = npm
               + Board::popcount(board.bitboards[strongSide][PAWN]) * PawnValueEg
               + push_to_edge(weakKing)
               + push_close(strongKing, weakKing);
    
    // Check if mate is forceable (Stockfish logic exactly)
    bool canForceMate = false;
    if (board.bitboards[strongSide][QUEEN]) {
        canForceMate = true;
    } else if (board.bitboards[strongSide][ROOK]) {
        canForceMate = true;
    } else if (Board::popcount(board.bitboards[strongSide][BISHOP]) >= 1 && 
               Board::popcount(board.bitboards[strongSide][KNIGHT]) >= 1) {
        canForceMate = true;
    } else {
        // Two bishops on different colors can mate
        uint64_t bishopBB = board.bitboards[strongSide][BISHOP];
        if ((bishopBB & ~DarkSquares) && (bishopBB & DarkSquares)) {
            canForceMate = true;
        }
    }
    
    if (canForceMate) {
        result = std::min(result + VALUE_KNOWN_WIN, VALUE_TB_WIN_IN_MAX_PLY - 1);
    }
    
    return result;
}

/// Mate with KBN vs K. This is similar to KX vs K, but we have to drive the
/// defending king towards a corner square that our bishop attacks.
int evaluateKBNK(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    // If our bishop does not attack A1/H8, we flip the enemy king square
    // to drive to opposite corners (A8/H1).
    // SQ_A1 = 0 (dark square)
    int adjustedWeakKing = oppositeColors(strongBishop, 0) ? Board::flipFile(weakKing) : weakKing;
    
    int result = (VALUE_KNOWN_WIN + 3520)
               + push_close(strongKing, weakKing)
               + 420 * push_to_corner(adjustedWeakKing);
    
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
    
    // Queening square for the pawn
    int queeningSquare = Board::position(Board::column(weakPawn), 
                                         weakSide == WHITE ? RANK_8 : RANK_1);
    int result;
    
    // If the stronger side's king is in front of the pawn, it's a win
    if (forward_file_bb(strongSide, strongKing) & (1ULL << weakPawn)) {
        result = RookValueEg - Board::distance(strongKing, weakPawn);
    }
    // If the weaker side's king is too far from the pawn and the rook, it's a win
    else if (Board::distance(weakKing, weakPawn) >= 3 + (board.sideToMove == weakSide ? 1 : 0)
          && Board::distance(weakKing, strongRook) >= 3) {
        result = RookValueEg - Board::distance(strongKing, weakPawn);
    }
    // If the pawn is far advanced and supported by the defending king,
    // the position is drawish
    else if (Board::relativeRank(strongSide, weakKing) <= RANK_3
          && Board::distance(weakKing, weakPawn) == 1
          && Board::relativeRank(strongSide, strongKing) >= RANK_4
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
    return push_to_edge(weakKing);
}

// KRKN: King + Rook vs King + Knight
int evaluateKRKN(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakKnight = Board::getLsb(board.bitboards[weakSide][KNIGHT]);
    return push_to_edge(weakKing) + push_away(weakKing, weakKnight);
}

// KQKP: King + Queen vs King + Pawn
int evaluateKQKP(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakPawn = Board::getLsb(board.bitboards[weakSide][PAWN]);
    
    int result = push_close(strongKing, weakKing);
    
    int pawnRelRank = Board::relativeRank(weakSide, weakPawn);
    int pawnFile = Board::column(weakPawn);
    
    // (FileBBB | FileDBB | FileEBB | FileGBB) & weakPawn
    // This means NOT on files A, C, F, H (i.e., on B, D, E, G)
    // So draw conditions are: pawn on 7th AND king adjacent AND pawn on A/C/F/H file
    if (pawnRelRank != RANK_7
        || Board::distance(weakKing, weakPawn) != 1
        || (pawnFile == FILE_B || pawnFile == FILE_D || pawnFile == FILE_E || pawnFile == FILE_G)) {
        result += QueenValueEg - PawnValueEg;
    }
    
    return result;
}

/// KQ vs KR. This is almost identical to KX vs K: we give the attacking
/// king a bonus for having the kings close together, and for forcing the
/// defending king towards the edge.
int evaluateKQKR(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    
    return QueenValueEg - RookValueEg
         + push_to_edge(weakKing)
         + push_close(strongKing, weakKing);
}

// KNNKP: King + Two Knights vs King + Pawn - Very drawish
int evaluateKNNKP(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int weakPawn = Board::getLsb(board.bitboards[weakSide][PAWN]);
    
    return PawnValueEg
         + 2 * push_to_edge(weakKing)
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
    uint64_t allPawns = board.bitboards[WHITE][PAWN] | board.bitboards[BLACK][PAWN];
    int strongBishop = Board::getLsb(board.bitboards[strongSide][BISHOP]);
    int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
    int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
    
    // All strongSide pawns are on a single rook file?
    if (!(strongPawns & ~FileABB) || !(strongPawns & ~FileHBB)) {
        int pawnSq = Board::getLsb(strongPawns);
        int queeningSquare = Board::relativeSquare(strongSide, 
                                                   Board::position(Board::column(pawnSq), RANK_8));
        
        // Wrong bishop color + weak king near promotion = draw
        if (oppositeColors(queeningSquare, strongBishop)
            && Board::distance(queeningSquare, weakKing) <= 1) {
            return SCALE_FACTOR_DRAW;
        }
    }
    
    // If all the pawns are on the same B or G file, then it's potentially a draw
    if ((!(allPawns & ~FileBBB) || !(allPawns & ~FileGBB))
        && (Board::popcount(board.bitboards[weakSide][KNIGHT]) * KnightValueMg +
            Board::popcount(board.bitboards[weakSide][BISHOP]) * BishopValueMg +
            Board::popcount(board.bitboards[weakSide][ROOK]) * RookValueMg +
            Board::popcount(board.bitboards[weakSide][QUEEN]) * QueenValueMg) == 0
        && Board::popcount(board.bitboards[weakSide][PAWN]) >= 1) {
        
        // Get the least advanced weakSide pawn (frontmost from strongSide's view)
        uint64_t weakPawns = board.bitboards[weakSide][PAWN];
        int weakPawn = (strongSide == WHITE) ? Board::getMsb(weakPawns) : Board::getLsb(weakPawns);
        
        // There's potential for a draw if our pawn is blocked on the 7th rank,
        // the bishop cannot attack it or they only have one pawn left.
        if (Board::relativeRank(strongSide, weakPawn) == RANK_7
            && (strongPawns & (1ULL << (weakPawn + Board::pawnPush(weakSide))))
            && (oppositeColors(strongBishop, weakPawn) || !Board::moreThanOne(strongPawns))) {
            
            int strongKingDist = Board::distance(weakPawn, strongKing);
            int weakKingDist = Board::distance(weakPawn, weakKing);
            
            // It's a draw if the weak king is on its back two ranks, within 2
            // squares of the blocking pawn and the strong king is not closer.
            if (Board::relativeRank(strongSide, weakKing) >= RANK_7
                && weakKingDist <= 2
                && weakKingDist <= strongKingDist) {
                return SCALE_FACTOR_DRAW;
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
    
    // Fortress: weak king on back ranks, strong king far, rook on 3rd rank defended by pawn
    if (Board::relativeRank(weakSide, weakKing) <= RANK_2
        && Board::relativeRank(weakSide, strongKing) >= RANK_4
        && Board::relativeRank(weakSide, weakRook) == RANK_3) {
        
        // Check if rook is defended by pawn and near king
        uint64_t weakPawns = board.bitboards[weakSide][PAWN];
        uint64_t kingAttacks = Board::getKingAttacks(weakKing);
        uint64_t pawnAttacks = Board::getPawnAttacks(1ULL << weakRook, strongSide);
        
        if (weakPawns & kingAttacks & pawnAttacks) {
            return SCALE_FACTOR_DRAW;
        }
    }
    
    return SCALE_FACTOR_NONE;
}

// KRPKR: King + Rook + Pawn vs King + Rook
// Most common rook endgame with many drawing patterns
int scaleKRPKR(const Board& board, Color strongSide) {
    Color weakSide = (Color)(1 - strongSide);
    
    // Assume strongSide is white and the pawn is on files A-D
    int strongKing = normalize(board, strongSide, Board::getLsb(board.bitboards[strongSide][KING]));
    int strongRook = normalize(board, strongSide, Board::getLsb(board.bitboards[strongSide][ROOK]));
    int strongPawn = normalize(board, strongSide, Board::getLsb(board.bitboards[strongSide][PAWN]));
    int weakKing = normalize(board, strongSide, Board::getLsb(board.bitboards[weakSide][KING]));
    int weakRook = normalize(board, strongSide, Board::getLsb(board.bitboards[weakSide][ROOK]));
    
    int pawnFile = Board::column(strongPawn);
    int pawnRank = Board::row(strongPawn);
    int queeningSquare = Board::position(pawnFile, RANK_8);
    int tempo = (board.sideToMove == strongSide) ? 1 : 0;
    
    // If the pawn is not too far advanced and the defending king defends the
    // queening square, use the third-rank defence.
    if (pawnRank <= RANK_5
        && Board::distance(weakKing, queeningSquare) <= 1
        && strongKing <= SQ_H5
        && (Board::row(weakRook) == RANK_6 || (pawnRank <= RANK_3 && Board::row(strongRook) != RANK_6))) {
        return SCALE_FACTOR_DRAW;
    }
    
    // The defending side saves a draw by checking from behind in case the pawn
    // has advanced to the 6th rank with the king behind.
    if (pawnRank == RANK_6
        && Board::distance(weakKing, queeningSquare) <= 1
        && Board::row(strongKing) + tempo <= RANK_6
        && (Board::row(weakRook) == RANK_1 || (!tempo && Board::columnDistance(weakRook, strongPawn) >= 3))) {
        return SCALE_FACTOR_DRAW;
    }
    
    if (pawnRank >= RANK_6
        && weakKing == queeningSquare
        && Board::row(weakRook) == RANK_1
        && (!tempo || Board::distance(strongKing, strongPawn) >= 2)) {
        return SCALE_FACTOR_DRAW;
    }
    
    // White pawn on a7 and rook on a8 is a draw if black's king is on g7 or h7
    // and the black rook is behind the pawn.
    if (strongPawn == SQ_A7
        && strongRook == SQ_A8
        && (weakKing == SQ_H7 || weakKing == SQ_G7)
        && Board::column(weakRook) == FILE_A
        && (Board::row(weakRook) <= RANK_3 || Board::column(strongKing) >= FILE_D || Board::row(strongKing) <= RANK_5)) {
        return SCALE_FACTOR_DRAW;
    }
    
    // If the defending king blocks the pawn and the attacking king is too far
    // away, it's a draw.
    if (pawnRank <= RANK_5
        && weakKing == strongPawn + NORTH
        && Board::distance(strongKing, strongPawn) - tempo >= 2
        && Board::distance(strongKing, weakRook) - tempo >= 2) {
        return SCALE_FACTOR_DRAW;
    }
    
    // Pawn on the 7th rank supported by the rook from behind usually wins if the
    // attacking king is closer to the queening square than the defending king,
    // and the defending king cannot gain tempi by threatening the attacking rook.
    if (pawnRank == RANK_7
        && pawnFile != FILE_A
        && Board::column(strongRook) == pawnFile
        && strongRook != queeningSquare
        && (Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, queeningSquare) - 2 + tempo)
        && (Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, strongRook) + tempo)) {
        return SCALE_FACTOR_MAX - 2 * Board::distance(strongKing, queeningSquare);
    }
    
    // Similar to the above, but with the pawn further back
    if (pawnFile != FILE_A
        && Board::column(strongRook) == pawnFile
        && strongRook < strongPawn
        && (Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, queeningSquare) - 2 + tempo)
        && (Board::distance(strongKing, strongPawn + NORTH) < Board::distance(weakKing, strongPawn + NORTH) - 2 + tempo)
        && (Board::distance(weakKing, strongRook) + tempo >= 3
            || (Board::distance(strongKing, queeningSquare) < Board::distance(weakKing, strongRook) + tempo
                && Board::distance(strongKing, strongPawn + NORTH) < Board::distance(weakKing, strongPawn) + tempo))) {
        return SCALE_FACTOR_MAX
             - 8 * Board::distance(strongPawn, queeningSquare)
             - 2 * Board::distance(strongKing, queeningSquare);
    }
    
    // If the pawn is not far advanced and the defending king is somewhere in
    // the pawn's path, it's probably a draw.
    if (pawnRank <= RANK_4 && weakKing > strongPawn) {
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
    uint64_t pawnBB = board.bitboards[WHITE][PAWN] | board.bitboards[BLACK][PAWN];
    
    // Test for a rook pawn
    if (pawnBB & (FileABB | FileHBB)) {
        int weakKing = Board::getLsb(board.bitboards[weakSide][KING]);
        int weakBishop = Board::getLsb(board.bitboards[weakSide][BISHOP]);
        int strongKing = Board::getLsb(board.bitboards[strongSide][KING]);
        int strongPawn = Board::getLsb(board.bitboards[strongSide][PAWN]);
        int pawnRank = Board::relativeRank(strongSide, strongPawn);
        int push = Board::pawnPush(strongSide);
        
        // If the pawn is on the 5th rank and the pawn (currently) is on
        // the same color square as the bishop then there is a chance of
        // a fortress.
        if (pawnRank == RANK_5 && !oppositeColors(weakBishop, strongPawn)) {
            int d = Board::distance(strongPawn + 3 * push, weakKing);
            
            if (d <= 2 && !(d == 0 && weakKing == strongKing + 2 * push)) {
                return 24;
            } else {
                return 48;
            }
        }
        
        // When the pawn has moved to the 6th rank we can be fairly sure
        // it's drawn if the bishop attacks the square in front of the
        // pawn from a reasonable distance and the defending king is near
        // the corner
        if (pawnRank == RANK_6
            && Board::distance(strongPawn + 2 * push, weakKing) <= 1
            && (Board::getBishopAttacks(weakBishop, board.allPiecesBB) & (1ULL << (strongPawn + push)))
            && Board::columnDistance(weakBishop, strongPawn) >= 2) {
            return 8;
        }
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
    
    // Simple check: if strong pawns have passed pawn potential, no scaling
    bool pawn1Passed = !(weakPawns & passed_pawn_span(strongSide, strongPawn1));
    bool pawn2Passed = !(weakPawns & passed_pawn_span(strongSide, strongPawn2));
    
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
    
    // If all pawns are ahead of the king on a single rook file, it's a draw.
    if (!(strongPawns & ~(FileABB | FileHBB))
        && !(strongPawns & ~passed_pawn_span(weakSide, weakKing))) {
        return SCALE_FACTOR_DRAW;
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
    
    // Case 1: Defending king blocks the pawn, and cannot be driven away
    if ((forward_file_bb(strongSide, strongPawn) & (1ULL << weakKing))
        && (oppositeColors(weakKing, strongBishop)
            || Board::relativeRank(strongSide, weakKing) <= RANK_6)) {
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
        // Both pawns are on the same file. It's an easy draw if the defender firmly
        // controls some square in the frontmost pawn's path.
        if (Board::column(weakKing) == Board::column(blockSq1)
            && Board::relativeRank(strongSide, weakKing) >= Board::relativeRank(strongSide, blockSq1)
            && oppositeColors(weakKing, strongBishop)) {
            return SCALE_FACTOR_DRAW;
        }
        return SCALE_FACTOR_NONE;
        
    case 1:
        // Pawns on adjacent files. It's a draw if the defender firmly controls the
        // square in front of the frontmost pawn's path, and the square diagonally
        // behind this square on the file of the other pawn.
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
        return SCALE_FACTOR_NONE;
        
    default:
        // The pawns are not on the same file or adjacent files. No scaling.
        return SCALE_FACTOR_NONE;
    }
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
            || Board::relativeRank(strongSide, weakKing) <= RANK_6)) {
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
    // INSUFFICIENT MATERIAL (must check FIRST!)
    // ========================================
    
    // KNK: Lone knight vs lone king - DRAW (insufficient material)
    if (wN == 1 && wB == 0 && wR == 0 && wQ == 0 && wP == 0 && bTotal == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KNK, true};
    }
    if (bN == 1 && bB == 0 && bR == 0 && bQ == 0 && bP == 0 && wTotal == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KNK, true};
    }
    
    // KBK: Lone bishop vs lone king - DRAW (insufficient material)
    if (wB == 1 && wN == 0 && wR == 0 && wQ == 0 && wP == 0 && bTotal == 0) {
        return EndgameInfo{WHITE, BLACK, ENDGAME_KBK, true};
    }
    if (bB == 1 && bN == 0 && bR == 0 && bQ == 0 && bP == 0 && wTotal == 0) {
        return EndgameInfo{BLACK, WHITE, ENDGAME_KBK, true};
    }
    
    // KBBK with same-colored bishops - DRAW (cannot checkmate)
    if (wB == 2 && wN == 0 && wR == 0 && wQ == 0 && wP == 0 && bTotal == 0) {
        uint64_t bishopBB = board.bitboards[WHITE][BISHOP];
        bool hasDark = (bishopBB & DarkSquares) != 0;
        bool hasLight = (bishopBB & ~DarkSquares) != 0;
        if (!(hasDark && hasLight)) {  // Same color = draw
            return EndgameInfo{WHITE, BLACK, ENDGAME_KBKB, true};
        }
    }
    if (bB == 2 && bN == 0 && bR == 0 && bQ == 0 && bP == 0 && wTotal == 0) {
        uint64_t bishopBB = board.bitboards[BLACK][BISHOP];
        bool hasDark = (bishopBB & DarkSquares) != 0;
        bool hasLight = (bishopBB & ~DarkSquares) != 0;
        if (!(hasDark && hasLight)) {
            return EndgameInfo{BLACK, WHITE, ENDGAME_KBKB, true};
        }
    }
    
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
    // Only if not caught by insufficient material checks above
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
    // Insufficient material - all return draw
    case ENDGAME_KNK:
    case ENDGAME_KBK:
    case ENDGAME_KBKB:
        value = VALUE_DRAW;
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
