#include "piece_values.h"

namespace Eval {

const int mg_value[7] = {
    0,   // EMPTY
    100, // PAWN
    320, // KNIGHT
    330, // BISHOP
    500, // ROOK
    900, // QUEEN
    0    // KING
};

}