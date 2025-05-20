#include <SFML/Graphics.hpp>
#include <map>
#include <iostream>
const int TILE_SIZE = 100;
const int BOARD_SIZE = 8;


//move pieces code
sf::Vector2i selectedSquare(-1, -1); // (-1, -1) = nothing selected
bool isSelecting = false;

extern std::string board[8][8]; // assuming 2D board


std::string board[8][8] = {
    "black-rook", "black-knight", "black-bishop", "black-queen", "black-king", "black-bishop", "black-knight", "black-rook",
    "black-pawn", "black-pawn", "black-pawn", "black-pawn", "black-pawn", "black-pawn", "black-pawn", "black-pawn",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "white-pawn", "white-pawn", "white-pawn", "white-pawn", "white-pawn", "white-pawn", "white-pawn", "white-pawn",
    "white-rook", "white-knight", "white-bishop", "white-queen", "white-king", "white-bishop", "white-knight", "white-rook"
};


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


	void drawPieces(sf::RenderWindow& window, std::map<std::string, sf::Texture>& textures) {
		const float tileSize = 100.0f;
		const float textureSize = 128.0f; // assuming 256x256 PNGs
	
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 8; ++col) {
                std::string piece = board[row][col];
				if (piece != "") {
					sf::Sprite sprite;
					sprite.setTexture(textures[piece]);
	
					// Scale up the sprite to fit the tile
					float scale = tileSize / textureSize;
					sprite.setScale(scale, scale);
	
					// Compute offset to center the piece
					float offset = (tileSize - textureSize * scale) / 2.0f;
					sprite.setPosition(col * tileSize + offset, row * tileSize + offset);
	
					window.draw(sprite);
				}
			}
		}
	}
	
int main() {
    sf::RenderWindow window(sf::VideoMode(800, 800), "C++ Chess");
    std::map<std::string, sf::Texture> textures;
    loadTextures(textures);
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
		window.clear();
		drawBoard(window);
		drawPieces(window, textures);

        window.display();
    }

    return 0;
}
