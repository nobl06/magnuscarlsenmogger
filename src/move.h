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
    
    // Check if this is a null move
    bool isNull() const {
        return from == 65 && to == 65;
    }
    
    // Create a null move (special marker)
    static Move null() {
        Move m;
        m.from = 65;
        m.to = 65;
        return m;
    }
};

Move parseMove(const std::string &s);