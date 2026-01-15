#pragma once
#include <cstdint>

namespace Magic {

    // Initialize magic bitboards (call once at startup)
    void init();
    
    // Runtime attack functions
    uint64_t getRookAttacks(int square, uint64_t occupied);
    uint64_t getBishopAttacks(int square, uint64_t occupied);
}

