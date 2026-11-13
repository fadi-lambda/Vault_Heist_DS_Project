#include <SFML/Graphics.hpp>
#include "MaxHeap.h"
#include "AVLTree.h"
#include <optional>
#include <iostream>
#include <string>
#include <cmath>

using namespace VaultHeist;

// Simple struct for 2D screen coordinates
struct Point {
    float x;
    float y;
};

// Calculates screen coordinates for tree nodes in a complete binary tree
Point getHeapNodePosition(int index, int totalSize, float screenWidth, float startY) {
    int level = static_cast<int>(std::floor(std::log2(index + 1)));
    int nodesAtLevel = 1 << level;
    int indexAtLevel = index - (nodesAtLevel - 1);

    float levelSpacing = 65.0f;
    float y = startY + level * levelSpacing;

    float sectionWidth = screenWidth / (nodesAtLevel + 1);
    float x = sectionWidth * (indexAtLevel + 1);

    return { x, y };
}

int main() {
    constexpr unsigned int WINDOW_WIDTH = 1050;
    constexpr unsigned int WINDOW_HEIGHT = 760;

    sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "Vault Heist - The Living Vault");
    window.setFramerateLimit(60);

    // Multi-path font fallback to prevent any missing font crash
    sf::Font font;
    bool fontLoaded = font.openFromFile("assets/fonts/arial.ttf") ||
                      font.openFromFile("../../assets/fonts/arial.ttf") ||
                      font.openFromFile("out/build/x64-debug/assets/fonts/arial.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");

    if (!fontLoaded) {
        std::cerr << "Warning: Could not load font from any known path!\n";
    }

    // --- GAME STATE FOR ACT 4 (HEAP FORTRESS) ---
    MaxHeap heap(30);

    // Initial Vault treasures
    int initialVault[] = { 45, 12, 85, 32, 89, 39, 67, 93, 20, 58 };
    int initialCount = 10;
    heap.buildMaxHeap(initialVault, initialCount);

    // Sorted region tracking (Section 6.2 requirement)
    int sortedVault[30];
    int sortedCount = 0;

    // Alarm meter and scoring (Section 2.2 requirement)
    int alarmSteps = 0;
    int alarmCapacity = 25; // Capacity based on level limit
    int score = 1000;
    std::string statusMessage = "Press [E] to Extract Max, [I] to Insert, [S] to Heap Sort, [R] to Reset";

    // View toggle: 0 = Act 4 (Max Heap), 1 = Act 3 (AVL Tree)
    int currentView = 0;

    // AVL Tree for Act 3 preview
    AVLTree avl;
    int avlInitial[] = { 40, 20, 60, 10, 30, 50, 70 };
    for (int v : avlInitial) avl.insert(v);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }

                // Switch between Act 4 (Heap) and Act 3 (AVL)
                if (keyPressed->code == sf::Keyboard::Key::Tab) {
                    currentView = (currentView == 0) ? 1 : 0;
                    statusMessage = (currentView == 0) ? "Switched to Act 4: The Heap Fortress" : "Switched to Act 3: AVL Architect's Wing";
                }

                if (currentView == 0) {
                    // --- ACT 4 HEAP CONTROLS ---

                    // [E] Extract Maximum
                    if (keyPressed->code == sf::Keyboard::Key::E) {
                        if (!heap.isEmpty()) {
                            int maxVal = heap.extractMax();
                            if (sortedCount < 30) {
                                sortedVault[sortedCount++] = maxVal;
                            }
                            alarmSteps++;
                            score = std::max(0, 1000 - (alarmSteps * 15));
                            statusMessage = "Extracted Max Treasure: " + std::to_string(maxVal) + " -> Moved to Sorted Vault!";
                        } else {
                            statusMessage = "All treasures extracted! Vault is unlocked!";
                        }
                    }

                    // [I] Insert New Treasure
                    if (keyPressed->code == sf::Keyboard::Key::I) {
                        if (heap.getSize() < 15) {
                            int newLoot = (rand() % 90) + 10;
                            heap.insert(newLoot);
                            alarmSteps++;
                            score = std::max(0, 1000 - (alarmSteps * 15));
                            statusMessage = "Guards heard noise! Stashed new loot: " + std::to_string(newLoot);
                        } else {
                            statusMessage = "Vault room capacity reached!";
                        }
                    }

                    // [S] Heap Sort (Step)
                    if (keyPressed->code == sf::Keyboard::Key::S) {
                        if (!heap.isEmpty()) {
                            int maxVal = heap.extractMax();
                            if (sortedCount < 30) {
                                sortedVault[sortedCount++] = maxVal;
                            }
                            alarmSteps++;
                            statusMessage = "Heap Sort Step: Extracted " + std::to_string(maxVal);
                        } else {
                            statusMessage = "Heap Sort Complete! Final Vault Unlocked!";
                        }
                    }

                    // [R] Reset Level
                    if (keyPressed->code == sf::Keyboard::Key::R) {
                        heap.buildMaxHeap(initialVault, initialCount);
                        sortedCount = 0;
                        alarmSteps = 0;
                        score = 1000;
                        statusMessage = "Vault reset! Build Max Heap ready.";
                    }
                } else {
                    // --- ACT 3 AVL CONTROLS ---
                    if (keyPressed->code == sf::Keyboard::Key::I) {
                        int newKey = (rand() % 89) + 11;
                        if (avl.insert(newKey)) {
                            statusMessage = "AVL Inserted: " + std::to_string(newKey) + " (Balanced!)";
                        } else {
                            statusMessage = "Duplicate rejected!";
                        }
                    }
                    if (keyPressed->code == sf::Keyboard::Key::R) {
                        statusMessage = "AVL Root Balance Factor: " + std::to_string(avl.getRootBalanceFactor());
                    }
                }
            }
        }

        // ================= RENDER =================
        window.clear(sf::Color(18, 22, 36)); // Dark slate fantasy theme

        // 1. TOP HEADER & HUD
        sf::Text headerText(font, (currentView == 0 ? "ACT 4: THE HEAP FORTRESS" : "ACT 3: THE ARCHITECT'S WING (AVL)"), 24);
        headerText.setFillColor(sf::Color(255, 215, 0)); // Gold
        headerText.setPosition({ 30.f, 20.f });
        window.draw(headerText);

        sf::Text subHeaderText(font, (currentView == 0 ? "Priority Security Mechanism: Complete Binary Tree & Synchronised Array" : "Self-Balancing Search Vault: Height O(log n)"), 14);
        subHeaderText.setFillColor(sf::Color(160, 180, 210));
        subHeaderText.setPosition({ 30.f, 55.f });
        window.draw(subHeaderText);

        // 2. ALARM METER BAR (Section 2.2)
        float alarmBarX = 720.f;
        float alarmBarY = 25.f;
        float alarmBarWidth = 280.f;
        float alarmBarHeight = 18.f;

        // Background
        sf::RectangleShape alarmBg({ alarmBarWidth, alarmBarHeight });
        alarmBg.setPosition({ alarmBarX, alarmBarY });
        alarmBg.setFillColor(sf::Color(40, 45, 60));
        alarmBg.setOutlineThickness(1.5f);
        alarmBg.setOutlineColor(sf::Color(100, 110, 130));
        window.draw(alarmBg);

        // Filled alarm
        float alarmRatio = std::min(1.0f, static_cast<float>(alarmSteps) / static_cast<float>(alarmCapacity));
        sf::RectangleShape alarmFill({ alarmBarWidth * alarmRatio, alarmBarHeight });
        alarmFill.setPosition({ alarmBarX, alarmBarY });
        alarmFill.setFillColor(alarmRatio > 0.8f ? sf::Color(230, 50, 50) : sf::Color(240, 160, 30));
        window.draw(alarmFill);

        sf::Text alarmLabel(font, "Alarm: " + std::to_string(alarmSteps) + " / " + std::to_string(alarmCapacity) + "  |  Score: " + std::to_string(score), 13);
        alarmLabel.setFillColor(sf::Color::White);
        alarmLabel.setPosition({ alarmBarX, alarmBarY + 24.f });
        window.draw(alarmLabel);

        // 3. TREE VIEW DRAWING (Section 6.2)
        if (currentView == 0) {
            int heapSize = heap.getSize();
            float treeStartY = 110.f;
            float nodeRadius = 22.f;

            // Draw connecting lines first
            for (int i = 0; i < heapSize; ++i) {
                Point pPos = getHeapNodePosition(i, heapSize, WINDOW_WIDTH, treeStartY);

                int leftChild = 2 * i + 1;
                int rightChild = 2 * i + 2;

                if (leftChild < heapSize) {
                    Point cPos = getHeapNodePosition(leftChild, heapSize, WINDOW_WIDTH, treeStartY);
                    sf::Vertex line[] = {
                        sf::Vertex({ pPos.x, pPos.y }, sf::Color(80, 110, 150)),
                        sf::Vertex({ cPos.x, cPos.y }, sf::Color(80, 110, 150))
                    };
                    window.draw(line, 2, sf::PrimitiveType::Lines);
                }

                if (rightChild < heapSize) {
                    Point cPos = getHeapNodePosition(rightChild, heapSize, WINDOW_WIDTH, treeStartY);
                    sf::Vertex line[] = {
                        sf::Vertex({ pPos.x, pPos.y }, sf::Color(80, 110, 150)),
                        sf::Vertex({ cPos.x, cPos.y }, sf::Color(80, 110, 150))
                    };
                    window.draw(line, 2, sf::PrimitiveType::Lines);
                }
            }

            // Draw nodes
            for (int i = 0; i < heapSize; ++i) {
                Point pos = getHeapNodePosition(i, heapSize, WINDOW_WIDTH, treeStartY);
                int val = heap.getElementAt(i);

                sf::CircleShape node(nodeRadius);
                node.setOrigin({ nodeRadius, nodeRadius });
                node.setPosition({ pos.x, pos.y });

                if (i == 0) {
                    // Root is maximum treasure: Highlighted Gold
                    node.setFillColor(sf::Color(220, 175, 40));
                    node.setOutlineColor(sf::Color(255, 235, 120));
                    node.setOutlineThickness(3.0f);
                } else {
                    node.setFillColor(sf::Color(35, 55, 90));
                    node.setOutlineColor(sf::Color(70, 130, 200));
                    node.setOutlineThickness(2.0f);
                }
                window.draw(node);

                // Number text
                sf::Text valText(font, std::to_string(val), 16);
                valText.setFillColor(i == 0 ? sf::Color::Black : sf::Color::White);
                auto bounds = valText.getLocalBounds();
                valText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
                valText.setPosition({ pos.x, pos.y });
                window.draw(valText);

                // Index badge below
                sf::Text idxText(font, "[" + std::to_string(i) + "]", 11);
                idxText.setFillColor(sf::Color(140, 160, 190));
                auto idxBounds = idxText.getLocalBounds();
                idxText.setOrigin({ idxBounds.position.x + idxBounds.size.x / 2.0f, idxBounds.position.y });
                idxText.setPosition({ pos.x, pos.y + nodeRadius + 3.0f });
                window.draw(idxText);
            }

            // 4. SYNCHRONISED ARRAY VIEW (Section 6.2)
            float arrayStartY = 460.f;
            sf::Text arrayTitle(font, "SYNCHRONISED ARRAY VIEW (ACTIVE HEAP & SORTED REGION)", 14);
            arrayTitle.setFillColor(sf::Color(200, 220, 255));
            arrayTitle.setPosition({ 40.f, arrayStartY - 25.f });
            window.draw(arrayTitle);

            float cellWidth = 55.f;
            float cellHeight = 45.f;
            float cellSpacing = 8.f;
            float startX = 40.f;

            // Draw Active Heap Array cells
            for (int i = 0; i < heapSize; ++i) {
                float cx = startX + i * (cellWidth + cellSpacing);
                float cy = arrayStartY;

                sf::RectangleShape cell({ cellWidth, cellHeight });
                cell.setPosition({ cx, cy });
                cell.setFillColor(i == 0 ? sf::Color(60, 50, 20) : sf::Color(25, 40, 65));
                cell.setOutlineThickness(2.0f);
                cell.setOutlineColor(i == 0 ? sf::Color(240, 190, 40) : sf::Color(70, 140, 220));
                window.draw(cell);

                int val = heap.getElementAt(i);
                sf::Text cellVal(font, std::to_string(val), 16);
                cellVal.setFillColor(sf::Color::White);
                auto bounds = cellVal.getLocalBounds();
                cellVal.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
                cellVal.setPosition({ cx + cellWidth / 2.0f, cy + cellHeight / 2.0f });
                window.draw(cellVal);

                sf::Text cellIdx(font, std::to_string(i), 11);
                cellIdx.setFillColor(sf::Color(130, 150, 180));
                cellIdx.setPosition({ cx + 4.f, cy + cellHeight + 3.f });
                window.draw(cellIdx);
            }

            // Draw Sorted Region cells
            float sortedStartX = startX + heapSize * (cellWidth + cellSpacing) + 20.f;
            if (sortedCount > 0) {
                sf::Text sortedLabel(font, "SORTED REGION", 12);
                sortedLabel.setFillColor(sf::Color(80, 220, 120));
                sortedLabel.setPosition({ sortedStartX, arrayStartY - 20.f });
                window.draw(sortedLabel);

                for (int s = 0; s < sortedCount; ++s) {
                    float sx = sortedStartX + s * (cellWidth + cellSpacing);
                    float sy = arrayStartY;

                    sf::RectangleShape cell({ cellWidth, cellHeight });
                    cell.setPosition({ sx, sy });
                    cell.setFillColor(sf::Color(20, 50, 35));
                    cell.setOutlineThickness(2.0f);
                    cell.setOutlineColor(sf::Color(60, 200, 100));
                    window.draw(cell);

                    int sVal = sortedVault[s];
                    sf::Text cellVal(font, std::to_string(sVal), 16);
                    cellVal.setFillColor(sf::Color(180, 255, 200));
                    auto bounds = cellVal.getLocalBounds();
                    cellVal.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
                    cellVal.setPosition({ sx + cellWidth / 2.0f, sy + cellHeight / 2.0f });
                    window.draw(cellVal);
                }
            }
        }

        // 5. STATUS MESSAGE & CONTROLS HUD (Bottom)
        sf::RectangleShape statusBg({ WINDOW_WIDTH - 60.f, 95.f });
        statusBg.setPosition({ 30.f, WINDOW_HEIGHT - 125.f });
        statusBg.setFillColor(sf::Color(25, 30, 48));
        statusBg.setOutlineThickness(1.5f);
        statusBg.setOutlineColor(sf::Color(70, 80, 110));
        window.draw(statusBg);

        sf::Text statusText(font, statusMessage, 15);
        statusText.setFillColor(sf::Color(255, 220, 120));
        statusText.setPosition({ 50.f, WINDOW_HEIGHT - 115.f });
        window.draw(statusText);

        std::string controlsStr = "CONTROLS: [E] Extract Max   |   [I] Insert Treasure   |   [S] Heap Sort Step   |   [R] Reset   |   [TAB] Switch View";
        sf::Text controlsText(font, controlsStr, 13);
        controlsText.setFillColor(sf::Color(170, 190, 220));
        controlsText.setPosition({ 50.f, WINDOW_HEIGHT - 75.f });
        window.draw(controlsText);

        window.display();
    }

    return 0;
}
