#define UNIT_TEST
#include "main.cpp"
#include <cassert>
#include <iostream>

void resetBoardState() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c]) {
                delete board[r][c];
                board[r][c] = nullptr;
            }
        }
    }
    clearSelection();
    isWhiteTurn = true;
}

Piece* makePiece(const std::string& type, bool white) {
    Piece* p = new Piece;
    p->type = type;
    p->isWhite = white;
    return p;
}

void testWhitePawn() {
    resetBoardState();
    Piece* p = makePiece("white-pawn", true);
    board[6][4] = p;
    selectedPiece = p;
    selectedPos = {6,4};
    moveWhitePawn(5,4);
    assert(board[5][4] == p && board[6][4] == nullptr);
}

void testBlackPawn() {
    resetBoardState();
    Piece* p = makePiece("black-pawn", false);
    board[1][3] = p;
    selectedPiece = p;
    selectedPos = {1,3};
    moveBlackPawn(2,3);
    assert(board[2][3] == p && board[1][3] == nullptr);
}

void testRook() {
    resetBoardState();
    Piece* p = makePiece("white-rook", true);
    board[4][4] = p;
    selectedPiece = p;
    selectedPos = {4,4};
    moveRook(4,7);
    assert(board[4][7] == p && board[4][4] == nullptr);
}

void testKnight() {
    resetBoardState();
    Piece* p = makePiece("white-knight", true);
    board[4][4] = p;
    selectedPiece = p;
    selectedPos = {4,4};
    moveKnight(5,6);
    assert(board[5][6] == p && board[4][4] == nullptr);
}

void testBishop() {
    resetBoardState();
    Piece* p = makePiece("white-bishop", true);
    board[4][4] = p;
    selectedPiece = p;
    selectedPos = {4,4};
    moveBishop(6,6);
    assert(board[6][6] == p && board[4][4] == nullptr);
}

void testQueen() {
    resetBoardState();
    Piece* p = makePiece("white-queen", true);
    board[4][4] = p;
    selectedPiece = p;
    selectedPos = {4,4};
    moveQueen(4,6);
    assert(board[4][6] == p && board[4][4] == nullptr);
}

void testKing() {
    resetBoardState();
    Piece* p = makePiece("white-king", true);
    board[4][4] = p;
    selectedPiece = p;
    selectedPos = {4,4};
    moveKing(5,5);
    assert(board[5][5] == p && board[4][4] == nullptr);
}

void testTurnSwitch() {
    resetBoardState();
    Piece* wp = makePiece("white-pawn", true);
    board[6][0] = wp;
    selectedPiece = wp;
    selectedPos = {6,0};
    moveWhitePawn(5,0);
    assert(!isWhiteTurn);

    Piece* bp = makePiece("black-pawn", false);
    board[1][0] = bp;
    selectedPiece = bp;
    selectedPos = {1,0};
    moveBlackPawn(2,0);
    assert(isWhiteTurn);
}

void testCannotCaptureKing() {
    resetBoardState();
    Piece* rook = makePiece("white-rook", true);
    Piece* king = makePiece("black-king", false);
    board[4][0] = rook;
    board[4][4] = king;
    selectedPiece = rook;
    selectedPos = {4,0};
    moveRook(4,4);
    assert(board[4][0] == rook && board[4][4] == king);
}

void testDetectCheck() {
    resetBoardState();
    Piece* rook = makePiece("white-rook", true);
    Piece* king = makePiece("black-king", false);
    board[4][0] = rook;
    board[4][4] = king;
    selectedPiece = rook;
    selectedPos = {4,0};
    moveRook(4,3);
    assert(board[4][3] == rook);
    assert(isSquareAttacked(4,4,true));
}

void testNoLeavingKingInCheck() {
    resetBoardState();
    Piece* wKing = makePiece("white-king", true);
    Piece* wRook = makePiece("white-rook", true);
    Piece* bRook = makePiece("black-rook", false);
    board[4][4] = wKing;
    board[4][0] = wRook;
    board[4][7] = bRook;
    selectedPiece = wRook;
    selectedPos = {4,0};
    moveRook(4,1);
    assert(board[4][0] == wRook && board[4][1] == nullptr);
}

void testPawnPromotion() {
    resetBoardState();
    Piece* p = makePiece("white-pawn", true);
    board[1][0] = p;
    selectedPiece = p;
    selectedPos = {1,0};
    moveWhitePawn(0,0);
    assert(board[0][0] == p);
    assert(board[0][0]->type == "white-queen");
}

int main() {
    testWhitePawn();
    testBlackPawn();
    testRook();
    testKnight();
    testBishop();
    testQueen();
    testKing();
    testTurnSwitch();
    testCannotCaptureKing();
    testDetectCheck();
    testNoLeavingKingInCheck();
    testPawnPromotion();
    std::cout << "All movement tests passed\n";
    resetBoardState();
    return 0;
}
