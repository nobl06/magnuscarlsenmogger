#include "tt.h"

namespace TT {
    // Global TT instance (128 MB by default)
    TranspositionTable tt(128);
    
    TranspositionTable::TranspositionTable(size_t sizeMB) {
        // Calculate number of entries
        size_t sizeBytes = sizeMB * 1024 * 1024;
        size_t size = sizeBytes / sizeof(TTEntry);
        
        // Round down to power of 2 for fast modulo with mask
        size_t power = 1;
        while (power * 2 <= size) {
            power *= 2;
        }
        size = power;
        mask = size - 1;
        
        // Allocate table using vector
        table.resize(size);
    }
    
    
    TTEntry* TranspositionTable::probe(uint64_t key) {
        size_t index = getIndex(key);
        TTEntry* entry = &table[index];
        
        // Check if the key matches
        if (entry->key == key) {
            return entry;
        }
        
        return nullptr;
    }
    
    void TranspositionTable::store(uint64_t key, int value, int depth, NodeType type, const Move& bestMove) {
        size_t index = getIndex(key);
        TTEntry* entry = &table[index];
        
        // Replace if empty or new depth >= stored depth
        if (entry->key == 0 || depth >= entry->depth) {
            entry->key = key;
            entry->value = static_cast<int16_t>(value);
            entry->depth = static_cast<int8_t>(depth);
            entry->type = type;
            entry->bestMove = bestMove;
        }
    }
    
    void TranspositionTable::clear() {
        std::fill(table.begin(), table.end(), TTEntry());
    }
}

