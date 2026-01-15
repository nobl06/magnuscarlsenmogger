#pragma once
#include "board.h"
#include "move.h"

class MoveGenerator {
public:
    MoveGenerator(Board& b, Color sideToMove) 
        : board(b), color(sideToMove) {}
    
    // Main move generation functions
    size_t generatePseudoLegalMoves(Move moves[220]) const;
    size_t filterLegalMoves(const Move pseudoLegalMoves[220], size_t pseudoLegalCount, Move legalMoves[220]);
    
    // Individual piece move generators
    // moves array and moveCount are passed by reference to be modified
    void generatePawnMoves(Move moves[220], size_t& moveCount, int from) const;
    void generateKnightMoves(Move moves[220], size_t& moveCount, int from) const;
    void generateBishopMoves(Move moves[220], size_t& moveCount, int from) const;
    void generateRookMoves  (Move moves[220], size_t& moveCount, int from) const;
    void generateQueenMoves (Move moves[220], size_t& moveCount, int from) const;
    void generateKingMoves  (Move moves[220], size_t& moveCount, int from) const;

private:
    Board& board;
    Color color;
};
