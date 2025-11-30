# Magnus Chess Engine - Test Suite

This directory contains comprehensive tests for validating the chess engine implementation (Steps 1-3).

## Test Files

### 1. `perft.cpp` - Move Generation Validation
**Purpose:** Validates that move generation is 100% correct by counting all positions at various depths.

**What it tests:**
- All piece movements (pawns, knights, bishops, rooks, queens, king)
- Castling (kingside and queenside for both colors)
- En passant captures
- Pawn promotions
- Legal move filtering (king safety)
- Check detection

**How to run:**
```bash
# Build tests
cd build
cmake ..
make

# Run perft tests (checks depths 1-5)
./perft_test

# Run perft with move breakdown (useful for debugging)
./perft_test divide 3
./perft_test divide 4
```

**Expected Results:**
- Depth 1: 20 nodes
- Depth 2: 400 nodes
- Depth 3: 8,902 nodes
- Depth 4: 197,281 nodes
- Depth 5: 4,865,609 nodes

If any number is wrong, there's a bug in move generation!

---

### 2. `unit_test.cpp` - Component Testing
**Purpose:** Tests individual components and functions in isolation.

**What it tests:**
- Board initialization
- Piece placement
- Castling rights tracking
- En passant target tracking
- Move parsing (e2e4, e7e8q, etc.)
- Move generation for individual piece types
- Legal move filtering
- Check detection

**How to run:**
```bash
./unit_test
```

---

### 3. `moves_test.txt` - Integration Test Input
**Purpose:** Sample game history for testing the full engine.

**How to use:**
```bash
./MagnusCarlsenMogger -H test/moves_test.txt -m output.txt
```

---

## Quick Test Guide

### After making changes to move generation:
```bash
cd build
make
./perft_test          # Should take ~30 seconds for depth 5
./unit_test           # Should be instant
```

### If perft fails:
```bash
./perft_test divide 3  # Shows which moves have wrong counts
```

### Before committing code:
```bash
# All tests must pass!
./perft_test && ./unit_test && echo "✓ All tests passed"
```

---

## Test Coverage

| Feature | Perft | Unit Tests |
|---------|-------|------------|
| Pawn moves | ✓ | ✓ |
| Pawn double push | ✓ | ✓ |
| Pawn captures | ✓ | - |
| En passant | ✓ | - |
| Promotions | ✓ | ✓ |
| Knight moves | ✓ | ✓ |
| Bishop moves | ✓ | - |
| Rook moves | ✓ | - |
| Queen moves | ✓ | - |
| King moves | ✓ | - |
| Castling | ✓ | ✓ |
| Check detection | ✓ | ✓ |
| Pin detection | ✓ | - |
| Legal move filtering | ✓ | ✓ |
| Board state tracking | - | ✓ |
| Move parsing | - | ✓ |

---

## Performance Benchmarks

**Target speeds (for reference):**
- Depth 1: < 1 ms
- Depth 2: < 1 ms
- Depth 3: < 10 ms
- Depth 4: < 200 ms
- Depth 5: ~30 seconds (depends on hardware)

Current implementation should achieve **1-10 million nodes/second**.

---

## Adding New Tests

### To add a perft test position:
```cpp
// In perft.cpp, add to tests vector:
{"Position Name", "", setupFunction, depth, expectedNodes}
```

### To add a unit test:
```cpp
// In unit_test.cpp:
TEST(your_test_name) {
    // Test code here
    ASSERT_EQ(actual, expected);
}

// Then in main():
RUN_TEST(your_test_name);
```

---

## Troubleshooting

**Q: Perft fails at depth 3 with 8903 instead of 8902**
A: You have a bug generating one extra move. Run `./perft_test divide 2` to narrow down which line has the error.

**Q: Tests are too slow**
A: Normal for depth 5. If depth 3-4 are slow (>1 second), there might be optimization opportunities.

**Q: Unit tests pass but perft fails**
A: Unit tests check individual components. Perft checks the complete system. The bug is likely in how components interact.

**Q: Perft passes but engine makes illegal moves**
A: Perft validates move generation, not move application. Check your `update_move()` function.

---

## Next Steps

After all tests pass:
1. ✅ Steps 1-3 are complete
2. ➡️ Add evaluation function (Step 5A)
3. ➡️ Implement search (Step 5B)
4. ➡️ Add more advanced tests (tactical positions, endgames)

