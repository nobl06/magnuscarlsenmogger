#include "search.h"
#include "eval/evaluate.h"
#include "gen.hpp"
#include <chrono>

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
        // make/unmake method (efficient - no board copying)
        BoardState state = board.makeMove(move);

        // recursive call with negated window
        int score = -alphaBeta(board, depth - 1, -beta, -alpha, ply + 1);

        // unmake the move to restore board state
        board.unmakeMove(move, state);

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

    // generate root moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);

    if (legalMoves.empty()) {
        return Move(); // no legal moves (checkmate or stalemate), return empty move
    }

    Move bestMove = legalMoves[0];
    int bestScore = -INFINITY_SCORE;
    int alpha = -INFINITY_SCORE;
    int beta = INFINITY_SCORE;

    // search each root move
    for (const Move &move : legalMoves) {
        // make/unmake method (efficient - no board copying)
        BoardState state = board.makeMove(move);

        int score = -alphaBeta(board, depth - 1, -beta, -alpha, 1);

        // unmake the move to restore board state
        board.unmakeMove(move, state);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        if (score > alpha) {
            alpha = score;
        }
    }

    stats.depthReached = depth;
    return bestMove;
}
} // namespace Search

//