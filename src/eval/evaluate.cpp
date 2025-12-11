#include "evaluate.h"
#include "material.h"
#include "positional.h"
#include "psqt.h"
#include <algorithm>

namespace Evaluation {

// constants that determine how much each piece type contributes to the game phase.
constexpr int KNIGHT_PHASE = 1;
constexpr int BISHOP_PHASE = 1;
constexpr int ROOK_PHASE = 2;
constexpr int QUEEN_PHASE = 4;

// Maximum phase value (opening position with all pieces, both teams included)
constexpr int MAX_PHASE = KNIGHT_PHASE * 4 + BISHOP_PHASE * 4 +
                          ROOK_PHASE * 4 + QUEEN_PHASE * 2;

int calculateGamePhase(const Board &board) {
    int phase = 0;

    // Count pieces for both teams
    for (int color = WHITE; color <= BLACK; color++) {
        int knights = Board::popcount(board.bitboards[color][KNIGHT]);
        int bishops = Board::popcount(board.bitboards[color][BISHOP]);
        int rooks = Board::popcount(board.bitboards[color][ROOK]);
        int queens = Board::popcount(board.bitboards[color][QUEEN]);

        phase += knights * KNIGHT_PHASE;
        phase += bishops * BISHOP_PHASE;
        phase += rooks * ROOK_PHASE;
        phase += queens * QUEEN_PHASE;
    }

    // in case of promotions
    return std::min(phase, MAX_PHASE);
}

int interpolate(int mgScore, int egScore, int phase) {
    // Stockfish formula for tapered evaluation
    return (mgScore * phase + egScore * (MAX_PHASE - phase)) / MAX_PHASE;
}

// BASIC EVALUATION
// Material + Piece-Square Tables
int basicEvaluate(const Board &board) {
    auto [materialMG, materialEG] = Material::evaluateMaterial(board);
    auto [psqtMG, psqtEG] = PSQT::evaluatePSQT(board);

    int mgScore = materialMG + psqtMG;
    int egScore = materialEG + psqtEG;

    // Calculate game phase
    int phase = calculateGamePhase(board);

    // Interpolate between midgame and endgame
    int score = interpolate(mgScore, egScore, phase);

    // Add tempo bonus
    constexpr int TEMPO_BONUS = 28;
    score += TEMPO_BONUS;

    // Return from side to move perspective
    return (board.sideToMove == WHITE) ? score : -score;
}

// ADVANCED EVALUATION
// Full positional evaluation
int advancedEvaluate(const Board &board) {
    // Sum up all evaluation components
    auto [materialMG, materialEG] = Material::evaluateMaterial(board);
    auto [psqtMG, psqtEG] = PSQT::evaluatePSQT(board);
    auto [positionalMG, positionalEG] = Positional::evaluatePositional(board);

    int mgScore = materialMG + psqtMG + positionalMG;
    int egScore = materialEG + psqtEG + positionalEG;

    // Adjust the endgame score based on position complexity
    auto [mgFinal, egFinal] = Positional::applyWinnable(board, mgScore, egScore);

    // calculate game phase
    int phase = calculateGamePhase(board);

    // Interpolate between midgame and endgame scores
    int score = interpolate(mgFinal, egFinal, phase);

    // Add tempo bonus for white starting position
    constexpr int TEMPO_BONUS = 28;
    score += TEMPO_BONUS;

    // Return score from the perspective of the side to move
    // If it's white's turn, return (positive = good for white)
    // If it's black's turn, return negative (positive = good for black)
    return (board.sideToMove == WHITE) ? score : -score;
}

// Choose between basic and advanced modes
int evaluate(const Board &board) {
    if constexpr (EVAL_MODE == EvalMode::BASIC) {
        return basicEvaluate(board);
    } else {
        return advancedEvaluate(board);
    }
}

} // namespace Evaluation
