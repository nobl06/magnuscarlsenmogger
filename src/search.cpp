#include "search.h"
#include "eval/evaluate.h"
#include "gen.hpp"
#include "tt.h"
#include "zobrist.h"
#include <algorithm>
#include <chrono>
#include <cmath>
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

// Late Move Reduction table
int reductionTable[LMR_TABLE_SIZE][LMR_TABLE_SIZE];

// Initialize LMR reduction table
void initReductions() {
    for (int depth = 1; depth < LMR_TABLE_SIZE; ++depth) {
        for (int moves = 1; moves < LMR_TABLE_SIZE; ++moves) {
            // base reduction based on depth and move count
            reductionTable[depth][moves] = int(0.5 + std::log(depth) * std::log(moves) / 2.25);
        }
    }
}

// Search path tracking for repetition detection
static uint64_t searchPath[MAX_PLY];

//Check if we have an upcoming repetition
// Combines game history + search path (unified check)
inline bool upcoming_repetition(const Board& board, uint64_t hashKey, int ply) {
    // Only check every 2 plies (same side to move)
    for (int i = ply - 2; i >= 0; i -= 2) {
        if (searchPath[i] == hashKey) {
            return true;
        }
    }
    
    // Check game history (positions that happened before search)
    // These were played in the actual game
    for (const auto& h : board.hashHistory) {
        if (h == hashKey) {
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

// Adjusts mate scores from "plies to mate from root" to "plies to mate from current position"
inline int value_to_tt(int v, int ply) {
    if (v >= MATE_SCORE - MAX_PLY)
        return v + ply;  // Mate in X moves
    if (v <= -MATE_SCORE + MAX_PLY)
        return v - ply;  // Mated in X moves
    return v;
}

// Inverse: adjusts from "plies to mate from current position" to "plies to mate from root"
inline int value_from_tt(int v, int ply) {
    if (v >= MATE_SCORE - MAX_PLY)
        return v - ply;
    if (v <= -MATE_SCORE + MAX_PLY)
        return v + ply;
    return v;
}

// quiescence search - searches only tactical moves (captures/promotions) until quiet
int quiescence(Board &board, int alpha, int beta, int ply) {
    // Prevent stack overflow
    if (ply >= MAX_PLY) {
        return Evaluation::evaluate(board);
    }
    
    if (out_of_time()) return alpha;
    
    stats.nodes++;
    
    // get stand-pat score (static evaluation)
    int standPat = Evaluation::evaluate(board);
    
    // beta cutoff 
    if (standPat >= beta) {
        return beta;
    }
    
    // update alpha with stand-pat
    if (standPat > alpha) {
        alpha = standPat;
    }
    
    // delta pruning constant - roughly queen value
    // if even capturing a queen can't raise alpha, skip searching
    constexpr int delta = 900;
    
    // generate all moves
    MoveGenerator gen(board, board.sideToMove);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legalMoves = gen.filterLegalMoves(pseudoLegal);
    
    // filter to only tactical moves (captures and promotions)
    std::vector<Move> tacticalMoves;
    for (const Move& move : legalMoves) {
        PieceType victim = board.pieceAt(move.to);
        bool isCapture = (victim != PieceType::EMPTY);
        bool isPromotion = (move.promotion != PieceType::EMPTY);
        
        if (isCapture || isPromotion) {
            tacticalMoves.push_back(move);
        }
    }
    
    // sort tactical moves by MVV-LVA
    for (Move& move : tacticalMoves) {
        move.score = scoreMove(move, board, ply);
    }
    std::sort(tacticalMoves.begin(), tacticalMoves.end(),
              [](const Move& a, const Move& b) { return a.score > b.score; });
    
    // search tactical moves
    for (const Move& move : tacticalMoves) {
        if (out_of_time()) break;
        
        // delta pruning - if this capture can't possibly raise alpha, skip it
        PieceType victim = board.pieceAt(move.to);
        if (victim != PieceType::EMPTY) {
            int captureValue = pieceValues[victim];
            
            // if stand-pat + captured piece value + very optimistic delta can't beat alpha, prune
            if (standPat + captureValue + delta < alpha) {
                continue; 
            }
        }
        
        BoardState state = board.makeMove(move);
        int score = -quiescence(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move, state);
        
        if (out_of_time()) break;
        
        if (score >= beta) {
            return beta;  
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

// alpha-beta search with TT integration
int alphaBeta(Board &board, int depth, int alpha, int beta, int ply, bool pvNode, Move* pv, Move* bestMoveOut, bool isNullMove) {
    // Initialize PV
    if (pv) pv[0] = Move();
    if (out_of_time()) return alpha;
    
    int originalAlpha = alpha;
    uint64_t hashKey = board.hashKey;
    
    stats.nodes++;
    
    // Store position in search path for repetition detection
    searchPath[ply] = hashKey;
    
    // Prevent TT from suggesting moves that lead to repetition
    if (ply > 0 && upcoming_repetition(board, hashKey, ply)) {
        // Return draw score
        return 0;
    }
    
    // Probe transposition table
    TT::TTEntry* ttEntry = TT::tt.probe(hashKey);
    Move ttMove;
    
    // TT CUTOFFS only at NON-PV nodes
    // PV nodes (root and expected best line) always get full search
    if (!pvNode && ttEntry != nullptr && ttEntry->depth >= depth) {
        ttMove = ttEntry->bestMove;
        
        // Adjust mate scores from TT
        int ttValue = value_from_tt(ttEntry->value, ply);
        
        if (ttEntry->type == TT::EXACT) {
            if (bestMoveOut) *bestMoveOut = ttMove;
            return ttValue;
        } else if (ttEntry->type == TT::LOWERBOUND) {
            if (ttValue >= beta) {
                if (bestMoveOut) *bestMoveOut = ttMove;
                return beta;
            }
            alpha = std::max(alpha, ttValue);
        } else if (ttEntry->type == TT::UPPERBOUND) {
            if (ttValue <= alpha) {
                if (bestMoveOut) *bestMoveOut = ttMove;
                return alpha;
            }
            beta = std::min(beta, ttValue);
        }
    } else if (ttEntry != nullptr) {
        // TT hit but insufficient depth - still use for move ordering
        ttMove = ttEntry->bestMove;
    }
    
    // terminal node - quiescence search
    if (depth == 0) {
        return quiescence(board, alpha, beta, ply);
    }
    
    
    // Check if in endgame
    bool inEndgame = (board.bitboards[board.sideToMove][KNIGHT] == 0 && 
                      board.bitboards[board.sideToMove][BISHOP] == 0 &&
                      board.bitboards[board.sideToMove][ROOK] == 0 &&
                      board.bitboards[board.sideToMove][QUEEN] == 0);
    
    // Null move pruning
    if (depth >= 3 && !pvNode && !isNullMove && !inEndgame && !board.isKingInCheck(board.sideToMove)) {
        // Evaluate current position
        int staticEval = Evaluation::evaluate(board);
        
        // Only try NMP if we're in a good position
        if (staticEval >= beta) {
            // Adaptive reduction
            int R = 3 + depth / 3;
            R += std::min((staticEval - beta) / 200, 2);
            R = std::min(R, depth - 1);
            
            // Make null move
            board.makeNullMove();
            
            // Search with reduced depth
            int nullScore = -alphaBeta(board, depth - R - 1 , -beta, -beta + 1, ply + 1, false, nullptr, nullptr, true);
            
            // Unmake null move
            board.unmakeNullMove();
            
            // If even after passing we are winning, cut off
            if (nullScore >= beta) {
                // Mate scores from null move can be unreliable 
                if (nullScore >= MATE_SCORE - MAX_PLY) {
                    return beta;
                }
                return nullScore;
            }
        }
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
    int moveCount = 0;
    
    // Child PV array
    Move childPv[MAX_PLY];
    
    for (const Move &move : legalMoves) {
        if (out_of_time()) break;
        
        moveCount++;
        
        // Check if this is a tactical move (capture or promotion)
        PieceType victim = board.pieceAt(move.to);
        bool isCapture = (victim != PieceType::EMPTY);
        bool isPromotion = (move.promotion != PieceType::EMPTY);
        
        // make/unmake method (efficient - no board copying)
        BoardState state = board.makeMove(move);
        
        // PV tracking:
        bool childPvNode = pvNode && (moveCount == 1 || alpha > originalAlpha);
        
        // Initialize child PV for this move
        childPv[0] = Move();
        
        int score;
        
        // === Late Move Reduction (LMR) ===
        // First move: always search at full depth with full window
        if (moveCount == 1) {
            score = -alphaBeta(board, depth - 1, -beta, -alpha, ply + 1, childPvNode, childPv);
        }
        // Later moves: use LMR with re-search
        else {
            // Calculate reduction for non-tactical quiet moves
            int reduction = 0;
            
            // Only reduce quiet moves at sufficient depth
            if (depth >= 3 && !isCapture && !isPromotion) {
                // Get base reduction from table
                int d = std::min(depth, LMR_TABLE_SIZE - 1);
                int m = std::min(moveCount, LMR_TABLE_SIZE - 1);
                reduction = reductionTable[d][m];
                
                // Don't reduce too much
                reduction = std::min(reduction, depth - 2);
            }
            
            // Step 1: Search with reduced depth and null window
            if (reduction > 0) {
                score = -alphaBeta(board, depth - 1 - reduction, -(alpha + 1), -alpha, ply + 1, false, childPv);
            } else {
                // No reduction: just null window search (PVS)
                score = -alphaBeta(board, depth - 1, -(alpha + 1), -alpha, ply + 1, false, childPv);
            }
            
            // Step 2: If reduced search failed high, re-search at full depth with null window
            if (reduction > 0 && score > alpha) {
                score = -alphaBeta(board, depth - 1, -(alpha + 1), -alpha, ply + 1, false, childPv);
            }
            
            // Step 3: If still failed high and we're in PV node, re-search with full window
            if (score > alpha && score < beta && pvNode) {
                score = -alphaBeta(board, depth - 1, -beta, -alpha, ply + 1, true, childPv);
            }
        }
        
        // unmake the move to restore board state
        board.unmakeMove(move, state);
        
        if (out_of_time()) break;
        
        // if this move is better than any seen so far
        // update best score and PV
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Update PV: current move + child's PV
            if (pv) {
                pv[0] = move;
                for (int i = 0; i < MAX_PLY - 1 && childPv[i].from != 0; i++) {
                    pv[i + 1] = childPv[i];
                }
                pv[MAX_PLY - 1] = Move();
            }
        }
        
        // beta cutoff - check first for efficiency
        // If score >= beta, opponent won't allow this line
        if (score >= beta) {
            // Update killer moves and history for quiet moves
            if (!isCapture && !isPromotion) {
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
    
    // Adjust mate scores for TT storage
    int ttScore = value_to_tt(bestScore, ply);
    TT::tt.store(hashKey, ttScore, depth, nodeType, bestMove);
    
    if (bestMoveOut) *bestMoveOut = bestMove;
    return bestScore;
}

// main search function with TT integration
Move findBestMove(Board &board, int depth) {
    stats.reset();
    info.reset();
    info.maxDepth = depth;
    
    // Initialize LMR reduction table
    static bool reductionsInitialized = false;
    if (!reductionsInitialized) {
        initReductions();
        reductionsInitialized = true;
    }
    
    // Increment TT generation for new search
    TT::tt.new_search();
    
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
    
    // PV from previous iteration
    Move previousPv[MAX_PLY];
    for (int i = 0; i < MAX_PLY; i++) previousPv[i] = Move();
    
    // Iterative deepening
    for (int currentDepth = 1; currentDepth <= depth; currentDepth++) {
        auto layer_start = std::chrono::steady_clock::now(); // Timing a depth
        uint64_t nodesAtStart = stats.nodes;
        
        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;
        
        Move bestMoveThisIter = legalMoves[0];
        int bestScoreThisIter = -INFINITY_SCORE;
        
        // PV for current iteration
        Move currentPv[MAX_PLY];
        for (int i = 0; i < MAX_PLY; i++) currentPv[i] = Move();
        
        // Re-sort moves using PV from previous iteration
        if (currentDepth > 1 && previousPv[0].from != 0) {
            for (Move &move : legalMoves) {
                // PV move from previous iteration gets highest priority
                if (move.from == previousPv[0].from && move.to == previousPv[0].to &&
                    move.promotion == previousPv[0].promotion) {
                    move.score = 3000000;
                }
            }
            std::sort(legalMoves.begin(), legalMoves.end(),
                      [](const Move &a, const Move &b) { return a.score > b.score; });
        }
        
        for (const Move &move : legalMoves) {
            if (out_of_time()) break;   

            // Child PV for this move
            Move childPv[MAX_PLY];
            for (int i = 0; i < MAX_PLY; i++) childPv[i] = Move();
            
            BoardState state = board.makeMove(move);
            // Root is always PV node - pass childPv array
            int score = -alphaBeta(board, currentDepth - 1, -beta, -alpha, 1, true, childPv);
            board.unmakeMove(move, state);
            
            if (out_of_time()) break;
            
            if (score > bestScoreThisIter) {
                bestScoreThisIter = score;
                bestMoveThisIter = move;
                
                // Update current iteration's PV
                currentPv[0] = move;
                for (int i = 0; i < MAX_PLY - 1 && childPv[i].from != 0; i++) {
                    currentPv[i + 1] = childPv[i];
                }
            }
            
            if (score > alpha) {
                alpha = score;
            }
        }
        
        auto layer_end = std::chrono::steady_clock::now();
        auto layer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            layer_end - layer_start)
                            .count();
        
        // Calculate nodes searched this depth
        uint64_t nodesThisDepth = stats.nodes - nodesAtStart;
        
        std::cout << "Depth " << currentDepth
                  << " took " << layer_ms << " ms"
                  << " (nodes: " << nodesThisDepth << ")\n";
        
        if (out_of_time()) {
            std::cout << "Time limit reached after depth " << currentDepth << "\n";
            break;
        }
        
        // Depth completed, update global best move and PV
        bestMove = bestMoveThisIter;
        bestScore = bestScoreThisIter;
        stats.depthReached = currentDepth;
        
        // Copy current PV to previous PV for next iteration 
        for (int i = 0; i < MAX_PLY; i++) {
            previousPv[i] = currentPv[i];
        }
    }
    
    return bestMove;
}
} // namespace Search
//
