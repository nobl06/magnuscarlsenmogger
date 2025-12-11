#pragma once
#include "board.h"
#include "move.h"
#include "tt.h"
#include <cstdint>

namespace Search {
// Constants
// we have chosen 3200 as the mate_score since its high enough to clearly
// be better than any positional evaluation
constexpr int MATE_SCORE = 32000;
// we use 32767 since its the maximum value for 16-bit signed integer
// thus it is arge enough to be bigger than any real evaluation
constexpr int INFINITY_SCORE = 32767;
// enough to search through 32 full moves
constexpr int MAX_PLY = 64;

// search stats
struct Stats {
    // number of nodes searched
    uint64_t nodes;
    // depth reached
    int depthReached;

    void reset() {
        nodes = 0;
        depthReached = 0;
    }
};

// search configuration
struct Info {
    // maximum depth to search
    int maxDepth;
    // maximum number of nodes to search
    uint64_t maxNodes;
    bool stopped;

    void reset() {
        maxDepth = 0;
        maxNodes = UINT64_MAX;
        stopped = false;
    }
};

// Main search entry point
Move findBestMove(Board &board, int depth);

// Internal alpha-beta function
int alphaBeta(Board &board, int depth, int alpha, int beta, int ply, bool pvNode, Move* pv, Move* bestMoveOut = nullptr, bool isNullMove = false);

// Quiescence search - searches captures until position is quiet
int quiescence(Board &board, int alpha, int beta, int ply);

// Helper function
int getMateScore(int ply);

// Killer moves: stores 2 quiet moves per ply that caused beta cutoffs
struct KillerMoves {
    Move moves[2];
    
    // Helper to compare moves
    static bool sameMove(const Move& a, const Move& b) {
        return (a.from == b.from && a.to == b.to && a.promotion == b.promotion);
    }
    
    void add(const Move& m) {
        // Don't add if already first killer
        if (sameMove(moves[0], m)) return;
        // Shift: move[0] becomes move[1], new move becomes move[0]
        moves[1] = moves[0];
        moves[0] = m;
    }
    
    void clear() {
        moves[0] = Move();
        moves[1] = Move();
    }
    
    bool isKiller(const Move& m) const {
        return (sameMove(m, moves[0]) || sameMove(m, moves[1]));
    }
};

// History heuristic: [from][to] -> score
// Tracks how often a move causes a beta cutoff
constexpr int HISTORY_MAX = 10000;  // Cap to prevent overflow
extern KillerMoves killers[MAX_PLY];
extern int history[64][64];

// Late Move Reduction
constexpr int LMR_TABLE_SIZE = 64;
extern int reductionTable[LMR_TABLE_SIZE][LMR_TABLE_SIZE];
void initReductions();

// Move ordering function
int scoreMove(const Move& move, const Board& board, int ply);

// Global statistics
extern Stats stats;
extern Info info;
} // namespace Search