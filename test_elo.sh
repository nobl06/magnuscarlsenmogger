#!/bin/bash
# Elo Testing Script for MagnusCarlsenMogger
# Usage: ./test_elo.sh [opponent_elo] [num_games]
#
# Examples:
#   ./test_elo.sh              # Default: 50 games against Stockfish at 1500 Elo
#   ./test_elo.sh 1000 100     # 100 games against Stockfish at 1000 Elo
#   ./test_elo.sh 2000 50      # 50 games against Stockfish at 2000 Elo

cd "$(dirname "$0")"

# Default values
OPPONENT_ELO=${1:-1500}
NUM_GAMES=${2:-50}

# Paths
ENGINE_PATH="./build/MagnusCarlsenMogger_UCI"
CUTECHESS_PATH="./build/cutechess-cli"
STOCKFISH_PATH="stockfish"
PGN_OUTPUT="./build/games_vs_${OPPONENT_ELO}.pgn"

# Check if engine exists
if [ ! -f "$ENGINE_PATH" ]; then
    echo "Error: Engine not found at $ENGINE_PATH"
    echo "Please run: cd build && cmake .. && make"
    exit 1
fi

# Check if cutechess exists
if [ ! -f "$CUTECHESS_PATH" ]; then
    echo "Error: cutechess-cli not found at $CUTECHESS_PATH"
    exit 1
fi

# Check if stockfish is available
if ! command -v stockfish &> /dev/null; then
    echo "Error: Stockfish not found. Please install with: brew install stockfish"
    exit 1
fi

echo "=============================================="
echo "  MagnusCarlsenMogger Elo Testing"
echo "=============================================="
echo ""
echo "Testing against Stockfish at ${OPPONENT_ELO} Elo"
echo "Number of games: ${NUM_GAMES}"
echo "PGN output: ${PGN_OUTPUT}"
echo ""
echo "Starting test... (this may take a while)"
echo ""

# Run cutechess-cli
$CUTECHESS_PATH \
    -engine name=MagnusCarlsenMogger cmd="$ENGINE_PATH" proto=uci \
    -engine name="Stockfish_${OPPONENT_ELO}" cmd="$STOCKFISH_PATH" \
        option.UCI_LimitStrength=true \
        option.UCI_Elo="$OPPONENT_ELO" \
        proto=uci \
    -each tc=30+0.3 \
    -rounds "$NUM_GAMES" \
    -games 2 \
    -pgnout "$PGN_OUTPUT" \
    -recover \
    -repeat

echo ""
echo "=============================================="
echo "  Test Complete!"
echo "=============================================="
echo ""
echo "Games saved to: ${PGN_OUTPUT}"
echo ""
echo "Quick Elo estimate:"
echo "  If you won 50%, your Elo ≈ ${OPPONENT_ELO}"
echo "  For every +10% win rate above 50%, add ~70 Elo"
echo "  For every -10% win rate below 50%, subtract ~70 Elo"
echo ""

