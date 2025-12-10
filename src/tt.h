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
        uint64_t key;         // Full 64-bit Zobrist key
        int16_t value;        // Evaluation score
        int8_t depth;         // Search depth  
        uint8_t generation;   // Age/generation for replacement strategy
        NodeType type;        // Node type
        Move bestMove;        // Best move found
        
        TTEntry() : key(0), value(0), depth(-1), generation(0), type(EXACT), bestMove() {}
        
        // Calculate age relative to current generation
        uint8_t relative_age(uint8_t currentGen) const {
            // Handles wrap-around correctly
            return (255 + currentGen - generation) & 0xFF;
        }
    };
    
    // Cluster of 3 entries (best relation between efficiency and accuracy)
    static constexpr int CLUSTER_SIZE = 3;
    
    struct Cluster {
        TTEntry entries[CLUSTER_SIZE];
    };
    
    // Transposition table class
    class TranspositionTable {
    private:
        std::vector<Cluster> table;
        size_t mask;
        uint8_t currentGeneration;  // Incremented each search
        
    public:
        TranspositionTable(size_t sizeMB = 128);
        
        // Probe the table
        TTEntry* probe(uint64_t key);
        
        // Store an entry
        void store(uint64_t key, int value, int depth, NodeType type, const Move& bestMove);
        
        // Clear the table
        void clear();
        
        // Start new search (increment generation)
        void new_search();
        
        // Get current generation
        uint8_t generation() const { return currentGeneration; }
        
        // Get cluster index from key
        inline size_t getIndex(uint64_t key) const {
            return key & mask;
        }
    };
    
    // Global transposition table
    extern TranspositionTable tt;
}

