#pragma once
#include "board.h"
#include <cstdint>
#include <string>

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    PieceType promotion = PieceType::EMPTY;
    int score = 0; 

    Move() = default;

    Move(uint8_t f, uint8_t t, PieceType p = PieceType::EMPTY)
        : from(f), to(t), promotion(p), score(0) {}

    std::string toString() const;
};

Move parseMove(const std::string &s);