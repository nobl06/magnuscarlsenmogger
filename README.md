♟️ MagnusCarlsenMogger

MagnusCarlsenMogger is a high-performance chess engine written from scratch in modern C++.
Designed with a strong focus on search efficiency and engine-level optimization, it reaches an estimated strength of ~2800 Elo.

🚀 Overview

This project was built as a part of our C/C++ Programming course. The objective being to design a chess bot in C++ and have it compete against other teams' bots under strict time control.

Core design goals:

Maximum search efficiency

Clean and modular architecture

High-performance move generation

Competitive engine strength

Deep understanding of pruning and reduction strategies

Positional understing and precise evalutations

Respect of time constraints


🧠 Search Framework

The engine is built around an optimized alpha–beta search with:

Iterative Deepening

Principal Variation Search (PVS)

Transposition Tables (Zobrist hashing)

Quiescence Search

Null-Move Pruning

Late Move Reductions (LMR)

Killer Move Heuristic

History Heuristic

Move ordering optimizations


⚡ Move Generation

Magic Bitboards for ultra-fast sliding piece move generation

Bitwise operations for efficient board representation

Optimized attack masks and occupancy calculations

Fully legal move generation


📊 Evaluation Function

The static evaluation function includes:

Material evaluation

Piece-square tables

Positional bonuses

Mobility evaluation

King safety considerations

Endgame scaling


🏎️ Performance Focus

This engine emphasizes:

Cache-friendly data structures

Reduced memory access overhead

Efficient pruning strategies

Aggressive move ordering

Low-level C++ optimizations


📈 Estimated Strength

Estimated rating: ~2800 Elo

(Exact strength may vary depending on time controls and hardware.)
