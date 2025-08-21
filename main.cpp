#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <iostream>
#include <cctype>
#include <vector>
#include <random>
#include <utility>
#include <cstdlib>
#include <cstdio>
#include <optional>
#include <sstream>

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

enum class GameState { MENU, SETTINGS, PLAYING };
GameState gameState = GameState::MENU;
bool aiEnabled = false;

struct AIMove { int sr, sc, er, ec; int score; };

std::string boardToSimpleString() {
    std::ostringstream oss;
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c]) {
                oss << board[r][c]->type << ' ';
            } else {
                oss << "-- ";
            }
        }
        if (r != BOARD_SIZE - 1) oss << '/';
    }
    return oss.str();
}

std::optional<AIMove> requestAIMoveFromChatGPT() {
    const char* apiKey = std::getenv("OPENAI_API_KEY");
    if (!apiKey) return std::nullopt;

    std::string boardState = boardToSimpleString();
    std::string cmd = "python3 chatgpt_move.py \"" + boardState + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return std::nullopt;
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    std::istringstream iss(result);
    AIMove m;
    if (iss >> m.sr >> m.sc >> m.er >> m.ec) {
        m.score = 0;
        return m;
    }
    return std::nullopt;
}

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

int pieceValue(const std::string& type) {
    if (type.find("pawn") != std::string::npos) return 1;
    if (type.find("knight") != std::string::npos) return 3;
    if (type.find("bishop") != std::string::npos) return 3;
    if (type.find("rook") != std::string::npos) return 5;
    if (type.find("queen") != std::string::npos) return 9;
    return 0;
}

bool isValidMove(Piece* p, int sr, int sc, int er, int ec) {
    if (!p || p->isWhite) return false;
    if (!isInsideBoard(er, ec)) return false;
    if (board[er][ec] && board[er][ec]->isWhite == p->isWhite) return false;
    if (board[er][ec] && board[er][ec]->type.find("king") != std::string::npos)
        return false;

    int dr = er - sr;
    int dc = ec - sc;

    if (p->type == "black-pawn") {
        if (dc == 0) {
            if (dr == 1 && board[er][ec] == nullptr) {
            } else if (dr == 2 && sr == 1 && board[er][ec] == nullptr && board[sr + 1][sc] == nullptr) {
            } else {
                return false;
            }
        } else if (abs(dc) == 1 && dr == 1 && board[er][ec] && board[er][ec]->isWhite) {
        } else {
            return false;
        }
    } else if (p->type.find("rook") != std::string::npos) {
        if (sr != er && sc != ec) return false;
        if (!isPathClear(sr, sc, er, ec)) return false;
    } else if (p->type.find("bishop") != std::string::npos) {
        if (abs(dr) != abs(dc)) return false;
        if (!isPathClear(sr, sc, er, ec)) return false;
    } else if (p->type.find("queen") != std::string::npos) {
        if (sr != er && sc != ec && abs(dr) != abs(dc)) return false;
        if (!isPathClear(sr, sc, er, ec)) return false;
    } else if (p->type.find("knight") != std::string::npos) {
        if (!((abs(dr) == 2 && abs(dc) == 1) || (abs(dr) == 1 && abs(dc) == 2)))
            return false;
    } else if (p->type.find("king") != std::string::npos) {
        if (abs(dr) > 1 || abs(dc) > 1) return false;
        if (isSquareAttacked(er, ec, true)) return false;
    } else {
        return false;
    }

    if (wouldLeaveInCheck(sr, sc, er, ec)) return false;
    return true;
}

std::vector<AIMove> generateLegalMovesForBlack() {
    std::vector<AIMove> moves;
    for (int sr = 0; sr < BOARD_SIZE; ++sr) {
        for (int sc = 0; sc < BOARD_SIZE; ++sc) {
            Piece* p = board[sr][sc];
            if (!p || p->isWhite) continue;
            for (int er = 0; er < BOARD_SIZE; ++er) {
                for (int ec = 0; ec < BOARD_SIZE; ++ec) {
                    if (isValidMove(p, sr, sc, er, ec)) {
                        int score = board[er][ec] ? pieceValue(board[er][ec]->type) : 0;
                        moves.push_back({sr, sc, er, ec, score});
                    }
                }
            }
        }
    }
    return moves;
}

void promotePawn(Piece* pawn) {
    std::string color = pawn->isWhite ? "white" : "black";
    std::string choice = "queen";
#ifdef UNIT_TEST
    // In tests, automatically promote to a queen
#else
    sf::RenderWindow promo(sf::VideoMode(TILE_SIZE * 4, TILE_SIZE), "Promote Pawn");
    sf::Sprite options[4];
    std::string names[4] = {"queen", "rook", "bishop", "knight"};
    for (int i = 0; i < 4; ++i) {
        options[i].setTexture(textures[color + "-" + names[i]]);
        options[i].setScale(TILE_SIZE / 128.0f, TILE_SIZE / 128.0f);
        float offset = (TILE_SIZE - 128.0f * options[i].getScale().x) / 2.0f;
        options[i].setPosition(i * TILE_SIZE + offset, offset);
    }
    while (promo.isOpen()) {
        sf::Event event;
        while (promo.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                promo.close();
            }
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                int col = event.mouseButton.x / TILE_SIZE;
                if (col >= 0 && col < 4) {
                    choice = names[col];
                    promo.close();
                }
            }
        }
        promo.clear(sf::Color(200, 200, 200));
        for (int i = 0; i < 4; ++i) promo.draw(options[i]);
        promo.display();
    }
#endif
    pawn->sprite.setTexture(textures[color + "-" + choice]);
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

void clearBoard() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            delete board[r][c];
            board[r][c] = nullptr;
        }
    }
}

void default_board() {
    clearBoard();
    isWhiteTurn = true;
    selectedPiece = nullptr;
    selectedPos = sf::Vector2i(-1, -1);

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

void aiMove(sf::RenderWindow& window) {
    if (isWhiteTurn) return;

    if (auto move = requestAIMoveFromChatGPT()) {
        if (isInsideBoard(move->sr, move->sc) && isInsideBoard(move->er, move->ec)) {
            Piece* p = board[move->sr][move->sc];
            if (isValidMove(p, move->sr, move->sc, move->er, move->ec)) {
                bool turnBefore = isWhiteTurn;
                selectedPiece = p;
                selectedPos = sf::Vector2i(move->sr, move->sc);
                if (p->type == "black-pawn") moveBlackPawn(move->er, move->ec);
                else if (p->type.find("rook") != std::string::npos) moveRook(move->er, move->ec);
                else if (p->type.find("knight") != std::string::npos) moveKnight(move->er, move->ec);
                else if (p->type.find("bishop") != std::string::npos) moveBishop(move->er, move->ec);
                else if (p->type.find("queen") != std::string::npos) moveQueen(move->er, move->ec);
                else if (p->type.find("king") != std::string::npos) moveKing(move->er, move->ec);
                if (isWhiteTurn != turnBefore) return;
            }
        }
    }

    auto moves = generateLegalMovesForBlack();
    if (moves.empty()) return;

    int bestScore = 0;
    for (const auto& m : moves) {
        if (m.score > bestScore) bestScore = m.score;
    }
    std::vector<AIMove> candidates;
    for (const auto& m : moves) {
        if (m.score == bestScore) candidates.push_back(m);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, candidates.size() - 1);
    AIMove choice = candidates[dist(gen)];
    Piece* p = board[choice.sr][choice.sc];
    selectedPiece = p;
    selectedPos = sf::Vector2i(choice.sr, choice.sc);
    if (p->type == "black-pawn") moveBlackPawn(choice.er, choice.ec);
    else if (p->type.find("rook") != std::string::npos) moveRook(choice.er, choice.ec);
    else if (p->type.find("knight") != std::string::npos) moveKnight(choice.er, choice.ec);
    else if (p->type.find("bishop") != std::string::npos) moveBishop(choice.er, choice.ec);
    else if (p->type.find("queen") != std::string::npos) moveQueen(choice.er, choice.ec);
    else if (p->type.find("king") != std::string::npos) moveKing(choice.er, choice.ec);
}

void drawMenu(sf::RenderWindow& window) {
    static sf::Font font;
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "Failed to load font\n";
        return;
    }
    sf::RectangleShape pvp(sf::Vector2f(200, 50));
    pvp.setPosition(300, 200);
    pvp.setFillColor(sf::Color(100, 149, 237)); // cornflower blue
    sf::Text pvpText("Play", font, 24);
    pvpText.setFillColor(sf::Color::White);
    pvpText.setPosition(370, 210);

    sf::RectangleShape ai(sf::Vector2f(200, 50));
    ai.setPosition(300, 270);
    ai.setFillColor(sf::Color(60, 179, 113)); // medium sea green
    sf::Text aiText("Play vs AI", font, 24);
    aiText.setFillColor(sf::Color::White);
    aiText.setPosition(335, 280);

    sf::RectangleShape settings(sf::Vector2f(200, 50));
    settings.setPosition(300, 340);
    settings.setFillColor(sf::Color(238, 232, 170)); // pale goldenrod
    sf::Text settingsText("Settings", font, 24);
    settingsText.setFillColor(sf::Color::Black);
    settingsText.setPosition(350, 350);

    sf::RectangleShape exit(sf::Vector2f(200, 50));
    exit.setPosition(300, 410);
    exit.setFillColor(sf::Color(205, 92, 92)); // indian red
    sf::Text exitText("Exit", font, 24);
    exitText.setFillColor(sf::Color::White);
    exitText.setPosition(370, 420);

    window.draw(pvp);
    window.draw(ai);
    window.draw(settings);
    window.draw(exit);
    window.draw(pvpText);
    window.draw(aiText);
    window.draw(settingsText);
    window.draw(exitText);
}

void handleMenuClick(sf::Vector2i mousePos, sf::RenderWindow& window) {
    sf::FloatRect pvpRect(300, 200, 200, 50);
    sf::FloatRect aiRect(300, 270, 200, 50);
    sf::FloatRect settingsRect(300, 340, 200, 50);
    sf::FloatRect exitRect(300, 410, 200, 50);
    if (pvpRect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
        aiEnabled = false;
        default_board();
        gameState = GameState::PLAYING;
    } else if (aiRect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
        aiEnabled = true;
        default_board();
        gameState = GameState::PLAYING;
    } else if (settingsRect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
        gameState = GameState::SETTINGS;
    } else if (exitRect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
        window.close();
    }
}

void drawSettings(sf::RenderWindow& window) {
    static sf::Font font;
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "Failed to load font\n";
        return;
    }
    sf::Text text("Settings - Click to return", font, 24);
    text.setPosition(180, 300);
    window.draw(text);
}

#ifndef UNIT_TEST
int main() {
    sf::RenderWindow window(sf::VideoMode(800, 800), "C++ Chess");
    loadTextures();
    std::cout << "Program started" << std::endl;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (gameState == GameState::MENU) {
                    handleMenuClick(mousePos, window);
                } else if (gameState == GameState::PLAYING) {
                    int col = mousePos.x / TILE_SIZE;
                    int row = mousePos.y / TILE_SIZE;
                    movePiece(row, col, window);
                } else if (gameState == GameState::SETTINGS) {
                    gameState = GameState::MENU;
                }
            }
        }

        if (gameState == GameState::PLAYING && aiEnabled && !isWhiteTurn) {
            aiMove(window);
        }

        window.clear(gameState == GameState::MENU ? sf::Color(50, 50, 50) : sf::Color::Black);
        if (gameState == GameState::MENU) {
            drawMenu(window);
        } else if (gameState == GameState::PLAYING) {
            drawBoard(window);
            drawPieces(window);
        } else if (gameState == GameState::SETTINGS) {
            drawSettings(window);
        }
        window.display();
    }

    clearBoard();
    return 0;
}
#endif
