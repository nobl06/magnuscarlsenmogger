#pragma once
#include <vector>
#include "board.h"
#include "move.h"

class MoveGenerator {
public:
    MoveGenerator(Board& b, Color sideToMove) 
        : board(b), color(sideToMove) {}
    
    // Main move generation functions
    std::vector<Move> generatePseudoLegalMoves() const;
    std::vector<Move> filterLegalMoves(const std::vector<Move>& pseudoLegalMoves);
    
    // Individual piece move generators
    void generatePawnMoves(std::vector<Move>& moves, int from) const;
    void generateKnightMoves(std::vector<Move>& moves, int from) const;
    void generateBishopMoves(std::vector<Move>& moves, int from) const;
    void generateRookMoves  (std::vector<Move>& moves, int from) const;
    void generateQueenMoves (std::vector<Move>& moves, int from) const;
    void generateKingMoves  (std::vector<Move>& moves, int from) const;

private:
    Board& board;
    Color color;
};
