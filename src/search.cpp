#include "search.h"
#include "eval/evaluate.h"
#include "gen.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

std::chrono::steady_clock::time_point start_time; // Initialize timer
int time_limit_ms = 9000;                         // 9 seconds

inline bool out_of_time() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time)
               .count() >= time_limit_ms;
}

namespace Search {
// global stats
Stats stats;
Info info;

int getMateScore(int ply) {
    return -MATE_SCORE + ply;
}

// alpha-beta search
int alphaBeta(Board &board, int depth, int alpha, int beta, int ply) {
    if (out_of_time()) return alpha; // We return immediately when out of time
    stats.nodes++;

    // terminal node -  return evaluation
    if (depth == 0) {
        return Evaluation::evaluate(board);
    }

    // generate moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);

    // check for checkmate/stalemate
    if (legalMoves.empty()) {
        if (board.isKingInCheck(board.sideToMove)) {
            // checkmate - return mate score adjusted by ply
            return getMateScore(ply);
        } else {
            // stalemate
            return 0;
        }
    }

    // alpha-beta loop
    int bestScore = -INFINITY_SCORE;

    for (const Move &move : legalMoves) {
        if (out_of_time()) break;

        // make/unmake method (efficient - no board copying)
        BoardState state = board.makeMove(move);

        // recursive call with negated window
        int score = -alphaBeta(board, depth - 1, -beta, -alpha, ply + 1);

        // unmake the move to restore board state
        board.unmakeMove(move, state);

        if (out_of_time()) break;

        // if this move is better than any seen so far
        // update best score
        if (score > bestScore) {
            bestScore = score;
        }

        // beta cutoff - check first for efficiency
        // If score >= beta, opponent won't allow this line
        if (score >= beta) {
            return beta; // fail-high cutoff
        }

        // update alpha - only if we didn't cut off
        if (score > alpha) {
            alpha = score;
        }
    }

    return bestScore;
}

// main search function
Move findBestMove(Board &board, int depth) {
    stats.reset();
    info.reset();
    info.maxDepth = depth;

    start_time = std::chrono::steady_clock::now();
    time_limit_ms = 9000; // 9 seconds

    // generate root moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);

    if (legalMoves.empty()) {
        return Move(); // no legal moves (checkmate or stalemate), return empty move
    }

    Move bestMove = legalMoves[0];
    int bestScore = -INFINITY_SCORE;

    // Iterative deepening
    for (int currentDepth = 1; currentDepth <= depth; currentDepth++) {
        auto layer_start = std::chrono::steady_clock::now(); // Timing a depth

        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;

        Move bestMoveThisIter = legalMoves[0];
        int bestScoreThisIter = -INFINITY_SCORE;

        for (const Move &move : legalMoves) {
            if (out_of_time()) break;

            BoardState state = board.makeMove(move);
            int score = -alphaBeta(board, currentDepth - 1, -beta, -alpha, 1);
            board.unmakeMove(move, state);

            if (out_of_time()) break;

            if (score > bestScoreThisIter) {
                bestScoreThisIter = score;
                bestMoveThisIter = move;
            }

            if (score > alpha) {
                alpha = score;
            }
        }

        auto layer_end = std::chrono::steady_clock::now();
        auto layer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            layer_end - layer_start)
                            .count();

        std::cout << "Depth " << currentDepth
                  << " took " << layer_ms << " ms\n";

        if (out_of_time()) break;

        // Depth completed, update global best move
        bestMove = bestMoveThisIter;
        bestScore = bestScoreThisIter;
        stats.depthReached = currentDepth;
    }

    return bestMove;
}
} // namespace Search

//