#pragma once

#include <cstdint>
#include <vector>
#include "move.h"

namespace TT {
    // Node types
    enum NodeType : uint8_t {
        EXACT = 0,      // PV node - exact score
        LOWERBOUND = 1, // All node - beta cutoff (score >= beta)
        UPPERBOUND = 2  // Cut node - alpha didn't improve (score <= alpha)
    };
    
    // Transposition table entry
    struct TTEntry {
        uint64_t key;        // Full 64-bit Zobrist key
        int16_t value;       // Evaluation score
        int8_t depth;        // Search depth
        NodeType type;       // Node type
        Move bestMove;       // Best move found
        
        TTEntry() : key(0), value(0), depth(-1), type(EXACT), bestMove() {}
    };
    
    // Transposition table class
    class TranspositionTable {
    private:
        std::vector<TTEntry> table;
        size_t mask;
        
    public:
        TranspositionTable(size_t sizeMB = 128);
        
        // Probe the table
        TTEntry* probe(uint64_t key);
        
        // Store an entry
        void store(uint64_t key, int value, int depth, NodeType type, const Move& bestMove);
        
        // Clear the table
        void clear();
        
        // Get index from key
        inline size_t getIndex(uint64_t key) const {
            return key & mask;
        }
    };
    
    // Global transposition table
    extern TranspositionTable tt;
}

