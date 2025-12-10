#include "tt.h"

namespace TT {
    // Global TT instance (128 MB by default)
    TranspositionTable tt(128);
    
    TranspositionTable::TranspositionTable(size_t sizeMB) : currentGeneration(0) {
        // Calculate number of clusters (each cluster has CLUSTER_SIZE entries)
        size_t sizeBytes = sizeMB * 1024 * 1024;
        size_t numClusters = sizeBytes / sizeof(Cluster);
        
        // Round down to power of 2 for fast modulo with mask
        size_t power = 1;
        while (power * 2 <= numClusters) {
            power *= 2;
        }
        numClusters = power;
        mask = numClusters - 1;
        
        // Allocate table using vector of clusters
        table.resize(numClusters);
    }
    
    
    TTEntry* TranspositionTable::probe(uint64_t key) {
        size_t clusterIndex = getIndex(key);
        Cluster* cluster = &table[clusterIndex];
        
        // Check all 3 entries in the cluster
        for (int i = 0; i < CLUSTER_SIZE; i++) {
            if (cluster->entries[i].key == key) {
                return &cluster->entries[i];
            }
        }
        
        return nullptr;
    }
    
    void TranspositionTable::store(uint64_t key, int value, int depth, NodeType type, const Move& bestMove) {
        size_t clusterIndex = getIndex(key);
        Cluster* cluster = &table[clusterIndex];
        
        // 1. First, check if the position already exists in the cluster
        for (int i = 0; i < CLUSTER_SIZE; i++) {
            if (cluster->entries[i].key == key) {
                // Preserve tt move if we don't have a new one
                if (bestMove.from == 0 && bestMove.to == 0) {
                } else {
                    cluster->entries[i].bestMove = bestMove;
                }
                
                // Always update if same position (can improve with deeper search)
                if (type == EXACT || depth > cluster->entries[i].depth - 4) {
                    cluster->entries[i].value = static_cast<int16_t>(value);
                    cluster->entries[i].depth = static_cast<int8_t>(depth);
                    cluster->entries[i].generation = currentGeneration;
                    cluster->entries[i].type = type;
                }
                return;
            }
        }
        
        // 2. Position not in cluster 
        //replace entry with lowest score
        // Score = depth - 8 * age
        TTEntry* replace = &cluster->entries[0];
        int minScore = replace->depth - 8 * replace->relative_age(currentGeneration);
        
        for (int i = 1; i < CLUSTER_SIZE; i++) {
            int score = cluster->entries[i].depth - 8 * cluster->entries[i].relative_age(currentGeneration);
            if (score < minScore) {
                minScore = score;
                replace = &cluster->entries[i];
            }
        }
        
        // Replace the entry
        replace->key = key;
        replace->value = static_cast<int16_t>(value);
        replace->depth = static_cast<int8_t>(depth);
        replace->generation = currentGeneration;
        replace->type = type;
        replace->bestMove = bestMove;
    }
    
    void TranspositionTable::clear() {
        std::fill(table.begin(), table.end(), Cluster());
        currentGeneration = 0;
    }
    
    void TranspositionTable::new_search() {
        // Increment generation for new search
        currentGeneration++;
    }
}

