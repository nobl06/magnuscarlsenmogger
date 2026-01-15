#include "magic.h"
#include <cstring>

namespace Magic {
    // Phase 2: Data structures
    static uint64_t rookMasks[64];
    static uint64_t bishopMasks[64];
    static uint64_t rookMagics[64];
    static uint64_t bishopMagics[64];
    static int rookShifts[64];
    static int bishopShifts[64];
    static uint64_t* rookAttackTable;
    static uint64_t* bishopAttackTable;
    static uint64_t* rookTableOffset[64];
    static uint64_t* bishopTableOffset[64];
    
    // Helper functions
    static inline int popcount(uint64_t bb) {
        return __builtin_popcountll(bb);
    }
    
    static inline int column(int sq) { return sq % 8; }
    static inline int row(int sq) { return sq / 8; }
    static inline int position(int col, int row) { return row * 8 + col; }
    
    // Phase 3: Build masks
    static uint64_t computeRookMask(int square) {
        uint64_t mask = 0ULL;
        int r = row(square);
        int c = column(square);
        
        // North (stop before edge)
        for (int rr = r + 1; rr < 7; rr++) {
            mask |= (1ULL << position(c, rr));
        }
        
        // South (stop before edge)
        for (int rr = r - 1; rr > 0; rr--) {
            mask |= (1ULL << position(c, rr));
        }
        
        // East (stop before edge)
        for (int cc = c + 1; cc < 7; cc++) {
            mask |= (1ULL << position(cc, r));
        }
        
        // West (stop before edge)
        for (int cc = c - 1; cc > 0; cc--) {
            mask |= (1ULL << position(cc, r));
        }
        
        return mask;
    }
    
    static uint64_t computeBishopMask(int square) {
        uint64_t mask = 0ULL;
        int r = row(square);
        int c = column(square);
        
        // Northeast
        for (int rr = r + 1, cc = c + 1; rr < 7 && cc < 7; rr++, cc++) {
            mask |= (1ULL << position(cc, rr));
        }
        
        // Northwest
        for (int rr = r + 1, cc = c - 1; rr < 7 && cc > 0; rr++, cc--) {
            mask |= (1ULL << position(cc, rr));
        }
        
        // Southeast
        for (int rr = r - 1, cc = c + 1; rr > 0 && cc < 7; rr--, cc++) {
            mask |= (1ULL << position(cc, rr));
        }
        
        // Southwest
        for (int rr = r - 1, cc = c - 1; rr > 0 && cc > 0; rr--, cc--) {
            mask |= (1ULL << position(cc, rr));
        }
        
        return mask;
    }
    
    // Phase 6: Slow reference implementation for computing attacks
    uint64_t getRookAttacksSlow(int square, uint64_t occupied) {
        uint64_t attacks = 0ULL;
        int r = row(square);
        int c = column(square);
        
        // North
        for (int rr = r + 1; rr < 8; rr++) {
            int sq = position(c, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        // South
        for (int rr = r - 1; rr >= 0; rr--) {
            int sq = position(c, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        // East
        for (int cc = c + 1; cc < 8; cc++) {
            int sq = position(cc, r);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        // West
        for (int cc = c - 1; cc >= 0; cc--) {
            int sq = position(cc, r);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        return attacks;
    }
    
    uint64_t getBishopAttacksSlow(int square, uint64_t occupied) {
        uint64_t attacks = 0ULL;
        int r = row(square);
        int c = column(square);
        
        // Northeast
        for (int rr = r + 1, cc = c + 1; rr < 8 && cc < 8; rr++, cc++) {
            int sq = position(cc, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        // Northwest
        for (int rr = r + 1, cc = c - 1; rr < 8 && cc >= 0; rr++, cc--) {
            int sq = position(cc, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        // Southeast
        for (int rr = r - 1, cc = c + 1; rr >= 0 && cc < 8; rr--, cc++) {
            int sq = position(cc, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        // Southwest
        for (int rr = r - 1, cc = c - 1; rr >= 0 && cc >= 0; rr--, cc--) {
            int sq = position(cc, rr);
            attacks |= (1ULL << sq);
            if (occupied & (1ULL << sq)) break;
        }
        
        return attacks;
    }
    
    // Helper: Convert subset index to occupancy bitboard
    static uint64_t indexToOccupancy(int index, int bits, uint64_t mask) {
        uint64_t occupancy = 0ULL;
        for (int i = 0; i < bits; i++) {
            int bitPos = __builtin_ctzll(mask);
            mask &= mask - 1; // Clear LSB
            if (index & (1 << i)) {
                occupancy |= (1ULL << bitPos);
            }
        }
        return occupancy;
    }
    
    // Phase 4: Pre-computed magic numbers from Lc0 (LeelaChessZero)
    static const uint64_t ROOK_MAGICS[64] = {
        0x088000102088C001ULL, 0x10C0200040001000ULL, 0x83001041000B2000ULL,
        0x0680280080041000ULL, 0x488004000A080080ULL, 0x0100180400010002ULL,
        0x040001C401021008ULL, 0x02000C04A980C302ULL, 0x0000800040082084ULL,
        0x5020C00820025000ULL, 0x0001002001044012ULL, 0x0402001020400A00ULL,
        0x00C0800800040080ULL, 0x4028800200040080ULL, 0x00A0804200802500ULL,
        0x8004800040802100ULL, 0x0080004000200040ULL, 0x1082810020400100ULL,
        0x0020004010080040ULL, 0x2004818010042800ULL, 0x0601010008005004ULL,
        0x4600808002001400ULL, 0x0010040009180210ULL, 0x020412000406C091ULL,
        0x040084228000C000ULL, 0x8000810100204000ULL, 0x0084110100402000ULL,
        0x0046001A00204210ULL, 0x2001040080080081ULL, 0x0144020080800400ULL,
        0x0840108400080229ULL, 0x0480308A0000410CULL, 0x0460324002800081ULL,
        0x620080A001804000ULL, 0x2800802000801006ULL, 0x0002809000800800ULL,
        0x4C09040080802800ULL, 0x4808800C00800200ULL, 0x0200311004001802ULL,
        0x0400008402002141ULL, 0x0410800140008020ULL, 0x000080C001050020ULL,
        0x004080204A020010ULL, 0x0224201001010038ULL, 0x0109001108010004ULL,
        0x0282004844020010ULL, 0x8228180110040082ULL, 0x0001000080C10002ULL,
        0x024000C120801080ULL, 0x0001406481060200ULL, 0x0101243200418600ULL,
        0x0108800800100080ULL, 0x4022080100100D00ULL, 0x0000843040600801ULL,
        0x8301000200CC0500ULL, 0x1000004500840200ULL, 0x1100104100800069ULL,
        0x2001008440001021ULL, 0x2002008830204082ULL, 0x0010145000082101ULL,
        0x01A2001004200842ULL, 0x1007000608040041ULL, 0x000A08100203028CULL,
        0x02D4048040290402ULL
    };
    
    static const uint64_t BISHOP_MAGICS[64] = {
        0x0008201802242020ULL, 0x0021040424806220ULL, 0x4006360602013080ULL,
        0x0004410020408002ULL, 0x2102021009001140ULL, 0x08C2021004000001ULL,
        0x6001031120200820ULL, 0x1018310402201410ULL, 0x401CE00210820484ULL,
        0x001029D001004100ULL, 0x2C00101080810032ULL, 0x0000082581000010ULL,
        0x10000A0210110020ULL, 0x200002016C202000ULL, 0x0201018821901000ULL,
        0x006A0300420A2100ULL, 0x0010014005450400ULL, 0x1008C12008028280ULL,
        0x00010010004A0040ULL, 0x3000820802044020ULL, 0x0000800405A02820ULL,
        0x8042004300420240ULL, 0x10060801210D2000ULL, 0x0210840500511061ULL,
        0x0008142118509020ULL, 0x0021109460040104ULL, 0x00A1480090019030ULL,
        0x0102008808008020ULL, 0x884084000880E001ULL, 0x040041020A030100ULL,
        0x3000810104110805ULL, 0x04040A2006808440ULL, 0x0044040404C01100ULL,
        0x4122B80800245004ULL, 0x0044020502380046ULL, 0x0100400888020200ULL,
        0x01C0002060020080ULL, 0x4008811100021001ULL, 0x8208450441040609ULL,
        0x0408004900008088ULL, 0x0294212051220882ULL, 0x000041080810E062ULL,
        0x10480A018E005000ULL, 0x80400A0204201600ULL, 0x2800200204100682ULL,
        0x0020200400204441ULL, 0x0A500600A5002400ULL, 0x801602004A010100ULL,
        0x0801841008040880ULL, 0x10010880C4200028ULL, 0x0400004424040000ULL,
        0x0401000142022100ULL, 0x00A00010020A0002ULL, 0x1010400204010810ULL,
        0x0829910400840000ULL, 0x0004235204010080ULL, 0x1002008143082000ULL,
        0x11840044440C2080ULL, 0x2802A02104030440ULL, 0x6100000900840401ULL,
        0x1C20A15A90420200ULL, 0x0088414004480280ULL, 0x0000204242881100ULL,
        0x0240080802809010ULL
    };
    
    // Phase 5 & 6: Initialize tables
    void init() {
        // Calculate total table size needed
        int rookTableSize = 0;
        int bishopTableSize = 0;
        
        for (int sq = 0; sq < 64; sq++) {
            rookMasks[sq] = computeRookMask(sq);
            bishopMasks[sq] = computeBishopMask(sq);
            
            int rookBits = popcount(rookMasks[sq]);
            int bishopBits = popcount(bishopMasks[sq]);
            
            rookShifts[sq] = 64 - rookBits;
            bishopShifts[sq] = 64 - bishopBits;
            
            rookTableSize += (1 << rookBits);
            bishopTableSize += (1 << bishopBits);
        }
        
        // Allocate tables
        rookAttackTable = new uint64_t[rookTableSize];
        bishopAttackTable = new uint64_t[bishopTableSize];
        
        memset(rookAttackTable, 0, rookTableSize * sizeof(uint64_t));
        memset(bishopAttackTable, 0, bishopTableSize * sizeof(uint64_t));
        
        // Fill tables
        uint64_t* rookPtr = rookAttackTable;
        uint64_t* bishopPtr = bishopAttackTable;
        
        for (int sq = 0; sq < 64; sq++) {
            rookMagics[sq] = ROOK_MAGICS[sq];
            bishopMagics[sq] = BISHOP_MAGICS[sq];
            
            // Rooks
            {
                rookTableOffset[sq] = rookPtr;
                uint64_t mask = rookMasks[sq];
                int bits = popcount(mask);
                int permutations = 1 << bits;
                
                for (int i = 0; i < permutations; i++) {
                    uint64_t occupancy = indexToOccupancy(i, bits, mask);
                    uint64_t attacks = getRookAttacksSlow(sq, occupancy);
                    uint64_t index = (occupancy * rookMagics[sq]) >> rookShifts[sq];
                    rookPtr[index] = attacks;
                }
                
                rookPtr += permutations;
            }
            
            // Bishops
            {
                bishopTableOffset[sq] = bishopPtr;
                uint64_t mask = bishopMasks[sq];
                int bits = popcount(mask);
                int permutations = 1 << bits;
                
                for (int i = 0; i < permutations; i++) {
                    uint64_t occupancy = indexToOccupancy(i, bits, mask);
                    uint64_t attacks = getBishopAttacksSlow(sq, occupancy);
                    uint64_t index = (occupancy * bishopMagics[sq]) >> bishopShifts[sq];
                    bishopPtr[index] = attacks;
                }
                
                bishopPtr += permutations;
            }
        }
    }
    
    // Phase 7: Runtime functions
    uint64_t getRookAttacks(int square, uint64_t occupied) {
        occupied &= rookMasks[square];
        uint64_t index = (occupied * rookMagics[square]) >> rookShifts[square];
        return rookTableOffset[square][index];
    }
    
    uint64_t getBishopAttacks(int square, uint64_t occupied) {
        occupied &= bishopMasks[square];
        uint64_t index = (occupied * bishopMagics[square]) >> bishopShifts[square];
        return bishopTableOffset[square][index];
    }
}

