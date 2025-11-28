#include "move.h"
#include "board.h"

std::string Move::toString() const {
    std::string result;
    result += static_cast<char>('a' + Board::column(from));
    result += static_cast<char>('1' + Board::row(from));
    result += static_cast<char>('a' + Board::column(to));
    result += static_cast<char>('1' + Board::row(to));
    return result;
};

Move parseMove(std::string &s) {
    int startcol = s[0] - 'a';
    int startrow = s[1] - '1';
    int endcol = s[2] - 'a';
    int endrow = s[3] - '1';
    PieceType promotion = PieceType::EMPTY;
    int startsq = Board::position(startcol, startrow);
    int endsq = Board::position(endcol, endrow);
    if (s.length() == 5) {
        char piece = s[4];
        if (piece == 'q') {
            promotion = PieceType::QUEEN;
        };
        if (piece == 'r') {
            promotion = PieceType::ROOK;
        };
        if (piece == 'b') {
            promotion = PieceType::BISHOP;
        };
        if (piece == 'k') {
            promotion = PieceType::KNIGHT;
        };
    }
    return Move(startsq, endsq, promotion);
}