#pragma once
#include "../board.h"
#include <utility>

namespace Material {


// Returns pair of (midgame_score, endgame_score) from white's perspective for material balance
std::pair<int, int> evaluateMaterial(const Board& board);

}

