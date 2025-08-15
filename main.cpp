#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <iostream>
#include <cctype>

const int TILE_SIZE = 100;
const int BOARD_SIZE = 8;

struct Piece {
    sf::Sprite sprite;
    std::string type;
    bool isWhite;
};

Piece* selectedPiece = nullptr;
sf::Vector2i selectedPos;
Piece* board[8][8] = {nullptr};
bool isWhiteTurn = true;
std::map<std::string, sf::Texture> textures;

bool isInsideBoard(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

bool isPathClear(int startRow, int startCol, int endRow, int endCol) {
    int dRow = (endRow > startRow) - (endRow < startRow);
    int dCol = (endCol > startCol) - (endCol < startCol);
    int r = startRow + dRow;
    int c = startCol + dCol;
    while (r != endRow || c != endCol) {
        if (board[r][c] != nullptr) return false;
        r += dRow;
        c += dCol;
    }
    return true;
}

void findKing(bool white, int& row, int& col) {
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            Piece* p = board[r][c];
            if (p && p->isWhite == white && p->type.find("king") != std::string::npos) {
                row = r;
                col = c;
                return;
            }
        }
    }
    row = col = -1;
}

bool isSquareAttacked(int row, int col, bool byWhite) {
    // Pawns
    int dir = byWhite ? -1 : 1;
    int pr = row + dir;
    if (isInsideBoard(pr, col - 1) && board[pr][col - 1] &&
        board[pr][col - 1]->isWhite == byWhite && board[pr][col - 1]->type.find("pawn") != std::string::npos)
        return true;
    if (isInsideBoard(pr, col + 1) && board[pr][col + 1] &&
        board[pr][col + 1]->isWhite == byWhite && board[pr][col + 1]->type.find("pawn") != std::string::npos)
        return true;

    // Knights
    int knightMoves[8][2] = {{2,1},{1,2},{-1,2},{-2,1},{-2,-1},{-1,-2},{1,-2},{2,-1}};
    for (auto& m : knightMoves) {
        int r = row + m[0];
        int c = col + m[1];
        if (isInsideBoard(r,c) && board[r][c] && board[r][c]->isWhite == byWhite &&
            board[r][c]->type.find("knight") != std::string::npos)
            return true;
    }

    // Kings
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr;
            int c = col + dc;
            if (isInsideBoard(r,c) && board[r][c] && board[r][c]->isWhite == byWhite &&
                board[r][c]->type.find("king") != std::string::npos)
                return true;
        }
    }

    // Rooks/Queens (straight lines)
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (auto& d : dirs) {
        int r = row + d[0];
        int c = col + d[1];
        while (isInsideBoard(r,c)) {
            if (board[r][c]) {
                if (board[r][c]->isWhite == byWhite &&
                    (board[r][c]->type.find("rook") != std::string::npos ||
                     board[r][c]->type.find("queen") != std::string::npos))
                    return true;
                break;
            }
            r += d[0];
            c += d[1];
        }
    }

    // Bishops/Queens (diagonals)
    int bdirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto& d : bdirs) {
        int r = row + d[0];
        int c = col + d[1];
        while (isInsideBoard(r,c)) {
            if (board[r][c]) {
                if (board[r][c]->isWhite == byWhite &&
                    (board[r][c]->type.find("bishop") != std::string::npos ||
                     board[r][c]->type.find("queen") != std::string::npos))
                    return true;
                break;
            }
            r += d[0];
            c += d[1];
        }
    }
    return false;
}

bool wouldLeaveInCheck(int startRow, int startCol, int endRow, int endCol) {
    Piece* moving = board[startRow][startCol];
    Piece* captured = board[endRow][endCol];
    board[endRow][endCol] = moving;
    board[startRow][startCol] = nullptr;
    int kRow, kCol;
    findKing(moving->isWhite, kRow, kCol);
    bool inCheck = isSquareAttacked(kRow, kCol, !moving->isWhite);
    board[startRow][startCol] = moving;
    board[endRow][endCol] = captured;
    return inCheck;
}

void promotePawn(Piece* pawn) {
    std::string color = pawn->isWhite ? "white" : "black";
    std::string choice;
#ifdef UNIT_TEST
    choice = "queen";
#else
    std::cout << "Promote pawn to (q)ueen, (r)ook, (b)ishop, or k(n)ight? ";
    char c;
    std::cin >> c;
    c = std::tolower(c);
    if (c == 'r')
        choice = "rook";
    else if (c == 'b')
        choice = "bishop";
    else if (c == 'n')
        choice = "knight";
    else
        choice = "queen";
    pawn->sprite.setTexture(textures[color + "-" + choice]);
#endif
    pawn->type = color + "-" + choice;
}

bool finalizeMove(int startRow, int startCol, int row, int col) {
    if (board[row][col] && board[row][col]->type.find("king") != std::string::npos) {
        std::cout << "Cannot capture the king.\n";
        selectedPiece = nullptr;
        selectedPos = sf::Vector2i(-1, -1);
        return false;
    }

    bool moverIsWhite = selectedPiece->isWhite;
    if (board[row][col] != nullptr) delete board[row][col];
    board[row][col] = selectedPiece;
    board[startRow][startCol] = nullptr;
    Piece* movedPiece = selectedPiece;
    std::cout << "Moved piece: " << board[row][col]->type << " to (" << col << ", " << row << ")\n";
    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
    if (movedPiece->type.find("pawn") != std::string::npos && (row == 0 || row == 7)) {
        promotePawn(movedPiece);
    }
    isWhiteTurn = !isWhiteTurn;

    int kRow, kCol;
    findKing(!moverIsWhite, kRow, kCol);
    if (kRow != -1 && isSquareAttacked(kRow, kCol, moverIsWhite)) {
        std::cout << (!moverIsWhite ? "White" : "Black") << " king is in check\n";
    }
    return true;
}

void drawBoard(sf::RenderWindow& window) {
    sf::RectangleShape square(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            bool isWhite = (row + col) % 2 == 0;
            square.setFillColor(isWhite ? sf::Color(240, 217, 181) : sf::Color(181, 136, 99));
            square.setPosition(col * TILE_SIZE, row * TILE_SIZE);
            window.draw(square);
        }
    }
}

Piece* createPiece(std::string name) {
    Piece* piece = new Piece;
    piece->sprite.setTexture(textures[name]);
    piece->sprite.setScale(TILE_SIZE / 128.0f, TILE_SIZE / 128.0f);
    piece->type = name;
    piece->isWhite = name.find("white") != std::string::npos;
    return piece;
}

void loadTextures() {
    std::string pieces[] = {
        "white-pawn", "white-knight", "white-bishop", "white-rook", "white-queen", "white-king",
        "black-pawn", "black-knight", "black-bishop", "black-rook", "black-queen", "black-king"
    };
    for (const auto& piece : pieces) {
        sf::Texture tex;
        tex.loadFromFile("assets/pieces/" + piece + ".png");
        textures[piece] = tex;
    }
}

void drawPieces(sf::RenderWindow& window) {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Piece* piece = board[row][col];
            if (piece) {
                float offset = (TILE_SIZE - 128.0f * piece->sprite.getScale().x) / 2.0f;
                piece->sprite.setPosition(col * TILE_SIZE + offset, row * TILE_SIZE + offset);
                window.draw(piece->sprite);
            }
        }
    }
}

void default_board() {
    for (int i = 0; i < 8; ++i) {
        board[1][i] = createPiece("black-pawn");
        board[6][i] = createPiece("white-pawn");
    }
    board[0][0] = createPiece("black-rook");
    board[0][1] = createPiece("black-knight");
    board[0][2] = createPiece("black-bishop");
    board[0][3] = createPiece("black-queen");
    board[0][4] = createPiece("black-king");
    board[0][5] = createPiece("black-bishop");
    board[0][6] = createPiece("black-knight");
    board[0][7] = createPiece("black-rook");
    board[7][0] = createPiece("white-rook");
    board[7][1] = createPiece("white-knight");
    board[7][2] = createPiece("white-bishop");
    board[7][3] = createPiece("white-queen");
    board[7][4] = createPiece("white-king");
    board[7][5] = createPiece("white-bishop");
    board[7][6] = createPiece("white-knight");
    board[7][7] = createPiece("white-rook");
}

void moveWhitePawn(int row, int col) {
    if (!selectedPiece || selectedPiece->type != "white-pawn" || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;

    int dy = row - startRow;
    int dx = col - startCol;

    bool moved = false;

    // Forward move
    if (dx == 0) {
        if (dy == -1 && board[row][col] == nullptr) {
            moved = true;
        }
        else if (dy == -2 && startRow == 6 && board[row][col] == nullptr && board[startRow - 1][col] == nullptr) {
            moved = true;
        }
    }
    // Capture move
    else if (abs(dx) == 1 && dy == -1 && board[row][col] != nullptr && !board[row][col]->isWhite) {
        moved = true;
    }

    if (moved) {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
    } else {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
        selectedPiece = nullptr;
        selectedPos = sf::Vector2i(-1, -1);
    }
}

void moveBlackPawn(int row, int col) {
    if (!selectedPiece || selectedPiece->type != "black-pawn" || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;

    int dy = row - startRow;
    int dx = col - startCol;

    bool moved = false;

    // Forward move
    if (dx == 0) {
        if (dy == 1 && board[row][col] == nullptr) {
            moved = true;
        } else if (dy == 2 && startRow == 1 && board[row][col] == nullptr && board[startRow + 1][col] == nullptr) {
            moved = true;
        }
    }
    // Capture move
    else if (abs(dx) == 1 && dy == 1 && board[row][col] != nullptr && board[row][col]->isWhite) {
        moved = true;
    }

    if (moved) {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
    } else {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
        selectedPiece = nullptr;
        selectedPos = sf::Vector2i(-1, -1);
    }
}

void moveRook(int row, int col) {
    if (!selectedPiece || selectedPiece->type.find("rook") == std::string::npos || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;

    if (startRow != row && startCol != col) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else if (!isPathClear(startRow, startCol, row, col) || (board[row][col] && board[row][col]->isWhite == selectedPiece->isWhite)) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
        return;
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
}

void moveBishop(int row, int col) {
    if (!selectedPiece || selectedPiece->type.find("bishop") == std::string::npos || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;
    if (abs(row - startRow) != abs(col - startCol)) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else if (!isPathClear(startRow, startCol, row, col) || (board[row][col] && board[row][col]->isWhite == selectedPiece->isWhite)) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
        return;
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
}

void moveKnight(int row, int col) {
    if (!selectedPiece || selectedPiece->type.find("knight") == std::string::npos || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;
    int dr = abs(row - startRow);
    int dc = abs(col - startCol);

    if (!((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) || (board[row][col] && board[row][col]->isWhite == selectedPiece->isWhite)) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
        return;
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
}

void moveQueen(int row, int col) {
    if (!selectedPiece || selectedPiece->type.find("queen") == std::string::npos || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;

    bool straight = (startRow == row || startCol == col);
    bool diagonal = abs(row - startRow) == abs(col - startCol);

    if (!straight && !diagonal) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else if (!isPathClear(startRow, startCol, row, col) || (board[row][col] && board[row][col]->isWhite == selectedPiece->isWhite)) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
        return;
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
}

void moveKing(int row, int col) {
    if (!selectedPiece || selectedPiece->type.find("king") == std::string::npos || !isInsideBoard(row, col)) {
        std::cout << "Invalid move attempt.\n";
        return;
    }

    int startRow = selectedPos.x;
    int startCol = selectedPos.y;

    int dr = abs(row - startRow);
    int dc = abs(col - startCol);

    if ((dr > 1 || dc > 1) || (board[row][col] && board[row][col]->isWhite == selectedPiece->isWhite)) {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    } else {
        if (!wouldLeaveInCheck(startRow, startCol, row, col)) {
            finalizeMove(startRow, startCol, row, col);
        } else {
            std::cout << "Move would leave king in check\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
        return;
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
}

void movePiece(int row, int col, sf::RenderWindow& window) {
    if (!isInsideBoard(row, col)) return;

    if (!selectedPiece) {
        if (board[row][col] != nullptr && board[row][col]->isWhite == isWhiteTurn) {
            selectedPiece = board[row][col];
            selectedPos = sf::Vector2i(row, col);
        }
    } else {
        if (selectedPiece->type == "white-pawn") {
            moveWhitePawn(row, col);
        } else if (selectedPiece->type == "black-pawn") {
            moveBlackPawn(row, col);
        } else if (selectedPiece->type.find("rook") != std::string::npos) {
            moveRook(row, col);
        } else if (selectedPiece->type.find("knight") != std::string::npos) {
            moveKnight(row, col);
        } else if (selectedPiece->type.find("bishop") != std::string::npos) {
            moveBishop(row, col);
        } else if (selectedPiece->type.find("queen") != std::string::npos) {
            moveQueen(row, col);
        } else if (selectedPiece->type.find("king") != std::string::npos) {
            moveKing(row, col);
        } else {
            std::cout << "Unknown piece type.\n";
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
        }
    }
}

#ifndef UNIT_TEST
int main() {
    sf::RenderWindow window(sf::VideoMode(800, 800), "C++ Chess");
    loadTextures();
    default_board();
    std::cout << "Program started" << std::endl;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                int col = mousePos.x / TILE_SIZE;
                int row = mousePos.y / TILE_SIZE;
                movePiece(row, col, window);
            }
        }

        window.clear();
        drawBoard(window);
        drawPieces(window);
        window.display();
    }

    for (int row = 0; row < 8; ++row)
        for (int col = 0; col < 8; ++col)
            delete board[row][col];

    return 0;
}
#endif
