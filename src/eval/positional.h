#pragma once
#include "../board.h"
#include "defs.h"
#include <utility>
#include <cstdint>

namespace Positional {

using Score = Eval::Score;

// Final evaluation function returning (midgame, endgame) scores
std::pair<int, int> evaluatePositional(const Board& board);

// Sub-evaluation functions
std::pair<int, int> evaluatePawns(const Board& board);
std::pair<int, int> evaluateMobility(const Board& board);
std::pair<int, int> evaluateKingSafety(const Board& board);
std::pair<int, int> evaluatePieces(const Board& board);
std::pair<int, int> evaluateThreats(const Board& board);
std::pair<int, int> evaluateSpace(const Board& board);

// Helper functions for attack maps
uint64_t getKingZone(int kingSq, Color color);

// Pawn structure detection
uint64_t getPassedPawns(const Board& board, Color color);
uint64_t getIsolatedPawns(const Board& board, Color color);
uint64_t getDoubledPawns(const Board& board, Color color);
uint64_t getBackwardPawns(const Board& board, Color color);

// Winnable/Complexity adjustment
std::pair<int, int> applyWinnable(const Board& board, int mg, int eg);
int countPassedPawns(const Board& board);

}

