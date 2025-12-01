#include "material.h"
#include "defs.h"

namespace Material {

// Use shared types and constants from defs.h
using namespace Eval;

// Material imbalance tables
// Order: [bishop_pair, pawn, knight, bishop, rook, queen]
// QuadraticOurs[piece1][piece2] - bonus for having piece1 and piece2 together
static constexpr Score QUADRATIC_OURS[6][6] = {
    // Bishop pair
    { Score(1419, 1455), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Pawn
    { Score(101, 28), Score(37, 39), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Knight
    { Score(57, 64), Score(249, 187), Score(-49, -62), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Bishop
    { Score(0, 0), Score(118, 137), Score(10, 27), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Rook
    { Score(-63, -68), Score(-5, 3), Score(100, 81), Score(132, 118), Score(-246, -244), Score(0, 0) },
    // Queen
    { Score(-210, -211), Score(37, 14), Score(147, 141), Score(161, 105), Score(-158, -174), Score(-9, -31) }
};

// QuadraticTheirs[piece1][piece2] - bonus/ penalty based on my piece(piece 1) and opponent's piece(piece 2)
static constexpr Score QUADRATIC_THEIRS[6][6] = {
    // Bishop pair
    { Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Pawn
    { Score(33, 30), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Knight
    { Score(46, 18), Score(106, 84), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Bishop
    { Score(75, 35), Score(59, 44), Score(60, 15), Score(0, 0), Score(0, 0), Score(0, 0) },
    // Rook
    { Score(26, 35), Score(6, 22), Score(38, 39), Score(-12, -2), Score(0, 0), Score(0, 0) },
    // Queen
    { Score(97, 93), Score(100, 163), Score(-58, -91), Score(112, 192), Score(276, 225), Score(0, 0) }
};

// count pieces on a bitboard
static int countPieces(uint64_t bb) {
    int count = 0;
    while (bb) {
        bb &= bb - 1;
        count++;
    }
    return count;
}

// Calculate material imbalance using Stockfish's formula
static std::pair<int, int> calculateImbalance(const Board& board, Color us) {
    Color them = (us == WHITE) ? BLACK : WHITE;
    
    // Count pieces for both sides based on index order: [bishop_pair, pawn, knight, bishop, rook, queen]
    int pieceCount[2][6] = {};
    
    // Bishop pair
    pieceCount[us][0] = (countPieces(board.bitboards[us][BISHOP]) > 1) ? 1 : 0;
    pieceCount[them][0] = (countPieces(board.bitboards[them][BISHOP]) > 1) ? 1 : 0;
    
    // Regular pieces
    pieceCount[us][1] = countPieces(board.bitboards[us][PAWN]);
    pieceCount[us][2] = countPieces(board.bitboards[us][KNIGHT]);
    pieceCount[us][3] = countPieces(board.bitboards[us][BISHOP]);
    pieceCount[us][4] = countPieces(board.bitboards[us][ROOK]);
    pieceCount[us][5] = countPieces(board.bitboards[us][QUEEN]);
    
    pieceCount[them][1] = countPieces(board.bitboards[them][PAWN]);
    pieceCount[them][2] = countPieces(board.bitboards[them][KNIGHT]);
    pieceCount[them][3] = countPieces(board.bitboards[them][BISHOP]);
    pieceCount[them][4] = countPieces(board.bitboards[them][ROOK]);
    pieceCount[them][5] = countPieces(board.bitboards[them][QUEEN]);
    
    int mgBonus = 0;
    int egBonus = 0;
    
    // Material imbalance formula
    for (int pt1 = 0; pt1 < 6; ++pt1) {
        if (!pieceCount[us][pt1])
            continue;
            
        int vMg = QUADRATIC_OURS[pt1][pt1].mg * pieceCount[us][pt1];
        int vEg = QUADRATIC_OURS[pt1][pt1].eg * pieceCount[us][pt1];
        
        for (int pt2 = 0; pt2 < pt1; ++pt2) {
            vMg += QUADRATIC_OURS[pt1][pt2].mg * pieceCount[us][pt2]
                 + QUADRATIC_THEIRS[pt1][pt2].mg * pieceCount[them][pt2];
            vEg += QUADRATIC_OURS[pt1][pt2].eg * pieceCount[us][pt2]
                 + QUADRATIC_THEIRS[pt1][pt2].eg * pieceCount[them][pt2];
        }
        
        mgBonus += pieceCount[us][pt1] * vMg;
        egBonus += pieceCount[us][pt1] * vEg;
    }
    
    return {mgBonus / 16, egBonus / 16};
}

// Function that calculates the material score for the current board position
std::pair<int, int> evaluateMaterial(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    
    // Count pieces and calculate base material
    int whitePawns   = countPieces(board.bitboards[WHITE][PAWN]);
    int whiteKnights = countPieces(board.bitboards[WHITE][KNIGHT]);
    int whiteBishops = countPieces(board.bitboards[WHITE][BISHOP]);
    int whiteRooks   = countPieces(board.bitboards[WHITE][ROOK]);
    int whiteQueens  = countPieces(board.bitboards[WHITE][QUEEN]);
    
    int blackPawns   = countPieces(board.bitboards[BLACK][PAWN]);
    int blackKnights = countPieces(board.bitboards[BLACK][KNIGHT]);
    int blackBishops = countPieces(board.bitboards[BLACK][BISHOP]);
    int blackRooks   = countPieces(board.bitboards[BLACK][ROOK]);
    int blackQueens  = countPieces(board.bitboards[BLACK][QUEEN]);
    
    // Base material values
    mgScore += whitePawns   * PAWN_VALUE_MG;
    mgScore += whiteKnights * KNIGHT_VALUE_MG;
    mgScore += whiteBishops * BISHOP_VALUE_MG;
    mgScore += whiteRooks   * ROOK_VALUE_MG;
    mgScore += whiteQueens  * QUEEN_VALUE_MG;
    
    mgScore -= blackPawns   * PAWN_VALUE_MG;
    mgScore -= blackKnights * KNIGHT_VALUE_MG;
    mgScore -= blackBishops * BISHOP_VALUE_MG;
    mgScore -= blackRooks   * ROOK_VALUE_MG;
    mgScore -= blackQueens  * QUEEN_VALUE_MG;
    
    egScore += whitePawns   * PAWN_VALUE_EG;
    egScore += whiteKnights * KNIGHT_VALUE_EG;
    egScore += whiteBishops * BISHOP_VALUE_EG;
    egScore += whiteRooks   * ROOK_VALUE_EG;
    egScore += whiteQueens  * QUEEN_VALUE_EG;
    
    egScore -= blackPawns   * PAWN_VALUE_EG;
    egScore -= blackKnights * KNIGHT_VALUE_EG;
    egScore -= blackBishops * BISHOP_VALUE_EG;
    egScore -= blackRooks   * ROOK_VALUE_EG;
    egScore -= blackQueens  * QUEEN_VALUE_EG;
    
    // Calculate material imbalance bonuses
    auto whiteImbalance = calculateImbalance(board, WHITE);
    auto blackImbalance = calculateImbalance(board, BLACK);
    
    mgScore += whiteImbalance.first - blackImbalance.first;
    egScore += whiteImbalance.second - blackImbalance.second;
    
    return {mgScore, egScore};
}

}

