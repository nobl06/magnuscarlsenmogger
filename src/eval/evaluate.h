#pragma once
#include "../board.h"

namespace Evaluation {
int evaluate(const Board& board);
int calculateGamePhase(const Board& board);
int interpolate(int mgScore, int egScore, int phase);
}

