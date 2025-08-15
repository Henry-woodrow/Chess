#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <iostream>

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

void finalizeMove(int startRow, int startCol, int row, int col) {
    if (board[row][col] != nullptr) delete board[row][col];
    board[row][col] = selectedPiece;
    board[startRow][startCol] = nullptr;
    std::cout << "Moved piece: " << selectedPiece->type << " to (" << col << ", " << row << ")\n";
    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
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

Piece* createPiece(std::string name, std::map<std::string, sf::Texture>& textures) {
    Piece* piece = new Piece;
    piece->sprite.setTexture(textures[name]);
    piece->sprite.setScale(TILE_SIZE / 128.0f, TILE_SIZE / 128.0f);
    piece->type = name;
    piece->isWhite = name.find("white") != std::string::npos;
    return piece;
}

void loadTextures(std::map<std::string, sf::Texture>& textures) {
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

void default_board(std::map<std::string, sf::Texture>& textures) {
    for (int i = 0; i < 8; ++i) {
        board[1][i] = createPiece("black-pawn", textures);
        board[6][i] = createPiece("white-pawn", textures);
    }
    board[0][0] = createPiece("black-rook", textures);
    board[0][1] = createPiece("black-knight", textures);
    board[0][2] = createPiece("black-bishop", textures);
    board[0][3] = createPiece("black-queen", textures);
    board[0][4] = createPiece("black-king", textures);
    board[0][5] = createPiece("black-bishop", textures);
    board[0][6] = createPiece("black-knight", textures);
    board[0][7] = createPiece("black-rook", textures);
    board[7][0] = createPiece("white-rook", textures);
    board[7][1] = createPiece("white-knight", textures);
    board[7][2] = createPiece("white-bishop", textures);
    board[7][3] = createPiece("white-queen", textures);
    board[7][4] = createPiece("white-king", textures);
    board[7][5] = createPiece("white-bishop", textures);
    board[7][6] = createPiece("white-knight", textures);
    board[7][7] = createPiece("white-rook", textures);
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
        delete board[row][col]; // capture
        moved = true;
    }

    if (moved) {
        board[row][col] = selectedPiece;
        board[startRow][startCol] = nullptr;
        std::cout << "Moved piece: " << selectedPiece->type << " to (" << col << ", " << row << ")\n";
    } else {
        std::cout << "Invalid move for piece: " << selectedPiece->type << "\n";
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
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
        delete board[row][col];
        moved = true;
    }

    if (moved) {
        finalizeMove(startRow, startCol, row, col);
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
        finalizeMove(startRow, startCol, row, col);
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
        finalizeMove(startRow, startCol, row, col);
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
        finalizeMove(startRow, startCol, row, col);
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
        finalizeMove(startRow, startCol, row, col);
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
        finalizeMove(startRow, startCol, row, col);
        return;
    }

    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);
}

void movePiece(int row, int col, sf::RenderWindow& window) {
    if (!isInsideBoard(row, col)) return;

    if (!selectedPiece) {
        if (board[row][col] != nullptr) {
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
    std::map<std::string, sf::Texture> textures;
    loadTextures(textures);
    default_board(textures);
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
