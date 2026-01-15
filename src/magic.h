#pragma once
#include <cstdint>

namespace Magic {

    // Initialize magic bitboards (call once at startup)
    void init();
    
    // Runtime attack functions for sliding pieces (magic bitboards)
    uint64_t getRookAttacks(int square, uint64_t occupied);
    uint64_t getBishopAttacks(int square, uint64_t occupied);
    
    // Runtime attack functions for non-sliding pieces (precomputed tables)
    uint64_t getKnightAttacks(int square);
    uint64_t getKingAttacks(int square);
    
    // Verification function - compares magic bitboards with slow implementation
    // Returns true if all tests pass
    bool verify();
}

