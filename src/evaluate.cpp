#include "evaluate.h"
#include "piece_values.h"
#include <cstdint>

int evaluate(const Board &b, Color stm) {
    int score = 0;

    for (int color = WHITE; color <= BLACK; ++color) {
        int sign = (color == WHITE ? +1 : -1);

        for (int piece = PAWN; piece <= KING; ++piece) {
            uint64_t bb = b.bitboards[color][piece];

            int count = 0;
            while (bb) {
                bb &= bb - 1;
                ++count;
            }

            score += sign * count * Eval::mg_value[piece];
        }
    }

    // Return from stm perspective
    return (stm == WHITE ? score : -score);
}