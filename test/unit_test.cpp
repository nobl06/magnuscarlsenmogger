#include "../src/board.h"
#include "../src/gen.hpp"
#include "../src/move.h"
#include <iostream>
#include <cassert>

// Test counters
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    try { \
        std::cout << "Running: " << #name << "... "; \
        test_##name(); \
        std::cout << "✓ PASS\n"; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "✗ FAIL: " << e.what() << "\n"; \
        tests_failed++; \
    } \
} while(0)

// Helper macro for assertions
#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #a + " == " + #b); \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Expected true: ") + #cond); \
    } \
} while(0)

#define ASSERT_FALSE(cond) do { \
    if (cond) { \
        throw std::runtime_error(std::string("Expected false: ") + #cond); \
    } \
} while(0)

// ============= Board Tests =============

TEST(board_initialization) {
    Board board;
    board.initStartPosition();
    
    // Check white pieces
    ASSERT_EQ(__builtin_popcountll(board.whitePawns), 8);
    ASSERT_EQ(__builtin_popcountll(board.whiteKnights), 2);
    ASSERT_EQ(__builtin_popcountll(board.whiteBishops), 2);
    ASSERT_EQ(__builtin_popcountll(board.whiteRooks), 2);
    ASSERT_EQ(__builtin_popcountll(board.whiteQueens), 1);
    ASSERT_EQ(__builtin_popcountll(board.whiteKing), 1);
    
    // Check black pieces
    ASSERT_EQ(__builtin_popcountll(board.blackPawns), 8);
    ASSERT_EQ(__builtin_popcountll(board.blackKnights), 2);
    ASSERT_EQ(__builtin_popcountll(board.blackBishops), 2);
    ASSERT_EQ(__builtin_popcountll(board.blackRooks), 2);
    ASSERT_EQ(__builtin_popcountll(board.blackQueens), 1);
    ASSERT_EQ(__builtin_popcountll(board.blackKing), 1);
    
    // Check initial state
    ASSERT_TRUE(board.whiteCanKingside);
    ASSERT_TRUE(board.whiteCanQueenside);
    ASSERT_TRUE(board.blackCanKingside);
    ASSERT_TRUE(board.blackCanQueenside);
    ASSERT_EQ(board.enPassantTarget, -1);
    ASSERT_EQ(board.sideToMove, Color::WHITE);
}

TEST(board_piece_placement) {
    Board board;
    board.initStartPosition();
    
    // Check white pieces on starting squares
    ASSERT_EQ(board.pieceAt(Board::position(0, 0)), PieceType::ROOK);
    ASSERT_EQ(board.pieceAt(Board::position(4, 0)), PieceType::KING);
    ASSERT_EQ(board.pieceAt(Board::position(1, 1)), PieceType::PAWN);
    
    // Check black pieces
    ASSERT_EQ(board.pieceAt(Board::position(4, 7)), PieceType::KING);
    ASSERT_EQ(board.pieceAt(Board::position(0, 6)), PieceType::PAWN);
    
    // Check empty squares
    ASSERT_EQ(board.pieceAt(Board::position(4, 4)), PieceType::EMPTY);
}

TEST(castling_rights_update) {
    Board board;
    board.initStartPosition();
    
    // Move white king
    Move kingMove(Board::position(4, 0), Board::position(4, 1));
    board.update_move(kingMove);
    
    ASSERT_FALSE(board.whiteCanKingside);
    ASSERT_FALSE(board.whiteCanQueenside);
    ASSERT_TRUE(board.blackCanKingside);
    ASSERT_TRUE(board.blackCanQueenside);
}

TEST(en_passant_tracking) {
    Board board;
    board.initStartPosition();
    
    // Move pawn two squares
    Move pawnMove(Board::position(4, 1), Board::position(4, 3));  // e2 to e4
    board.update_move(pawnMove);
    
    ASSERT_EQ(board.enPassantTarget, Board::position(4, 2));  // e3
}

// ============= Move Generation Tests =============

TEST(starting_position_moves) {
    Board board;
    board.initStartPosition();
    
    MoveGenerator gen(board, Color::WHITE);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legal = gen.filterLegalMoves(pseudoLegal);
    
    ASSERT_EQ(legal.size(), 20);  // 16 pawn + 4 knight moves
}

TEST(pawn_moves_generation) {
    Board board;
    board.clear();
    board.whitePawns = Board::bit(4, 1);  // Pawn on e2
    board.sideToMove = Color::WHITE;
    
    MoveGenerator gen(board, Color::WHITE);
    std::vector<Move> moves;
    gen.generatePawnMoves(moves, Board::position(4, 1));
    
    ASSERT_EQ(moves.size(), 2);  // e2-e3 and e2-e4
}

TEST(knight_moves_generation) {
    Board board;
    board.clear();
    board.whiteKnights = Board::bit(4, 4);  // Knight on e4
    board.whiteKing = Board::bit(0, 0);     // King somewhere
    board.sideToMove = Color::WHITE;
    
    MoveGenerator gen(board, Color::WHITE);
    std::vector<Move> moves;
    gen.generateKnightMoves(moves, Board::position(4, 4));
    
    ASSERT_EQ(moves.size(), 8);  // All 8 knight moves from center
}

TEST(castling_move_generation) {
    Board board;
    board.clear();
    
    // Setup for white kingside castling
    board.whiteKing = Board::bit(4, 0);      // e1
    board.whiteRooks = Board::bit(7, 0);     // h1
    board.whiteCanKingside = true;
    board.sideToMove = Color::WHITE;
    
    MoveGenerator gen(board, Color::WHITE);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    
    // Check that castling move is generated
    bool foundCastling = false;
    for (const Move& m : pseudoLegal) {
        if (m.from == Board::position(4, 0) && m.to == Board::position(6, 0)) {
            foundCastling = true;
            break;
        }
    }
    
    ASSERT_TRUE(foundCastling);
}

// ============= Move Parsing Tests =============

TEST(move_parsing_basic) {
    Move m = parseMove("e2e4");
    ASSERT_EQ(m.from, Board::position(4, 1));
    ASSERT_EQ(m.to, Board::position(4, 3));
    ASSERT_EQ(m.promotion, PieceType::EMPTY);
}

TEST(move_parsing_promotion) {
    Move m = parseMove("e7e8q");
    ASSERT_EQ(m.from, Board::position(4, 6));
    ASSERT_EQ(m.to, Board::position(4, 7));
    ASSERT_EQ(m.promotion, PieceType::QUEEN);
}

TEST(move_to_string) {
    Move m(Board::position(4, 1), Board::position(4, 3));
    ASSERT_EQ(m.toString(), "e2e4");
}

// ============= Legal Move Filtering Tests =============

TEST(filter_illegal_moves_in_check) {
    Board board;
    board.clear();
    
    // White king in check from black rook
    board.whiteKing = Board::bit(4, 0);      // e1
    board.whitePawns = Board::bit(3, 1);     // d2
    board.blackRooks = Board::bit(4, 7);     // e8 (checking)
    board.blackKing = Board::bit(0, 7);      // a8
    board.sideToMove = Color::WHITE;
    
    MoveGenerator gen(board, Color::WHITE);
    std::vector<Move> pseudoLegal = gen.generatePseudoLegalMoves();
    std::vector<Move> legal = gen.filterLegalMoves(pseudoLegal);
    
    // Most moves should be illegal (can't leave king in check)
    // Only moves that block or king moves that escape
    ASSERT_TRUE(legal.size() < pseudoLegal.size());
}

TEST(check_detection) {
    Board board;
    board.clear();
    
    board.whiteKing = Board::bit(4, 0);
    board.blackRooks = Board::bit(4, 7);
    board.blackKing = Board::bit(0, 7);
    
    ASSERT_TRUE(board.isKingInCheck(Color::WHITE));
    ASSERT_FALSE(board.isKingInCheck(Color::BLACK));
}

// ============= Main Test Runner =============

int main() {
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "  Magnus Chess Engine - Unit Tests\n";
    std::cout << "═══════════════════════════════════════════\n\n";
    
    // Run all tests
    try { RUN_TEST(board_initialization); } catch(...) { tests_failed++; }
    try { RUN_TEST(board_piece_placement); } catch(...) { tests_failed++; }
    try { RUN_TEST(castling_rights_update); } catch(...) { tests_failed++; }
    try { RUN_TEST(en_passant_tracking); } catch(...) { tests_failed++; }
    try { RUN_TEST(starting_position_moves); } catch(...) { tests_failed++; }
    try { RUN_TEST(pawn_moves_generation); } catch(...) { tests_failed++; }
    try { RUN_TEST(knight_moves_generation); } catch(...) { tests_failed++; }
    try { RUN_TEST(castling_move_generation); } catch(...) { tests_failed++; }
    try { RUN_TEST(move_parsing_basic); } catch(...) { tests_failed++; }
    try { RUN_TEST(move_parsing_promotion); } catch(...) { tests_failed++; }
    try { RUN_TEST(move_to_string); } catch(...) { tests_failed++; }
    try { RUN_TEST(filter_illegal_moves_in_check); } catch(...) { tests_failed++; }
    try { RUN_TEST(check_detection); } catch(...) { tests_failed++; }
    
    // Summary
    std::cout << "\n═══════════════════════════════════════════\n";
    std::cout << "  Test Summary\n";
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "  Total:  " << (tests_passed + tests_failed) << "\n";
    std::cout << "  Passed: " << tests_passed << " ✓\n";
    std::cout << "  Failed: " << tests_failed << (tests_failed > 0 ? " ✗" : "") << "\n";
    std::cout << "═══════════════════════════════════════════\n";
    
    if (tests_failed == 0) {
        std::cout << "\n🎉 All unit tests passed!\n";
        return 0;
    } else {
        std::cout << "\n⚠️  Some tests failed.\n";
        return 1;
    }
}

