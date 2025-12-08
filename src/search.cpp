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

// Killer moves and history tables
KillerMoves killers[MAX_PLY];
int history[64][64] = {0};

// Search path tracking for 2-fold repetition detection
static uint64_t searchPath[MAX_PLY];

// Check if current position appears in search path (prevents cycles in search)
inline bool isRepetitionInSearchPath(uint64_t hashKey, int ply) {
    // Check every 2 plies (same side to move)
    for (int i = ply - 2; i >= 0; i -= 2) {
        if (searchPath[i] == hashKey) {
            return true;
        }
    }
    return false;
}

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

// move ordering
int scoreMove(const Move &move, const Board &board, int ply) {
    PieceType victim = board.pieceAt(move.to);
    PieceType attacker = board.pieceAt(move.from);

    // 1. Captures (MVV-LVA) - highest priority
    if (victim != PieceType::EMPTY) {
        return 1000000 + 10 * pieceValues[victim] - pieceValues[attacker];
    }

    // 2. Promotions - very high priority
    if (move.promotion != PieceType::EMPTY) {
        return 900000 + pieceValues[move.promotion];
    }

    // 3. Killer moves - good quiet moves from sibling nodes
    if (killers[ply].isKiller(move)) {
        return 800000;
    }

    // 4. History heuristic - historically good moves
    return history[move.from][move.to];
}

int getMateScore(int ply) {
    return -MATE_SCORE + ply;
}

// alpha-beta search with TT integration (Stockfish-style)
int alphaBeta(Board &board, int depth, int alpha, int beta, int ply, Move* bestMoveOut) {
    if (out_of_time()) return alpha;
    
    int originalAlpha = alpha;
    uint64_t hashKey = board.hashKey;
    
    stats.nodes++;
    
    // Store position in search path for repetition detection
    searchPath[ply] = hashKey;
    
    // REPETITION CHECK FIRST
    // Check for 2-fold in search path (upcoming repetition)
    if (ply > 0 && isRepetitionInSearchPath(hashKey, ply)) {
        return 0;
    }
    
    // Check for threefold repetition in game history
    if (board.isThreefoldRepetition()) {
        return 0;
    }
    
    // Probe transposition table
    TT::TTEntry* ttEntry = TT::tt.probe(hashKey);
    Move ttMove;
    
    // TT probe for move ordering and cutoffs
    if (ttEntry != nullptr && ttEntry->depth >= depth) {
        ttMove = ttEntry->bestMove;
        
        // TT CUTOFFS - only at ply >= 3 (avoid repetition issues near root)
        if (ply >= 3) {
            if (ttEntry->type == TT::EXACT) {
                if (bestMoveOut) *bestMoveOut = ttMove;
                return ttEntry->value;
            } else if (ttEntry->type == TT::LOWERBOUND) {
                if (ttEntry->value >= beta) {
                    if (bestMoveOut) *bestMoveOut = ttMove;
                    return beta;
                }
                alpha = std::max(alpha, (int)ttEntry->value);
            } else if (ttEntry->type == TT::UPPERBOUND) {
                if (ttEntry->value <= alpha) {
                    if (bestMoveOut) *bestMoveOut = ttMove;
                    return alpha;
                }
                beta = std::min(beta, (int)ttEntry->value);
            }
        }
    } else if (ttEntry != nullptr) {
        // TT hit but insufficient depth - still use for move ordering
        ttMove = ttEntry->bestMove;
    }
    
    // terminal node - return evaluation
    if (depth == 0) {
        return Evaluation::evaluate(board);
    }
    
    // generate moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);
    
    // sort moves with score (TT move > Captures > Killers > History)
    for (Move &move : legalMoves) {
        // Give TT move highest priority
        if (ttMove.from != 0 && move.from == ttMove.from && move.to == ttMove.to && 
            move.promotion == ttMove.promotion) {
            move.score = 2000000;
        } else {
            move.score = scoreMove(move, board, ply);
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
            // Update killer moves and history for quiet moves
            PieceType victim = board.pieceAt(move.to);
            if (victim == PieceType::EMPTY && move.promotion == PieceType::EMPTY) {
                // Killer moves: store quiet move that caused cutoff
                killers[ply].add(move);
                
                // History heuristic: reward with depth² bonus
                int bonus = depth * depth;
                history[move.from][move.to] += bonus;
                // Cap to prevent overflow
                if (history[move.from][move.to] > HISTORY_MAX) {
                    history[move.from][move.to] = HISTORY_MAX;
                }
            }
            
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
    
    // Clear killer moves for new search
    for (int i = 0; i < MAX_PLY; i++) {
        killers[i].clear();
    }
    
    // Clear history table for new search
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            history[i][j] = 0;
        }
    }
    
    // Clear search path for repetition detection
    for (int i = 0; i < MAX_PLY; i++) {
        searchPath[i] = 0;
    }
    
    start_time = std::chrono::steady_clock::now();
    time_limit_ms = 9500; // 9 seconds
    
    // generate root moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);
    
    // Check TT for move ordering
    uint64_t hashKey = board.hashKey;
    TT::TTEntry* ttEntry = TT::tt.probe(hashKey);
    
    // sort moves with score (TT > Captures > Killers > History)
    for (Move &move : legalMoves) {
        // Give TT move highest priority
        if (ttEntry && ttEntry->bestMove.from != 0 && 
            move.from == ttEntry->bestMove.from && 
            move.to == ttEntry->bestMove.to &&
            move.promotion == ttEntry->bestMove.promotion) {
            move.score = 2000000;  // TT move - highest priority
        } else {
            move.score = scoreMove(move, board, 0);  // ply=0 at root
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
    }
    
    return bestMove;
}
} // namespace Search
//
