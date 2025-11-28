#pragma once
#include <vector>
#include "board.h"
#include "move.h"

class MoveGenerator {
public:
    MoveGenerator(const Board& b, Color sideToMove) 
        : board(b), color(sideToMove) {}
    
    std::vector<Move> generatePseudoLegalMoves() const;
    
    void generatePawnMoves(std::vector<Move>& moves, int from) const;
    
private:
    const Board& board;
    Color color;
};