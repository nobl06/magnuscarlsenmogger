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
int alphaBeta(Board &board, int depth, int alpha, int beta, int ply, Move* bestMoveOut = nullptr);

// Helper function
int getMateScore(int ply);

// Global statistics
extern Stats stats;
extern Info info;
} // namespace Search