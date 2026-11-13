#include <SFML/Graphics.hpp>
#include "MaxHeap.h"
#include <optional>
#include <iostream>

using namespace VaultHeist;

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1024, 768 }), "Vault Heist - Act 4: The Heap Fortress");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "Could not load font\n";
    }

    MaxHeap heap;
    int initialVault[] = { 45, 12, 85, 32, 89, 39, 67, 93 };
    heap.buildMaxHeap(initialVault, 8);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        window.clear(sf::Color(15, 20, 35));
        window.display();
    }

    return 0;
}
