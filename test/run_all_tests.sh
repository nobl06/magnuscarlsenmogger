#!/bin/bash

# Magnus Chess Engine - Test Runner Script
# Runs all tests and reports results

echo "═══════════════════════════════════════════"
echo "  Running All Tests for Magnus Engine"
echo "═══════════════════════════════════════════"
echo ""

# Check if build directory exists
if [ ! -d "../build" ]; then
    echo "❌ Build directory not found!"
    echo "Please run: cd build && cmake .. && make"
    exit 1
fi

cd ../build

# Track overall status
all_passed=true

# Run unit tests
echo "📝 Running Unit Tests..."
echo "----------------------------------------"
if ./unit_test; then
    echo "✓ Unit tests passed"
    echo ""
else
    echo "✗ Unit tests failed"
    echo ""
    all_passed=false
fi

# Run perft tests
echo "🔍 Running Perft Tests..."
echo "----------------------------------------"
if ./perft_test; then
    echo "✓ Perft tests passed"
    echo ""
else
    echo "✗ Perft tests failed"
    echo ""
    all_passed=false
fi

# Test engine with empty history
echo "🎮 Testing Engine with Empty History..."
echo "----------------------------------------"
if ./MagnusCarlsenMogger -H ../test/moves_test.txt -m /tmp/test_output.txt > /dev/null 2>&1; then
    echo "✓ Engine runs successfully"
    echo ""
else
    echo "✗ Engine failed to run"
    echo ""
    all_passed=false
fi

# Final summary
echo "═══════════════════════════════════════════"
if [ "$all_passed" = true ]; then
    echo "  🎉 ALL TESTS PASSED!"
    echo "═══════════════════════════════════════════"
    echo ""
    echo "Your engine is ready for Steps 1-3 ✓"
    echo "Next: Implement evaluation function"
    exit 0
else
    echo "  ⚠️  SOME TESTS FAILED"
    echo "═══════════════════════════════════════════"
    echo ""
    echo "Please fix the failing tests before proceeding."
    exit 1
fi

