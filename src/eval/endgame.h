#pragma once
#include "../board.h"
#include <optional>

namespace Endgame {

// Constants for endgame evaluation
constexpr int VALUE_KNOWN_WIN = 10000;
constexpr int VALUE_DRAW = 0;
constexpr int VALUE_TB_WIN_IN_MAX_PLY = 30000;

// Scale factors
constexpr int SCALE_FACTOR_DRAW = 0;
constexpr int SCALE_FACTOR_NORMAL = 64;
constexpr int SCALE_FACTOR_MAX = 128;
constexpr int SCALE_FACTOR_NONE = 255;

// Piece values for endgame calculations - Stockfish values
constexpr int PawnValueEg = 208;
constexpr int KnightValueMg = 781;
constexpr int BishopValueMg = 825;
constexpr int RookValueMg = 1276;
constexpr int RookValueEg = 1380;
constexpr int QueenValueMg = 2538;
constexpr int QueenValueEg = 2682;

// Endgame types
enum EndgameType {
    // Evaluation functions (return absolute score)
    ENDGAME_KXK,     // K + material vs K
    ENDGAME_KBNK,    // KBN vs K
    ENDGAME_KNNK,    // KNN vs K (draw)
    ENDGAME_KRKP,    // KR vs KP
    ENDGAME_KRKB,    // KR vs KB
    ENDGAME_KRKN,    // KR vs KN
    ENDGAME_KQKP,    // KQ vs KP
    ENDGAME_KQKR,    // KQ vs KR
    ENDGAME_KNNKP,   // KNN vs KP
    
    // Insufficient material (draws)
    ENDGAME_KNK,     // KN vs K (insufficient material)
    ENDGAME_KBK,     // KB vs K (insufficient material)
    ENDGAME_KBKB,    // KB vs KB same color (draw)
    
    // Scaling functions (return scale factor 0-128)
    SCALE_KBPsK,     // KB + pawns vs K
    SCALE_KQKRPs,    // KQ vs KR + pawns
    SCALE_KRPKR,     // KRP vs KR
    SCALE_KRPKB,     // KRP vs KB
    SCALE_KRPPKRP,   // KRPP vs KRP
    SCALE_KPsK,      // K + pawns vs K
    SCALE_KBPKB,     // KBP vs KB
    SCALE_KBPPKB,    // KBPP vs KB
    SCALE_KBPKN,     // KBP vs KN
    
    ENDGAME_NONE     // No specialized endgame
};

// Result of endgame detection
struct EndgameInfo {
    Color strongSide;
    Color weakSide;
    EndgameType type;
    bool hasEvalFunction;  // true = evaluation function, false = scaling function
};

// Main API functions
std::optional<EndgameInfo> detectEndgame(const Board& board);
int evaluate(const Board& board, const EndgameInfo& info);
int getScaleFactor(const Board& board, const EndgameInfo& info);

// Helper functions (used internally)
int push_to_edge(int sq);
int push_to_corner(int sq);
int push_close(int sq1, int sq2);
int push_away(int sq1, int sq2);

// Evaluation functions
int evaluateKXK(const Board& board, Color strongSide);
int evaluateKBNK(const Board& board, Color strongSide);
int evaluateKNNK(const Board& board, Color strongSide);
int evaluateKRKP(const Board& board, Color strongSide);
int evaluateKRKB(const Board& board, Color strongSide);
int evaluateKRKN(const Board& board, Color strongSide);
int evaluateKQKP(const Board& board, Color strongSide);
int evaluateKQKR(const Board& board, Color strongSide);
int evaluateKNNKP(const Board& board, Color strongSide);

// Scaling functions
int scaleKBPsK(const Board& board, Color strongSide);
int scaleKQKRPs(const Board& board, Color strongSide);
int scaleKRPKR(const Board& board, Color strongSide);
int scaleKRPKB(const Board& board, Color strongSide);
int scaleKRPPKRP(const Board& board, Color strongSide);
int scaleKPsK(const Board& board, Color strongSide);
int scaleKBPKB(const Board& board, Color strongSide);
int scaleKBPPKB(const Board& board, Color strongSide);
int scaleKBPKN(const Board& board, Color strongSide);

} // namespace Endgame
