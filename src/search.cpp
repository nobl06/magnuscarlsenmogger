#include "search.h"
#include "eval/evaluate.h"
#include "gen.hpp"
#include "tt.h"
#include "zobrist.h"
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

// piece values for MVV-LVA
constexpr int pieceValues[7] = {
    0,   // EMPTY
    100, // PAWN
    300, // KNIGHT
    300, // BISHOP
    500, // ROOK
    900, // QUEEN
    0    // KING
};

int scoreMove(const Move &move, const Board &board) {
    PieceType victim = board.pieceAt(move.to);
    PieceType attacker = board.pieceAt(move.from);

    // captures score
    if (victim != PieceType::EMPTY) {
        return 10000 + 10 * pieceValues[victim] - pieceValues[attacker];
    }

    // promotions score
    if (move.promotion != PieceType::EMPTY) {
        return 9000 + pieceValues[move.promotion];
    }

    // quiet move
    return 0;
}

int getMateScore(int ply) {
    return -MATE_SCORE + ply;
}

// alpha-beta search with TT integration
int alphaBeta(Board &board, int depth, int alpha, int beta, int ply, Move* bestMoveOut) {
    if (out_of_time()) return alpha;
    
    int originalAlpha = alpha;
    uint64_t hashKey = board.hashKey;
    
    // Probe transposition table
    TT::TTEntry* ttEntry = TT::tt.probe(hashKey);
    Move ttMove;
    
    if (ttEntry != nullptr && ttEntry->depth >= depth) {
        // TT hit with sufficient depth
        ttMove = ttEntry->bestMove;
        
        if (ttEntry->type == TT::EXACT) {
            if (bestMoveOut) *bestMoveOut = ttMove;
            return ttEntry->value;
        } else if (ttEntry->type == TT::LOWERBOUND) {
            if (ttEntry->value >= beta) {
                if (bestMoveOut) *bestMoveOut = ttMove;
                return ttEntry->value;
            }
            alpha = std::max(alpha, (int)ttEntry->value);
        } else if (ttEntry->type == TT::UPPERBOUND) {
            if (ttEntry->value <= alpha) {
                if (bestMoveOut) *bestMoveOut = ttMove;
                return ttEntry->value;
            }
            beta = std::min(beta, (int)ttEntry->value);
        }
    } else if (ttEntry != nullptr) {
        // TT hit but insufficient depth - still use move for ordering
        ttMove = ttEntry->bestMove;
    }
    
    stats.nodes++;
    
    // Check for threefold repetition (draw)
    if (board.isThreefoldRepetition()) {
        return 0;
    }
    
    // terminal node - return evaluation
    if (depth == 0) {
        return Evaluation::evaluate(board);
    }
    
    // generate moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);
    
    // sort moves with score (MVV-LVA ordering + TT move priority)
    for (Move &move : legalMoves) {
        // Give TT move highest priority
        if (ttMove.from != 0 && move.from == ttMove.from && move.to == ttMove.to && 
            move.promotion == ttMove.promotion) {
            move.score = 1000000;  // Highest priority
        } else {
            move.score = scoreMove(move, board);
        }
    }
    std::sort(legalMoves.begin(), legalMoves.end(),
              [](const Move &a, const Move &b) { return a.score > b.score; });
    
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
    Move bestMove = legalMoves[0];
    
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
            bestMove = move;
        }
        
        // beta cutoff - check first for efficiency
        // If score >= beta, opponent won't allow this line
        if (score >= beta) {
            // Store in TT as LOWERBOUND (beta cutoff)
            TT::tt.store(hashKey, score, depth, TT::LOWERBOUND, bestMove);
            if (bestMoveOut) *bestMoveOut = bestMove;
            return beta; // fail-high cutoff
        }
        
        // update alpha - only if we didn't cut off
        if (score > alpha) {
            alpha = score;
        }
    }
    
    // Determine node type and store in TT
    TT::NodeType nodeType;
    if (bestScore <= originalAlpha) {
        nodeType = TT::UPPERBOUND;  // All moves failed low
    } else if (bestScore >= beta) {
        nodeType = TT::LOWERBOUND;  // Beta cutoff (shouldn't reach here)
    } else {
        nodeType = TT::EXACT;  // PV node - exact score
    }
    
    TT::tt.store(hashKey, bestScore, depth, nodeType, bestMove);
    
    if (bestMoveOut) *bestMoveOut = bestMove;
    return bestScore;
}

// main search function with TT integration
Move findBestMove(Board &board, int depth) {
    stats.reset();
    info.reset();
    info.maxDepth = depth;
    
    start_time = std::chrono::steady_clock::now();
    time_limit_ms = 9500; // 9 seconds
    
    // generate root moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);
    
    // Check TT for move ordering
    uint64_t hashKey = board.hashKey;
    TT::TTEntry* ttEntry = TT::tt.probe(hashKey);
    
    // sort moves with score (MVV-LVA ordering + TT move priority)
    for (Move &move : legalMoves) {
        // Give TT move highest priority
        if (ttEntry && ttEntry->bestMove.from != 0 && 
            move.from == ttEntry->bestMove.from && 
            move.to == ttEntry->bestMove.to &&
            move.promotion == ttEntry->bestMove.promotion) {
            move.score = 1000000;  // Highest priority
        } else {
            move.score = scoreMove(move, board);
        }
    }
    std::sort(legalMoves.begin(), legalMoves.end(),
              [](const Move &a, const Move &b) { return a.score > b.score; });
    
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
            Move childBestMove;
            int score = -alphaBeta(board, currentDepth - 1, -beta, -alpha, 1, &childBestMove);
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
        
        // Store root position in TT
        TT::tt.store(hashKey, bestScore, currentDepth, TT::EXACT, bestMove);
    }
    
    return bestMove;
}
} // namespace Search

//