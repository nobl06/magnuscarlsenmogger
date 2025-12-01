#include "gen.hpp"

// PAWNS ------------------------
void MoveGenerator::generatePawnMoves(std::vector<Move> &moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // Determine direction and starting rank based on color
    int direction = (color == Color::WHITE) ? 1 : -1;
    int startRank = (color == Color::WHITE) ? 1 : 6;
    int promotionRank = (color == Color::WHITE) ? 7 : 0;

    std::uint64_t allOccupied = board.getAllPieces();
    std::uint64_t enemyOccupied = (color == Color::WHITE) ? board.getAllBlackPieces() : board.getAllWhitePieces();

    // Every basic forward move
    int toSq = from + direction * 8;
    if (row + direction >= 0 && row + direction < 8) {
        std::uint64_t toBit = 1ULL << toSq;

        if (!(allOccupied & toBit)) {
            if (row + direction == promotionRank) {
                moves.emplace_back(from, toSq, PieceType::QUEEN);
                moves.emplace_back(from, toSq, PieceType::ROOK);
                moves.emplace_back(from, toSq, PieceType::BISHOP);
                moves.emplace_back(from, toSq, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, toSq);
                // If we are at the starting position we can do a double move
                if (row == startRank) {
                    int to2Sq = from + 2 * direction * 8;
                    std::uint64_t to2Bit = 1ULL << to2Sq;
                    if (!(allOccupied & to2Bit)) {
                        moves.emplace_back(from, to2Sq);
                    }
                }
            }
        }
    }

    // all the captures that can be done to the left
    if (col > 0 && row + direction >= 0 && row + direction < 8) {
        int captureSq = from + direction * 8 - 1;
        std::uint64_t captureBit = 1ULL << captureSq;

        if (enemyOccupied & captureBit) {
            int captureRank = Board::row(captureSq);

            if (captureRank == promotionRank) {
                moves.emplace_back(from, captureSq, PieceType::QUEEN);
                moves.emplace_back(from, captureSq, PieceType::ROOK);
                moves.emplace_back(from, captureSq, PieceType::BISHOP);
                moves.emplace_back(from, captureSq, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, captureSq);
            }
        }
    }

    // all the captures that can be done to the right
    if (col < 7 && row + direction >= 0 && row + direction < 8) {
        int captureSq = from + direction * 8 + 1;
        std::uint64_t captureBit = 1ULL << captureSq;

        if (enemyOccupied & captureBit) {
            int captureRank = Board::row(captureSq);

            if (captureRank == promotionRank) {
                moves.emplace_back(from, captureSq, PieceType::QUEEN);
                moves.emplace_back(from, captureSq, PieceType::ROOK);
                moves.emplace_back(from, captureSq, PieceType::BISHOP);
                moves.emplace_back(from, captureSq, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, captureSq);
            }
        }
    }

    // Adds en passant captures if possible
    if (board.enPassantTarget != -1) {
        int epSquare = board.enPassantTarget;
        int epCol = Board::column(epSquare);
        int epRow = Board::row(epSquare);

        // Check if we can capture en passant to the left
        if (col > 0 && epCol == col - 1 && epRow == row + direction) {
            moves.emplace_back(from, epSquare);
        }

        // Check if we can capture en passant to the right
        if (col < 7 && epCol == col + 1 && epRow == row + direction) {
            moves.emplace_back(from, epSquare);
        }
    }
}

// KNIGHTS ------------------------

void MoveGenerator::generateKnightMoves(std::vector<Move> &moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    const int dCol[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    const int dRow[8] = {2, 1, -1, -2, -2, -1, 1, 2};

    for (int i = 0; i < 8; ++i) {
        int nc = col + dCol[i];
        int nr = row + dRow[i];
        if (nc < 0 || nc > 7 || nr < 0 || nr > 7) continue;

        int to = Board::position(nc, nr);

        // skip if our own piece is there
        if (board.isSquareOccupiedByColor(to, color)) continue;

        moves.emplace_back(from, to);
    }
}

//  BISHOPS ------------------------

void MoveGenerator::generateBishopMoves(std::vector<Move> &moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // 4 diagonal directions
    const int dCol[4] = {1, 1, -1, -1};
    const int dRow[4] = {1, -1, 1, -1};

    for (int dir = 0; dir < 4; ++dir) {
        int nc = col + dCol[dir];
        int nr = row + dRow[dir];

        while (nc >= 0 && nc < 8 && nr >= 0 && nr < 8) {
            int to = Board::position(nc, nr);

            if (board.isSquareOccupiedByColor(to, color)) {
                // our own piece blocks further moves
                break;
            }

            moves.emplace_back(from, to);

            if (!board.isSquareEmpty(to)) {
                // captured enemy → stop in this direction
                break;
            }

            nc += dCol[dir];
            nr += dRow[dir];
        }
    }
}

//  ROOKS ------------------------

void MoveGenerator::generateRookMoves(std::vector<Move> &moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // 4 straight directions: up, down, right, left
    const int dCol[4] = {0, 0, 1, -1};
    const int dRow[4] = {1, -1, 0, 0};

    for (int dir = 0; dir < 4; ++dir) {
        int nc = col + dCol[dir];
        int nr = row + dRow[dir];

        while (nc >= 0 && nc < 8 && nr >= 0 && nr < 8) {
            int to = Board::position(nc, nr);

            if (board.isSquareOccupiedByColor(to, color)) {
                // Our own piece blocks
                break;
            }

            moves.emplace_back(from, to);

            if (!board.isSquareEmpty(to)) {
                // Captured enemy → stop along this line
                break;
            }

            nc += dCol[dir];
            nr += dRow[dir];
        }
    }
}

//  QUEENS ------------------------

void MoveGenerator::generateQueenMoves(std::vector<Move> &moves, int from) const {
    generateBishopMoves(moves, from);
    generateRookMoves(moves, from); // queen moves = bishop + rook hence
}

// KING ------------------------

void MoveGenerator::generateKingMoves(std::vector<Move> &moves, int from) const {
    int col = Board::column(from);
    int row = Board::row(from);

    // Adjacent squares
    const int dCol[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dRow[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; ++i) {
        int nc = col + dCol[i];
        int nr = row + dRow[i];
        if (nc < 0 || nc > 7 || nr < 0 || nr > 7) continue;

        int to = Board::position(nc, nr);

        if (board.isSquareOccupiedByColor(to, color)) continue;

        moves.emplace_back(from, to);
    }

    // Castling
    std::uint64_t allOccupied = board.getAllPieces();

    if (color == Color::WHITE) {
        // White kingside castling: e1 to g1
        if (board.whiteCanKingside && from == Board::position(4, 0)) {
            int f1 = Board::position(5, 0);
            int g1 = Board::position(6, 0);
            // Check that f1 and g1 are empty
            if (!((allOccupied >> f1) & 1) && !((allOccupied >> g1) & 1)) {
                moves.emplace_back(from, g1);
            }
        }

        // White queenside castling: e1 to c1
        if (board.whiteCanQueenside && from == Board::position(4, 0)) {
            int d1 = Board::position(3, 0);
            int c1 = Board::position(2, 0);
            int b1 = Board::position(1, 0);
            // Check that b1, c1, d1 are empty
            if (!((allOccupied >> d1) & 1) && !((allOccupied >> c1) & 1) && !((allOccupied >> b1) & 1)) {
                moves.emplace_back(from, c1);
            }
        }
    } else {
        // Black kingside castling: e8 to g8
        if (board.blackCanKingside && from == Board::position(4, 7)) {
            int f8 = Board::position(5, 7);
            int g8 = Board::position(6, 7);
            if (!((allOccupied >> f8) & 1) && !((allOccupied >> g8) & 1)) {
                moves.emplace_back(from, g8);
            }
        }

        // Black queenside castling: e8 to c8
        if (board.blackCanQueenside && from == Board::position(4, 7)) {
            int d8 = Board::position(3, 7);
            int c8 = Board::position(2, 7);
            int b8 = Board::position(1, 7);
            if (!((allOccupied >> d8) & 1) && !((allOccupied >> c8) & 1) && !((allOccupied >> b8) & 1)) {
                moves.emplace_back(from, c8);
            }
        }
    }
}

std::vector<Move> MoveGenerator::generatePseudoLegalMoves() const {
    std::vector<Move> moves;

    int c = color; // WHITE = 0, BLACK = 1

    // Pawns
    uint64_t pawns = board.bitboards[c][PAWN];
    while (pawns) {
        int sq = Board::popLsb(pawns);
        generatePawnMoves(moves, sq);
    }

    // Knights
    uint64_t knights = board.bitboards[c][KNIGHT];
    while (knights) {
        int sq = Board::popLsb(knights);
        generateKnightMoves(moves, sq);
    }

    // Bishops
    uint64_t bishops = board.bitboards[c][BISHOP];
    while (bishops) {
        int sq = Board::popLsb(bishops);
        generateBishopMoves(moves, sq);
    }

    // Rooks
    uint64_t rooks = board.bitboards[c][ROOK];
    while (rooks) {
        int sq = Board::popLsb(rooks);
        generateRookMoves(moves, sq);
    }

    // Queens
    uint64_t queens = board.bitboards[c][QUEEN];
    while (queens) {
        int sq = Board::popLsb(queens);
        generateQueenMoves(moves, sq);
    }

    // King
    uint64_t king = board.bitboards[c][KING];
    if (king) {
        int sq = Board::popLsb(king);
        generateKingMoves(moves, sq);
    }

    return moves;
}

std::vector<Move> MoveGenerator::filterLegalMoves(const std::vector<Move> &pseudoLegalMoves) const {
    std::vector<Move> legalMoves;

    for (const Move &move : pseudoLegalMoves) {
        // Make a copy of the board to test the move
        Board testBoard = board;
        testBoard.update_move(move);

        // We check if our king is in check after the move.
        // Note: after update_move, sideToMove has been toggled, so we check the opposite color
        Color ourColor = (testBoard.sideToMove == Color::WHITE) ? Color::BLACK : Color::WHITE;

        // check if the castling move is legal
        PieceType movedPiece = board.pieceAt(move.from);
        if (movedPiece == PieceType::KING && std::abs(static_cast<int>(move.to) - static_cast<int>(move.from)) == 2) {
            int fromCol = Board::column(move.from);
            int toCol = Board::column(move.to);
            int row = Board::row(move.from);

            // Check if the king is in check before castling
            if (board.isKingInCheck(ourColor)) {
                continue;
            }

            // Check if the king is in check while castling
            int middleCol = (fromCol + toCol) / 2;
            int middleSq = Board::position(middleCol, row);
            if (board.isSquareAttackedBy(middleSq, (ourColor == Color::WHITE) ? Color::BLACK : Color::WHITE)) {
                continue;
            }
        }

        if (!testBoard.isKingInCheck(ourColor)) {
            legalMoves.emplace_back(move);
        }
    }

    return legalMoves;
}

Move chooseMove(const std::vector<Move> &legalMoves) {
    return legalMoves[0];
}
