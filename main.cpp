#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <iostream>
const int TILE_SIZE = 100;
const int BOARD_SIZE = 8;

struct Piece {
    sf::Sprite sprite;
    std::string type; // e.g., 'P' for pawn, 'K' for king, etc.
    bool isWhite;
};

Piece* selectedPiece = nullptr; // Pointer to the selected piece
sf::Vector2i selectedPos;
sf::Vector2i selectedSquare(-1, -1); // (-1, -1) = nothing selected
bool isSelecting = false;

// saving the board as an object  instead of just a string to give me flexibility in the future
// This will allow me to add more functionality later, such as piece movement, capturing, etc.
// Each Piece pointer will point to a Piece object, or nullptr if the square is empty

Piece* board[8][8] = {nullptr};




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
    piece->type = name.back(); // crude type detection
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
        // Initialize empty squares
        
        for (int row = 2; row < 6; ++row) {
            for (int col = 0; col < 8; ++col) {
                board[row][col] = nullptr;
            }
        }
    }

void movePiece(sf::RenderWindow& window) {
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    int col = mousePos.x / TILE_SIZE;
    int row = mousePos.y / TILE_SIZE;
    if (!selectedPiece)
    {
        if(board[row][col] != nullptr){
            selectedPiece = board[row][col];
            selectedPos = sf::Vector2i(row, col);
        }                
    }else
    {
        if (board[row][col] == nullptr)
        {
            board[row][col] = selectedPiece;
            board[selectedPos.x][selectedPos.y] = nullptr;
            std::cout << "Moved piece: " << selectedPiece->type << " to (" << col << ", " << row << ")\n";

            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);

        }else
        {
            // If the square is occupied, deselect the piece
            selectedPiece = nullptr;
            selectedPos = sf::Vector2i(-1, -1);
            std::cout << "Deselected piece at (" << col << ", " << row << ")\n";
        }
        
    }
}
	
int main() {
    sf::RenderWindow window(sf::VideoMode(800, 800), "C++ Chess");
    std::map<std::string, sf::Texture> textures;
    loadTextures(textures);
    default_board(textures);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                movePiece(window);
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

