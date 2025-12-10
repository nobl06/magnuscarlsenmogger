#pragma once
#include "../board.h"

namespace Evaluation {

// Evaluation mode selection
enum class EvalMode {
    BASIC,      // Material + PSQT
    ADVANCED    // Full positional evaluation
};

// Set this to switch between evaluation modes
constexpr EvalMode EVAL_MODE = EvalMode::ADVANCED;

// Main evaluation functions
int evaluate(const Board& board);           // calls basic or advanced
int basicEvaluate(const Board& board);      // Material + PSQT
int advancedEvaluate(const Board& board);   // Full evaluation

// Helper functions
int calculateGamePhase(const Board& board);
int interpolate(int mgScore, int egScore, int phase);
}

