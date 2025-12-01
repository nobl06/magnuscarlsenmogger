#include "psqt.h"
#include <algorithm>
#include <utility>

namespace PSQT {

// Bonus tables contain positional bonuses for each piece type
// Scores are explicit for columns A to D,  mirrored for E to H
// Format: [row][column] where row 0 = row 1, row 7 = row 8

static constexpr Score KnightBonus[8][4] = {
    { Score(-175, -96), Score(-92,-65), Score(-74,-49), Score(-73,-21) },
    { Score( -77, -67), Score(-41,-54), Score(-27,-18), Score(-15,  8) },
    { Score( -61, -40), Score(-17,-27), Score(  6, -8), Score( 12, 29) },
    { Score( -35, -35), Score(  8, -2), Score( 40, 13), Score( 49, 28) },
    { Score( -34, -45), Score( 13,-16), Score( 44,  9), Score( 51, 39) },
    { Score(  -9, -51), Score( 22,-44), Score( 58,-16), Score( 53, 17) },
    { Score( -67, -69), Score(-27,-50), Score(  4,-51), Score( 37, 12) },
    { Score(-201,-100), Score(-83,-88), Score(-56,-56), Score(-26,-17) }
};

static constexpr Score BishopBonus[8][4] = {
    { Score(-37,-40), Score(-4 ,-21), Score( -6,-26), Score(-16, -8) },
    { Score(-11,-26), Score(  6, -9), Score( 13,-12), Score(  3,  1) },
    { Score(-5 ,-11), Score( 15, -1), Score( -4, -1), Score( 12,  7) },
    { Score(-4 ,-14), Score(  8, -4), Score( 18,  0), Score( 27, 12) },
    { Score(-8 ,-12), Score( 20, -1), Score( 15,-10), Score( 22, 11) },
    { Score(-11,-21), Score(  4,  4), Score(  1,  3), Score(  8,  4) },
    { Score(-12,-22), Score(-10,-14), Score(  4, -1), Score(  0,  1) },
    { Score(-34,-32), Score(  1,-29), Score(-10,-26), Score(-16,-17) }
};

static constexpr Score RookBonus[8][4] = {
    { Score(-31, -9), Score(-20,-13), Score(-14,-10), Score(-5, -9) },
    { Score(-21,-12), Score(-13, -9), Score( -8, -1), Score( 6, -2) },
    { Score(-25,  6), Score(-11, -8), Score( -1, -2), Score( 3, -6) },
    { Score(-13, -6), Score( -5,  1), Score( -4, -9), Score(-6,  7) },
    { Score(-27, -5), Score(-15,  8), Score( -4,  7), Score( 3, -6) },
    { Score(-22,  6), Score( -2,  1), Score(  6, -7), Score(12, 10) },
    { Score( -2,  4), Score( 12,  5), Score( 16, 20), Score(18, -5) },
    { Score(-17, 18), Score(-19,  0), Score( -1, 19), Score( 9, 13) }
};

static constexpr Score QueenBonus[8][4] = {
    { Score( 3,-69), Score(-5,-57), Score(-5,-47), Score( 4,-26) },
    { Score(-3,-54), Score( 5,-31), Score( 8,-22), Score(12, -4) },
    { Score(-3,-39), Score( 6,-18), Score(13, -9), Score( 7,  3) },
    { Score( 4,-23), Score( 5, -3), Score( 9, 13), Score( 8, 24) },
    { Score( 0,-29), Score(14, -6), Score(12,  9), Score( 5, 21) },
    { Score(-4,-38), Score(10,-18), Score( 6,-11), Score( 8,  1) },
    { Score(-5,-50), Score( 6,-27), Score(10,-24), Score( 8, -8) },
    { Score(-2,-74), Score(-2,-52), Score( 1,-43), Score(-2,-34) }
};

static constexpr Score KingBonus[8][4] = {
    { Score(271,  1), Score(327, 45), Score(271, 85), Score(198, 76) },
    { Score(278, 53), Score(303,100), Score(234,133), Score(179,135) },
    { Score(195, 88), Score(258,130), Score(169,169), Score(120,175) },
    { Score(164,103), Score(190,156), Score(138,172), Score( 98,172) },
    { Score(154, 96), Score(179,166), Score(105,199), Score( 70,199) },
    { Score(123, 92), Score(145,172), Score( 81,184), Score( 31,191) },
    { Score( 88, 47), Score(120,121), Score( 65,116), Score( 33,131) },
    { Score( 59, 11), Score( 89, 59), Score( 45, 73), Score( -1, 78) }
};

// Pawn bonuses (asymmetric - includes all columns)
static constexpr Score PawnBonus[8][8] = {
    { Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) },
    { Score(  2, -8), Score(  4, -6), Score( 11,  9), Score( 18,  5), Score( 16, 16), Score( 21,  6), Score(  9, -6), Score( -3,-18) },
    { Score( -9, -9), Score(-15, -7), Score( 11,-10), Score( 15,  5), Score( 31,  2), Score( 23,  3), Score(  6, -8), Score(-20, -5) },
    { Score( -3,  7), Score(-20,  1), Score(  8, -8), Score( 19, -2), Score( 39,-14), Score( 17,-13), Score(  2,-11), Score( -5, -6) },
    { Score( 11, 12), Score( -4,  6), Score(-11,  2), Score(  2, -6), Score( 11, -5), Score(  0, -4), Score(-12, 14), Score(  5,  9) },
    { Score(  3, 27), Score(-11, 18), Score( -6, 19), Score( 22, 29), Score( -8, 30), Score( -5,  9), Score(-14,  8), Score(-11, 14) },
    { Score( -7, -1), Score(  6,-14), Score( -2, 13), Score(-11, 22), Score(  4, 24), Score(-14, 17), Score( 10,  7), Score( -9,  7) },
    { Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0), Score(0, 0) }
};

// get column distance from edge to help mirroring
static constexpr int columnDistance(int column) {
    return std::min(column, 7 - column);
}

// flip row for black pieces
static constexpr int flipRow(int square) {
    int column = square % 8;
    int row = square / 8;
    return (7 - row) * 8 + column;
}

// Create a piece square table to store the positional bonuses for each piece type and square
Score psqTable[7][2][64];

// Initialize the piece square table
void init() {
    // first initialize all the scores to zero
    for (int piece = 0; piece < 7; ++piece) {
        for (int color = 0; color < 2; ++color) {
            for (int square = 0; square < 64; ++square) {
                psqTable[piece][color][square] = Score();
            }
        }
    }
    
    // For each piece type and square, store the positional bonus
    for (int square = 0; square < 64; ++square) {
        int column = square % 8;
        int row = square / 8;
        int mirroredColumn = columnDistance(column);
        
        // store the positional bonus for each piece type
        Score pawnBonus = PawnBonus[row][column];
        psqTable[static_cast<int>(PieceType::PAWN)][static_cast<int>(Color::WHITE)][square] = pawnBonus;
        psqTable[static_cast<int>(PieceType::PAWN)][static_cast<int>(Color::BLACK)][flipRow(square)] = Score(-pawnBonus.mg, -pawnBonus.eg);
        
        Score knightBonus = KnightBonus[row][mirroredColumn];
        psqTable[static_cast<int>(PieceType::KNIGHT)][static_cast<int>(Color::WHITE)][square] = knightBonus;
        psqTable[static_cast<int>(PieceType::KNIGHT)][static_cast<int>(Color::BLACK)][flipRow(square)] = Score(-knightBonus.mg, -knightBonus.eg);
        
        Score bishopBonus = BishopBonus[row][mirroredColumn];
        psqTable[static_cast<int>(PieceType::BISHOP)][static_cast<int>(Color::WHITE)][square] = bishopBonus;
        psqTable[static_cast<int>(PieceType::BISHOP)][static_cast<int>(Color::BLACK)][flipRow(square)] = Score(-bishopBonus.mg, -bishopBonus.eg);
        
        Score rookBonus = RookBonus[row][mirroredColumn];
        psqTable[static_cast<int>(PieceType::ROOK)][static_cast<int>(Color::WHITE)][square] = rookBonus;
        psqTable[static_cast<int>(PieceType::ROOK)][static_cast<int>(Color::BLACK)][flipRow(square)] = Score(-rookBonus.mg, -rookBonus.eg);
        
        Score queenBonus = QueenBonus[row][mirroredColumn];
        psqTable[static_cast<int>(PieceType::QUEEN)][static_cast<int>(Color::WHITE)][square] = queenBonus;
        psqTable[static_cast<int>(PieceType::QUEEN)][static_cast<int>(Color::BLACK)][flipRow(square)] = Score(-queenBonus.mg, -queenBonus.eg);
        
        Score kingBonus = KingBonus[row][mirroredColumn];
        psqTable[static_cast<int>(PieceType::KING)][static_cast<int>(Color::WHITE)][square] = kingBonus;
        psqTable[static_cast<int>(PieceType::KING)][static_cast<int>(Color::BLACK)][flipRow(square)] = Score(-kingBonus.mg, -kingBonus.eg);
    }
}

// Helper function to process a bitboard of pieces and accumulate their scores
static void processPieces(uint64_t bitboard, PieceType pieceType, Color color,
                         int& mgScore, int& egScore) {
    uint64_t pieces = bitboard;
    while (pieces) {
        int square = Board::popLsb(pieces);
        Score bonus = getScore(pieceType, color, square);
        
        // Add to score: we add the score for white and subtract the score for black
        if (color == Color::WHITE) {
            mgScore += bonus.mg;
            egScore += bonus.eg;
        } else {
            mgScore -= bonus.mg;
            egScore -= bonus.eg;
        }
    }
}

// Evaluate the psqt score for the current position of all pieces
std::pair<int, int> evaluatePSQT(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    
    // Process all white pieces
    processPieces(board.whitePawns, PieceType::PAWN, Color::WHITE, mgScore, egScore);
    processPieces(board.whiteKnights, PieceType::KNIGHT, Color::WHITE, mgScore, egScore);
    processPieces(board.whiteBishops, PieceType::BISHOP, Color::WHITE, mgScore, egScore);
    processPieces(board.whiteRooks, PieceType::ROOK, Color::WHITE, mgScore, egScore);
    processPieces(board.whiteQueens, PieceType::QUEEN, Color::WHITE, mgScore, egScore);
    processPieces(board.whiteKing, PieceType::KING, Color::WHITE, mgScore, egScore);
    
    // Process all black pieces
    processPieces(board.blackPawns, PieceType::PAWN, Color::BLACK, mgScore, egScore);
    processPieces(board.blackKnights, PieceType::KNIGHT, Color::BLACK, mgScore, egScore);
    processPieces(board.blackBishops, PieceType::BISHOP, Color::BLACK, mgScore, egScore);
    processPieces(board.blackRooks, PieceType::ROOK, Color::BLACK, mgScore, egScore);
    processPieces(board.blackQueens, PieceType::QUEEN, Color::BLACK, mgScore, egScore);
    processPieces(board.blackKing, PieceType::KING, Color::BLACK, mgScore, egScore);
    
    return {mgScore, egScore};
}

}

