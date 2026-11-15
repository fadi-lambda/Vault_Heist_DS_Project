// =============================================================================
// Vault Heist: The Living Vault — Main Game Source
// Acts 3 & 4 (Member 2): AVL Wing (3.1–3.5) + Heap Fortress (4.1–4.5)
// =============================================================================

#include <SFML/Graphics.hpp>
#include "structures/MaxHeap.h"
#include "structures/AVLTree.h"
#include <optional>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace VaultHeist;

// Screen State Constants (no enum allowed)
static const int SCREEN_MAIN_MENU     = 0;
static const int SCREEN_CAMPAIGN_MAP  = 1;
static const int SCREEN_LEVEL_PLAY    = 2;
static const int SCREEN_RESULT        = 3;
static const int SCREEN_SANDBOX       = 4;

// Level ID Constants
static const int LEVEL_3_1 = 31;  // Read the Badges (Identification)
static const int LEVEL_3_2 = 32;  // Manual Repair — Single (LL / RR)
static const int LEVEL_3_3 = 33;  // Manual Repair — Double (LR / RL)
static const int LEVEL_3_4 = 34;  // Auto Mode & Prediction
static const int LEVEL_3_5 = 35;  // Boss: Order-Statistic Heist
static const int LEVEL_4_1 = 41;  // Activate the Vault (Build Heap bottom-up)
static const int LEVEL_4_2 = 42;  // Repair Security Locks (Sift-Up / Sift-Down)
static const int LEVEL_4_3 = 43;  // Extract the Greatest Treasure
static const int LEVEL_4_4 = 44;  // Race Against the Guards
static const int LEVEL_4_5 = 45;  // Final Vault Boss (Heap Sort)

// Structs for Custom / Manual Tree Puzzles
struct ManualNode {
    int key;
    int height;
    int size;
    ManualNode* left;
    ManualNode* right;

    ManualNode(int k, int h = 1, int s = 1)
        : key(k), height(h), size(s), left(nullptr), right(nullptr) {}
};

void deleteManualTree(ManualNode* node) {
    if (!node) return;
    deleteManualTree(node->left);
    deleteManualTree(node->right);
    delete node;
}

void updateManualMetadata(ManualNode* node) {
    if (!node) return;
    updateManualMetadata(node->left);
    updateManualMetadata(node->right);
    int lh = node->left ? node->left->height : 0;
    int rh = node->right ? node->right->height : 0;
    node->height = 1 + (lh > rh ? lh : rh);
    int ls = node->left ? node->left->size : 0;
    int rs = node->right ? node->right->size : 0;
    node->size = 1 + ls + rs;
}

ManualNode* manualRotateRight(ManualNode* y) {
    if (!y || !y->left) return y;
    ManualNode* x = y->left;
    ManualNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    updateManualMetadata(y);
    updateManualMetadata(x);
    return x;
}

ManualNode* manualRotateLeft(ManualNode* x) {
    if (!x || !x->right) return x;
    ManualNode* y = x->right;
    ManualNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    updateManualMetadata(x);
    updateManualMetadata(y);
    return y;
}

struct Point { float x; float y; };

Point getHeapNodePos(int i, float screenW, float startY) {
    int level = static_cast<int>(std::floor(std::log2(i + 1)));
    int nodesAtLvl = 1 << level;
    int idxAtLvl   = i - (nodesAtLvl - 1);
    return { (screenW / (nodesAtLvl + 1)) * (idxAtLvl + 1), startY + level * 58.f };
}

struct LevelInfo {
    int id;
    int act;
    std::string title;
    std::string objective;
    std::string controls;
    int alarmCapacity;
    bool unlocked;
    int stars;

    LevelInfo() : id(0), act(0), alarmCapacity(20), unlocked(false), stars(0) {}
    LevelInfo(int i, int a, std::string t, std::string o, std::string c, int cap, bool u, int s)
        : id(i), act(a), title(t), objective(o), controls(c), alarmCapacity(cap), unlocked(u), stars(s) {}
};

// =============================================================================
// Interactive Button Component
// =============================================================================
bool drawButton(sf::RenderWindow& window, const sf::Font& font,
                const std::string& label, float x, float y, float w, float h,
                sf::Color bgColor = sf::Color(35, 50, 75),
                sf::Color txtColor = sf::Color::White,
                sf::Color outlineColor = sf::Color(80, 130, 200),
                unsigned int charSize = 14)
{
    sf::RectangleShape btn({w, h});
    btn.setPosition({x, y});
    btn.setFillColor(bgColor);
    btn.setOutlineThickness(2.f);
    btn.setOutlineColor(outlineColor);
    window.draw(btn);

    sf::Text txt(font, label, charSize);
    txt.setFillColor(txtColor);
    auto b = txt.getLocalBounds();
    txt.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    txt.setPosition({x + w / 2.f, y + h / 2.f});
    window.draw(txt);

    return false;
}

bool isInside(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

// =============================================================================
// Drawing Functions
// =============================================================================
void drawAVLNode(sf::RenderWindow& window, const sf::Font& font,
                 AVLNode* node, float x, float y, float hSpacing,
                 int highlightKey = -9999)
{
    if (!node) return;
    float r = 21.f, vSp = 64.f;

    if (node->left) {
        float cx = x - hSpacing, cy = y + vSp;
        sf::Vertex line[] = {
            sf::Vertex({x, y}, sf::Color(100, 140, 180)),
            sf::Vertex({cx, cy}, sf::Color(100, 140, 180))
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
        drawAVLNode(window, font, node->left, cx, cy, hSpacing * 0.52f, highlightKey);
    }
    if (node->right) {
        float cx = x + hSpacing, cy = y + vSp;
        sf::Vertex line[] = {
            sf::Vertex({x, y}, sf::Color(100, 140, 180)),
            sf::Vertex({cx, cy}, sf::Color(100, 140, 180))
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
        drawAVLNode(window, font, node->right, cx, cy, hSpacing * 0.52f, highlightKey);
    }

    bool isHl = (node->key == highlightKey);
    sf::CircleShape circle(r);
    circle.setOrigin({r, r});
    circle.setPosition({x, y});
    circle.setFillColor(isHl ? sf::Color(90, 60, 20) : sf::Color(30, 50, 80));
    circle.setOutlineColor(isHl ? sf::Color(255, 215, 50) : sf::Color(90, 160, 240));
    circle.setOutlineThickness(isHl ? 3.5f : 2.5f);
    window.draw(circle);

    sf::Text val(font, std::to_string(node->key), 15);
    val.setFillColor(sf::Color::White);
    auto b = val.getLocalBounds();
    val.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    val.setPosition({x, y});
    window.draw(val);

    // BF badge
    int bf = (node->left ? node->left->height : 0) - (node->right ? node->right->height : 0);
    sf::CircleShape badge(9.f);
    badge.setOrigin({9.f, 9.f});
    badge.setPosition({x + 18.f, y - 18.f});
    badge.setFillColor((bf >= -1 && bf <= 1) ? sf::Color(40, 180, 80) : sf::Color(220, 50, 50));
    badge.setOutlineThickness(1.5f);
    badge.setOutlineColor(sf::Color::White);
    window.draw(badge);

    sf::Text bfTxt(font, std::to_string(bf), 10);
    bfTxt.setFillColor(sf::Color::White);
    auto bb = bfTxt.getLocalBounds();
    bfTxt.setOrigin({bb.position.x + bb.size.x / 2.f, bb.position.y + bb.size.y / 2.f});
    bfTxt.setPosition({x + 18.f, y - 18.f});
    window.draw(bfTxt);

    // Subtree size
    sf::Text szTxt(font, "sz:" + std::to_string(node->size), 10);
    szTxt.setFillColor(sf::Color(170, 190, 220));
    auto sb = szTxt.getLocalBounds();
    szTxt.setOrigin({sb.position.x + sb.size.x / 2.f, sb.position.y});
    szTxt.setPosition({x, y + r + 3.f});
    window.draw(szTxt);
}

void drawManualNode(sf::RenderWindow& window, const sf::Font& font,
                    ManualNode* node, float x, float y, float hSpacing,
                    int highlightKey = -9999)
{
    if (!node) return;
    float r = 21.f, vSp = 64.f;

    if (node->left) {
        float cx = x - hSpacing, cy = y + vSp;
        sf::Vertex line[] = {
            sf::Vertex({x, y}, sf::Color(100, 140, 180)),
            sf::Vertex({cx, cy}, sf::Color(100, 140, 180))
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
        drawManualNode(window, font, node->left, cx, cy, hSpacing * 0.52f, highlightKey);
    }
    if (node->right) {
        float cx = x + hSpacing, cy = y + vSp;
        sf::Vertex line[] = {
            sf::Vertex({x, y}, sf::Color(100, 140, 180)),
            sf::Vertex({cx, cy}, sf::Color(100, 140, 180))
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
        drawManualNode(window, font, node->right, cx, cy, hSpacing * 0.52f, highlightKey);
    }

    int lh = node->left ? node->left->height : 0;
    int rh = node->right ? node->right->height : 0;
    int bf = lh - rh;

    bool isHl = (node->key == highlightKey);
    sf::CircleShape circle(r);
    circle.setOrigin({r, r});
    circle.setPosition({x, y});
    circle.setFillColor(isHl ? sf::Color(90, 65, 20) : sf::Color(30, 50, 80));
    circle.setOutlineColor(isHl ? sf::Color(255, 220, 50) : (bf < -1 || bf > 1 ? sf::Color(240, 80, 80) : sf::Color(90, 160, 240)));
    circle.setOutlineThickness(isHl ? 3.5f : 2.5f);
    window.draw(circle);

    sf::Text val(font, std::to_string(node->key), 15);
    val.setFillColor(sf::Color::White);
    auto b = val.getLocalBounds();
    val.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    val.setPosition({x, y});
    window.draw(val);

    sf::CircleShape badge(9.f);
    badge.setOrigin({9.f, 9.f});
    badge.setPosition({x + 18.f, y - 18.f});
    badge.setFillColor((bf >= -1 && bf <= 1) ? sf::Color(40, 180, 80) : sf::Color(220, 50, 50));
    badge.setOutlineThickness(1.5f);
    badge.setOutlineColor(sf::Color::White);
    window.draw(badge);

    sf::Text bfTxt(font, std::to_string(bf), 10);
    bfTxt.setFillColor(sf::Color::White);
    auto bb = bfTxt.getLocalBounds();
    bfTxt.setOrigin({bb.position.x + bb.size.x / 2.f, bb.position.y + bb.size.y / 2.f});
    bfTxt.setPosition({x + 18.f, y - 18.f});
    window.draw(bfTxt);

    sf::Text szTxt(font, "sz:" + std::to_string(node->size), 10);
    szTxt.setFillColor(sf::Color(170, 190, 220));
    auto sb = szTxt.getLocalBounds();
    szTxt.setOrigin({sb.position.x + sb.size.x / 2.f, sb.position.y});
    szTxt.setPosition({x, y + r + 3.f});
    window.draw(szTxt);
}

void drawHeapCustom(sf::RenderWindow& window, const sf::Font& font,
                    const int* arr, int activeSize, const int* sortedArr, int sortedCount,
                    unsigned int W, int hl1 = -1, int hl2 = -1)
{
    // Draw Tree Edges
    for (int i = 0; i < activeSize; ++i) {
        Point p = getHeapNodePos(i, static_cast<float>(W), 100.f);
        for (int child : {2*i+1, 2*i+2}) {
            if (child < activeSize) {
                Point c = getHeapNodePos(child, static_cast<float>(W), 100.f);
                bool edgeBroken = (arr[i] < arr[child]);
                sf::Vertex line[] = {
                    sf::Vertex({p.x, p.y}, edgeBroken ? sf::Color(220, 70, 70) : sf::Color(80, 110, 150)),
                    sf::Vertex({c.x, c.y}, edgeBroken ? sf::Color(220, 70, 70) : sf::Color(80, 110, 150))
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }
    }

    // Draw Tree Nodes
    for (int i = 0; i < activeSize; ++i) {
        Point p = getHeapNodePos(i, static_cast<float>(W), 100.f);
        bool isHl = (i == hl1 || i == hl2);
        bool violation = (i > 0 && arr[i] > arr[(i-1)/2]);

        sf::CircleShape node(20.f);
        node.setOrigin({20.f, 20.f});
        node.setPosition({p.x, p.y});

        if (isHl) {
            node.setFillColor(sf::Color(100, 70, 20));
            node.setOutlineColor(sf::Color(255, 220, 60));
        } else if (violation) {
            node.setFillColor(sf::Color(80, 25, 25));
            node.setOutlineColor(sf::Color(240, 70, 70));
        } else if (i == 0) {
            node.setFillColor(sf::Color(200, 160, 30));
            node.setOutlineColor(sf::Color(255, 235, 120));
        } else {
            node.setFillColor(sf::Color(35, 55, 90));
            node.setOutlineColor(sf::Color(70, 130, 200));
        }
        node.setOutlineThickness(isHl ? 3.5f : 2.5f);
        window.draw(node);

        sf::Text vt(font, std::to_string(arr[i]), 14);
        vt.setFillColor(i == 0 && !isHl ? sf::Color::Black : sf::Color::White);
        auto b = vt.getLocalBounds();
        vt.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        vt.setPosition({p.x, p.y});
        window.draw(vt);

        sf::Text it(font, "[" + std::to_string(i) + "]", 10);
        it.setFillColor(sf::Color(140, 160, 190));
        auto ib = it.getLocalBounds();
        it.setOrigin({ib.position.x + ib.size.x / 2.f, ib.position.y});
        it.setPosition({p.x, p.y + 22.f});
        window.draw(it);
    }

    // Array View
    sf::Text arrayLbl(font, "SYNCHRONISED ARRAY VIEW  [Active: " + std::to_string(activeSize) + "]", 13);
    arrayLbl.setFillColor(sf::Color(200, 220, 255));
    arrayLbl.setPosition({40.f, 320.f});
    window.draw(arrayLbl);

    for (int i = 0; i < activeSize; ++i) {
        float cx = 40.f + i * 58.f;
        sf::RectangleShape cell({50.f, 40.f});
        cell.setPosition({cx, 342.f});

        bool isHl = (i == hl1 || i == hl2);
        if (isHl) {
            cell.setFillColor(sf::Color(90, 65, 20));
            cell.setOutlineColor(sf::Color(255, 220, 60));
        } else if (i == 0) {
            cell.setFillColor(sf::Color(60, 50, 20));
            cell.setOutlineColor(sf::Color(240, 190, 40));
        } else {
            cell.setFillColor(sf::Color(25, 40, 65));
            cell.setOutlineColor(sf::Color(70, 140, 220));
        }
        cell.setOutlineThickness(2.f);
        window.draw(cell);

        sf::Text ct(font, std::to_string(arr[i]), 15);
        ct.setFillColor(sf::Color::White);
        auto b = ct.getLocalBounds();
        ct.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        ct.setPosition({cx + 25.f, 362.f});
        window.draw(ct);

        sf::Text idxTxt(font, std::to_string(i), 9);
        idxTxt.setFillColor(sf::Color(130, 150, 180));
        idxTxt.setPosition({cx + 3.f, 343.f});
        window.draw(idxTxt);
    }

    // Sorted Region
    if (sortedCount > 0) {
        float sx = 40.f + activeSize * 58.f + 25.f;
        sf::Text sl(font, "SORTED (" + std::to_string(sortedCount) + ")", 12);
        sl.setFillColor(sf::Color(80, 220, 120));
        sl.setPosition({sx, 320.f});
        window.draw(sl);

        for (int s = 0; s < sortedCount; ++s) {
            float scx = sx + s * 58.f;
            sf::RectangleShape cell({50.f, 40.f});
            cell.setPosition({scx, 342.f});
            cell.setFillColor(sf::Color(20, 50, 35));
            cell.setOutlineThickness(2.f);
            cell.setOutlineColor(sf::Color(60, 200, 100));
            window.draw(cell);

            sf::Text ct(font, std::to_string(sortedArr[s]), 15);
            ct.setFillColor(sf::Color(180, 255, 200));
            auto b = ct.getLocalBounds();
            ct.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
            ct.setPosition({scx + 25.f, 362.f});
            window.draw(ct);
        }
    }
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================
int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    constexpr unsigned int W = 1100, H = 780;
    sf::RenderWindow window(sf::VideoMode({W, H}), "Vault Heist - The Living Vault");
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("assets/fonts/arial.ttf") ||
                      font.openFromFile("../../assets/fonts/arial.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) std::cerr << "Warning: Arial Font could not be loaded!\n";

    // 10 Campaign Levels
    const int TOTAL_LEVELS = 10;
    LevelInfo levels[10];

    levels[0] = LevelInfo(LEVEL_3_1, 3, "3.1 Read the Badges",
        "Click EVERY unbalanced node (|BF| > 1). Red badge = violation!",
        "Click node circles on tree  |  [Enter] Submit  |  [R] Reset", 15, true, 0);

    levels[1] = LevelInfo(LEVEL_3_2, 3, "3.2 Manual Repair - Single Rotations",
        "Repair LL / RR imbalance. Click the correct rotation button below!",
        "Click buttons below  |  [R] Right-Rotate (LL)  |  [L] Left-Rotate (RR)", 20, false, 0);

    levels[2] = LevelInfo(LEVEL_3_3, 3, "3.3 Manual Repair - Double Rotations",
        "Identify and perform LR / RL double rotations.",
        "Click buttons below  |  [1] LR Double Rotation  |  [2] RL Double Rotation", 22, false, 0);

    levels[3] = LevelInfo(LEVEL_3_4, 3, "3.4 Auto Mode and Prediction",
        "Predict which rotation occurs before each incoming AVL insertion!",
        "Click prediction [1-5]  |  Click [Confirm & Insert]", 25, false, 0);

    levels[4] = LevelInfo(LEVEL_3_5, 3, "3.5 Boss: Order-Statistic Heist",
        "Steal the target treasure using subtree sizes in O(log n) steps!",
        "Click your answer button [1-4] or press [1-4]", 20, false, 0);

    levels[5] = LevelInfo(LEVEL_4_1, 4, "4.1 Activate the Vault",
        "Build a Max Heap bottom-up! Red edges = violations. Sift-down non-leafs.",
        "Click [Sift-Down Next] or press [S]  |  Click [Validate]", 25, false, 0);

    levels[6] = LevelInfo(LEVEL_4_2, 4, "4.2 Repair the Security Locks",
        "Guards sabotaged the heap! Use Sift-Down to restore order.",
        "Click [Sift-Down Root] or press [D]  |  Click [Validate]", 20, false, 0);

    levels[7] = LevelInfo(LEVEL_4_3, 4, "4.3 Extract the Greatest Treasure",
        "Extract root maximum to sorted region, then Heapify remainder.",
        "Click [Extract Max] or [E]  |  Click [Heapify] or [H]", 25, false, 0);

    levels[8] = LevelInfo(LEVEL_4_4, 4, "4.4 Race Against the Guards",
        "Complete multiple Heap Sort iterations before the alarm meter fills!",
        "Click [Extract & Heapify] or press [E]", 20, false, 0);

    levels[9] = LevelInfo(LEVEL_4_5, 4, "4.5 Final Vault Boss",
        "Full Heap Sort! Build Max Heap [B], then Extract All [E] to unlock vault!",
        "Click buttons: [Build Max Heap] -> [Extract All] -> [Unlock Vault]", 30, false, 0);

    levels[0].unlocked = true;
    levels[5].unlocked = true;

    // Game state
    int currentScreen = SCREEN_MAIN_MENU;
    int currentLevelIdx = -1;
    std::string status = "Welcome to Vault Heist!";
    int alarmSteps = 0, score = 1000;

    // Act 3 State
    ManualNode* manualTree = nullptr;
    int manualSubPuzzle = 1;
    bool clickedKeys31[100];
    for (int i = 0; i < 100; ++i) clickedKeys31[i] = false;

    AVLTree avl;
    int avlQueue[10] = { 40, 20, 10, 25, 60, 50, 70, 65, 80, 75 };
    std::string expectedRots[10] = { "None", "None", "LL", "LR", "None", "None", "RR", "RL", "None", "RL" };
    int avlQueuePos = 0;
    std::string playerPred = "";

    int bossQueryIdx = 1;

    // Act 4 State
    int heapArr[30];
    int activeHeapSize = 0;
    int sortedArr[30];
    int sortedCount = 0;
    int buildStepIdx = -1;
    int hlIndex1 = -1, hlIndex2 = -1;

    // Helper to start level
    auto startLevel = [&](int idx) {
        currentLevelIdx = idx;
        currentScreen = SCREEN_LEVEL_PLAY;
        alarmSteps = 0;
        score = 1000;
        sortedCount = 0;
        hlIndex1 = -1;
        hlIndex2 = -1;
        deleteManualTree(manualTree);
        manualTree = nullptr;

        int lid = levels[idx].id;

        if (lid == LEVEL_3_1) {
            for (int i = 0; i < 100; ++i) clickedKeys31[i] = false;
            auto n5  = new ManualNode(5, 1, 1);
            auto n10 = new ManualNode(10, 2, 2); n10->left = n5;
            auto n20 = new ManualNode(20, 3, 3); n20->left = n10;
            auto n30 = new ManualNode(30, 4, 4); n30->left = n20;
            auto n60 = new ManualNode(60, 1, 1);
            auto n90 = new ManualNode(90, 1, 1);
            auto n80 = new ManualNode(80, 2, 2); n80->right = n90;
            auto n70 = new ManualNode(70, 3, 4); n70->left = n60; n70->right = n80;
            auto n50 = new ManualNode(50, 5, 9); n50->left = n30; n50->right = n70;
            manualTree = n50;
            updateManualMetadata(manualTree);
            status = "Click both RED violation nodes (30 and 20), then click Submit!";
        }
        else if (lid == LEVEL_3_2) {
            manualSubPuzzle = 1;
            auto n10 = new ManualNode(10, 1, 1);
            auto n20 = new ManualNode(20, 2, 2); n20->left = n10;
            auto n30 = new ManualNode(30, 3, 3); n30->left = n20;
            manualTree = n30;
            updateManualMetadata(manualTree);
            status = "Puzzle 1/2: LL Imbalance! Root 30 has BF=+2. Click [Right-Rotate (LL)]!";
        }
        else if (lid == LEVEL_3_3) {
            manualSubPuzzle = 1;
            auto n20 = new ManualNode(20, 1, 1);
            auto n10 = new ManualNode(10, 2, 2); n10->right = n20;
            auto n30 = new ManualNode(30, 3, 3); n30->left = n10;
            manualTree = n30;
            updateManualMetadata(manualTree);
            status = "Puzzle 1/2: LR Case! Root 30 has BF=+2, child 10 is right-heavy. Click [LR Double Rotation]!";
        }
        else if (lid == LEVEL_3_4) {
            avl = AVLTree();
            avl.insert(40);
            avl.insert(20);
            avlQueuePos = 2;
            playerPred = "";
            status = "Incoming: 10! Choose your prediction button, then click [Confirm & Insert]!";
        }
        else if (lid == LEVEL_3_5) {
            avl = AVLTree();
            int bKeys[] = { 40, 20, 60, 10, 30, 50, 70, 25 };
            for (int k : bKeys) avl.insert(k);
            bossQueryIdx = 1;
            status = "Boss Challenge 1/2: Find the 3rd smallest treasure (k=3)! Click your answer button below.";
        }
        else if (lid == LEVEL_4_1) {
            int initArr[] = { 15, 60, 25, 90, 75, 40, 80 };
            activeHeapSize = 7;
            for (int i = 0; i < 7; ++i) heapArr[i] = initArr[i];
            buildStepIdx = 2;
            hlIndex1 = 2; hlIndex2 = 6;
            status = "UNSORTED array loaded. Step 1: Click [Sift-Down Next] to fix node [2] (val 25).";
        }
        else if (lid == LEVEL_4_2) {
            int initArr[] = { 10, 85, 70, 40, 30, 60, 50, 20, 35 };
            activeHeapSize = 9;
            for (int i = 0; i < 9; ++i) heapArr[i] = initArr[i];
            hlIndex1 = 0; hlIndex2 = 1;
            status = "Sabotage detected! Root 10 is smaller than children. Click [Sift-Down Root]!";
        }
        else if (lid == LEVEL_4_3) {
            int initArr[] = { 95, 80, 75, 60, 40, 70, 50 };
            activeHeapSize = 7;
            for (int i = 0; i < 7; ++i) heapArr[i] = initArr[i];
            hlIndex1 = 0;
            status = "Valid Max Heap ready! Click [Extract Max] to move root (95) into sorted region.";
        }
        else if (lid == LEVEL_4_4) {
            int initArr[] = { 90, 70, 80, 40, 50, 60, 30 };
            activeHeapSize = 7;
            for (int i = 0; i < 7; ++i) heapArr[i] = initArr[i];
            hlIndex1 = 0;
            status = "RACE! Click [Extract & Heapify] repeatedly before the alarm meter reaches 20!";
        }
        else if (lid == LEVEL_4_5) {
            int initArr[] = { 25, 80, 15, 95, 60, 40, 70, 30 };
            activeHeapSize = 8;
            for (int i = 0; i < 8; ++i) heapArr[i] = initArr[i];
            status = "Level 4.5 BOSS: Phase 1 — Click [Build Max Heap]!";
        }
    };

    // Level Action Triggers (shared by mouse buttons and keyboard)
    auto triggerAction = [&](int actionCode) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        int lid = levels[currentLevelIdx].id;
        int alarmCap = levels[currentLevelIdx].alarmCapacity;

        // --- LEVEL 3.1: Read the Badges ---
        if (lid == LEVEL_3_1) {
            if (actionCode == 100) { // Submit
                bool correct = clickedKeys31[30] && clickedKeys31[20];
                int wrong = 0;
                for (int i = 0; i < 100; ++i) {
                    if (clickedKeys31[i] && i != 30 && i != 20) wrong++;
                }
                if (correct && wrong == 0) {
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "CORRECT! You identified all unbalanced nodes (|BF| > 1)!";
                } else {
                    alarmSteps += 2;
                    status = "WRONG! Look for the nodes with RED badges (|BF| > 1). Nodes 30 and 20!";
                }
            } else if (actionCode == 101) { // Reset
                for (int i = 0; i < 100; ++i) clickedKeys31[i] = false;
                status = "Selection reset! Click the nodes with RED badges.";
            }
        }
        // --- LEVEL 3.2: Single Rotations ---
        else if (lid == LEVEL_3_2) {
            if (manualSubPuzzle == 1) { // LL Case
                if (actionCode == 1) { // Right rotate
                    manualTree = manualRotateRight(manualTree);
                    alarmSteps++;
                    manualSubPuzzle = 2;
                    status = "LL Rotation SUCCESS! Node 20 is balanced root! Click [Next: Puzzle 2 (RR Case)].";
                } else if (actionCode == 2) {
                    alarmSteps += 2; score -= 50;
                    status = "WRONG! Left-Left (LL) imbalance requires RIGHT rotation!";
                }
            } else if (manualSubPuzzle == 2 && actionCode == 3) { // Next puzzle
                deleteManualTree(manualTree);
                auto n30 = new ManualNode(30, 1, 1);
                auto n20 = new ManualNode(20, 2, 2); n20->right = n30;
                auto n10 = new ManualNode(10, 3, 3); n10->right = n20;
                manualTree = n10;
                updateManualMetadata(manualTree);
                manualSubPuzzle = 3;
                status = "Puzzle 2/2: RR Imbalance! Root 10 has BF=-2. Click [Left-Rotate (RR)]!";
            } else if (manualSubPuzzle == 3) {
                if (actionCode == 2) { // Left rotate
                    manualTree = manualRotateLeft(manualTree);
                    alarmSteps++;
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "RR Rotation SUCCESS! All single rotations mastered!";
                } else if (actionCode == 1) {
                    alarmSteps += 2; score -= 50;
                    status = "WRONG! Right-Right (RR) imbalance requires LEFT rotation!";
                }
            }
        }
        // --- LEVEL 3.3: Double Rotations ---
        else if (lid == LEVEL_3_3) {
            if (manualSubPuzzle == 1) {
                if (actionCode == 1) { // LR
                    manualTree->left = manualRotateLeft(manualTree->left);
                    updateManualMetadata(manualTree);
                    manualTree = manualRotateRight(manualTree);
                    alarmSteps += 2;
                    manualSubPuzzle = 2;
                    status = "LR Double Rotation SUCCESS! Click [Next: Puzzle 2 (RL Case)].";
                } else if (actionCode == 2) {
                    alarmSteps += 2; score -= 50;
                    status = "WRONG! Left child is right-heavy: this is an LR case, not RL!";
                }
            } else if (manualSubPuzzle == 2 && actionCode == 3) {
                deleteManualTree(manualTree);
                auto n20 = new ManualNode(20, 1, 1);
                auto n30 = new ManualNode(30, 2, 2); n30->left = n20;
                auto n10 = new ManualNode(10, 3, 3); n10->right = n30;
                manualTree = n10;
                updateManualMetadata(manualTree);
                manualSubPuzzle = 3;
                status = "Puzzle 2/2: RL Case! Root 10 has BF=-2, child 30 is left-heavy. Click [RL Double Rotation]!";
            } else if (manualSubPuzzle == 3) {
                if (actionCode == 2) { // RL
                    manualTree->right = manualRotateRight(manualTree->right);
                    updateManualMetadata(manualTree);
                    manualTree = manualRotateLeft(manualTree);
                    alarmSteps += 2;
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "RL SUCCESS! Double rotations mastered!";
                } else if (actionCode == 1) {
                    alarmSteps += 2; score -= 50;
                    status = "WRONG! Right child is left-heavy: this is an RL case, not LR!";
                }
            }
        }
        // --- LEVEL 3.4: Auto Mode & Prediction ---
        else if (lid == LEVEL_3_4) {
            if (actionCode >= 1 && actionCode <= 5) {
                if (actionCode == 1) playerPred = "LL";
                if (actionCode == 2) playerPred = "RR";
                if (actionCode == 3) playerPred = "LR";
                if (actionCode == 4) playerPred = "RL";
                if (actionCode == 5) playerPred = "None";
                status = "Selected prediction: " + playerPred + ". Now click [Confirm & Insert Key]!";
            } else if (actionCode == 10 && !playerPred.empty()) {
                int incoming = avlQueue[avlQueuePos];
                std::string expected = expectedRots[avlQueuePos];
                bool match = (playerPred == expected);

                avl.insert(incoming);
                alarmSteps++;
                if (match) {
                    score += 50;
                    status = "CORRECT! Prediction was " + playerPred + ". Inserted " + std::to_string(incoming) + ".";
                } else {
                    alarmSteps++; score -= 30;
                    status = "MISSED! Predicted " + playerPred + ", but was " + expected + ". Inserted " + std::to_string(incoming) + ".";
                }

                avlQueuePos++;
                playerPred = "";

                if (avlQueuePos >= 5) {
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "Level 3.4 Complete! AVL Tree auto-balanced flawlessly!";
                } else {
                    status += " Next incoming: " + std::to_string(avlQueue[avlQueuePos]) + ". Select prediction!";
                }
            }
        }
        // --- LEVEL 3.5: Boss Order Statistics ---
        else if (lid == LEVEL_3_5) {
            if (bossQueryIdx == 1) {
                if (actionCode == 2) { // Option 2 = 25
                    alarmSteps++;
                    bossQueryIdx = 2;
                    status = "CORRECT! 3rd smallest is 25! Next: Question 2 — What is rank(40)?";
                } else {
                    alarmSteps += 2;
                    status = "INCORRECT! Count left subtree size: Root 40 left size is 4 -> go Left to 20 -> left size 1 -> go Right to 30 -> left size 1 -> 25!";
                }
            } else if (bossQueryIdx == 2) {
                if (actionCode == 3) { // Option 3 = 5
                    alarmSteps++;
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "BOSS DEFEATED! Rank(40) = 5 found in O(log n)! Act 3 Complete!";
                } else {
                    alarmSteps += 2;
                    status = "INCORRECT! Keys <= 40 are {10, 20, 25, 30, 40}. Exactly 5 keys!";
                }
            }
        }
        // --- LEVEL 4.1: Build Heap Bottom-Up ---
        else if (lid == LEVEL_4_1) {
            if (actionCode == 1) { // Sift-Down Next
                if (buildStepIdx == 2) {
                    std::swap(heapArr[2], heapArr[6]);
                    alarmSteps++;
                    buildStepIdx = 1;
                    hlIndex1 = 1; hlIndex2 = 3;
                    status = "Step 1: Swapped 25 with 80! Next: Sift-down node [1] (val 60). Click [Sift-Down Next]!";
                } else if (buildStepIdx == 1) {
                    std::swap(heapArr[1], heapArr[3]);
                    alarmSteps++;
                    buildStepIdx = 0;
                    hlIndex1 = 0; hlIndex2 = 3;
                    status = "Step 2: Swapped 60 with 90! Next: Sift-down root [0] (val 15). Click [Sift-Down Next]!";
                } else if (buildStepIdx == 0) {
                    std::swap(heapArr[0], heapArr[1]);
                    std::swap(heapArr[1], heapArr[4]);
                    alarmSteps += 2;
                    buildStepIdx = -1;
                    hlIndex1 = -1; hlIndex2 = -1;
                    status = "Bottom-up Construction Done! Click [Validate Max Heap] to activate vault!";
                }
            } else if (actionCode == 2) { // Validate
                bool valid = true;
                for (int i = 0; i <= (activeHeapSize - 2) / 2; ++i) {
                    if (2*i+1 < activeHeapSize && heapArr[i] < heapArr[2*i+1]) valid = false;
                    if (2*i+2 < activeHeapSize && heapArr[i] < heapArr[2*i+2]) valid = false;
                }
                if (valid) {
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "VAULT ACTIVATED! Valid Max Heap constructed in O(n) time!";
                } else {
                    alarmSteps += 2;
                    status = "Heap still violates property! Follow bottom-up swaps [Sift-Down Next].";
                }
            }
        }
        // --- LEVEL 4.2: Repair Security Locks ---
        else if (lid == LEVEL_4_2) {
            if (actionCode == 1) { // Sift-Down Root
                std::swap(heapArr[0], heapArr[1]);
                std::swap(heapArr[1], heapArr[3]);
                alarmSteps += 2;
                hlIndex1 = -1; hlIndex2 = -1;
                status = "Root 10 sifted down to valid leaf! Click [Validate Security Locks]!";
            } else if (actionCode == 2) { // Validate
                bool valid = true;
                for (int i = 0; i <= (activeHeapSize - 2) / 2; ++i) {
                    if (2*i+1 < activeHeapSize && heapArr[i] < heapArr[2*i+1]) valid = false;
                    if (2*i+2 < activeHeapSize && heapArr[i] < heapArr[2*i+2]) valid = false;
                }
                if (valid) {
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "SECURITY LOCKS REPAIRED! Max Heap property restored!";
                } else {
                    alarmSteps += 2;
                    status = "Violations remain! Click [Sift-Down Root].";
                }
            }
        }
        // --- LEVEL 4.3: Extract Max ---
        else if (lid == LEVEL_4_3) {
            if (actionCode == 1 && activeHeapSize > 0) { // Extract
                int maxVal = heapArr[0];
                std::swap(heapArr[0], heapArr[activeHeapSize - 1]);
                sortedArr[sortedCount++] = maxVal;
                activeHeapSize--;
                alarmSteps++;
                hlIndex1 = 0;
                status = "Extracted " + std::to_string(maxVal) + " to sorted region! Click [Heapify Remainder].";
            } else if (actionCode == 2 && activeHeapSize > 0) { // Heapify
                int curr = 0;
                while (true) {
                    int l = 2*curr+1, r = 2*curr+2, largest = curr;
                    if (l < activeHeapSize && heapArr[l] > heapArr[largest]) largest = l;
                    if (r < activeHeapSize && heapArr[r] > heapArr[largest]) largest = r;
                    if (largest != curr) {
                        std::swap(heapArr[curr], heapArr[largest]);
                        curr = largest;
                    } else break;
                }
                alarmSteps++;
                hlIndex1 = -1;
                if (activeHeapSize <= 1) {
                    if (activeHeapSize == 1) sortedArr[sortedCount++] = heapArr[0];
                    activeHeapSize = 0;
                    levels[currentLevelIdx].stars = 3;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "All treasures extracted and sorted!";
                } else {
                    status = "Heapified! Next root is " + std::to_string(heapArr[0]) + ". Click [Extract Max].";
                }
            }
        }
        // --- LEVEL 4.4: Race Against Guards ---
        else if (lid == LEVEL_4_4) {
            if (actionCode == 1 && activeHeapSize > 0) { // Extract + sift
                int maxVal = heapArr[0];
                std::swap(heapArr[0], heapArr[activeHeapSize - 1]);
                sortedArr[sortedCount++] = maxVal;
                activeHeapSize--;
                int curr = 0;
                while (true) {
                    int l = 2*curr+1, r = 2*curr+2, largest = curr;
                    if (l < activeHeapSize && heapArr[l] > heapArr[largest]) largest = l;
                    if (r < activeHeapSize && heapArr[r] > heapArr[largest]) largest = r;
                    if (largest != curr) {
                        std::swap(heapArr[curr], heapArr[largest]);
                        curr = largest;
                    } else break;
                }
                alarmSteps += 2;
                status = "Extracted: " + std::to_string(maxVal) + " | Alarm: " + std::to_string(alarmSteps) + "/" + std::to_string(alarmCap);

                if (activeHeapSize == 0) {
                    levels[currentLevelIdx].stars = (alarmSteps <= 14) ? 3 : 2;
                    levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "RACE WON! Escaped before guards arrived!";
                }
            }
        }
        // --- LEVEL 4.5: Final Vault Boss ---
        else if (lid == LEVEL_4_5) {
            if (actionCode == 1) { // Build
                for (int i = (activeHeapSize / 2) - 1; i >= 0; --i) {
                    int curr = i;
                    while (true) {
                        int l = 2*curr+1, r = 2*curr+2, largest = curr;
                        if (l < activeHeapSize && heapArr[l] > heapArr[largest]) largest = l;
                        if (r < activeHeapSize && heapArr[r] > heapArr[largest]) largest = r;
                        if (largest != curr) {
                            std::swap(heapArr[curr], heapArr[largest]);
                            curr = largest;
                        } else break;
                    }
                }
                alarmSteps += 4;
                status = "Max Heap Built! Phase 2: Click [Extract Next Max] repeatedly!";
            } else if (actionCode == 2 && activeHeapSize > 0) { // Extract
                int maxVal = heapArr[0];
                std::swap(heapArr[0], heapArr[activeHeapSize - 1]);
                sortedArr[sortedCount++] = maxVal;
                activeHeapSize--;
                int curr = 0;
                while (true) {
                    int l = 2*curr+1, r = 2*curr+2, largest = curr;
                    if (l < activeHeapSize && heapArr[l] > heapArr[largest]) largest = l;
                    if (r < activeHeapSize && heapArr[r] > heapArr[largest]) largest = r;
                    if (largest != curr) {
                        std::swap(heapArr[curr], heapArr[largest]);
                        curr = largest;
                    } else break;
                }
                alarmSteps += 2;
                status = "Extracted " + std::to_string(maxVal) + ". Remaining active: " + std::to_string(activeHeapSize);
                if (activeHeapSize == 0) {
                    status = "All treasures sorted! Click [Unlock Final Vault Door]!";
                }
            } else if (actionCode == 3 && activeHeapSize == 0) { // Unlock
                levels[currentLevelIdx].stars = 3;
                currentScreen = SCREEN_RESULT;
                status = "FINAL VAULT UNLOCKED! HEAP SORT COMPLETE! CAMPAIGN CONQUERED!";
            }
        }

        // Alarm failure check
        if (alarmSteps >= alarmCap) {
            levels[currentLevelIdx].stars = 0;
            currentScreen = SCREEN_RESULT;
            status = "ALARM FULL! Guards arrived. Level failed! Click Retry.";
        }
    };

    // =========================================================================
    // MAIN LOOP
    // =========================================================================
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Escape) {
                    if (currentScreen == SCREEN_LEVEL_PLAY || currentScreen == SCREEN_RESULT) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                    } else if (currentScreen == SCREEN_CAMPAIGN_MAP || currentScreen == SCREEN_SANDBOX) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else {
                        window.close();
                    }
                }

                // Keyboard Shortcuts mapped to Level Actions
                if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    int lid = levels[currentLevelIdx].id;

                    if (lid == LEVEL_3_1) {
                        if (kp->code == sf::Keyboard::Key::Enter) triggerAction(100);
                        if (kp->code == sf::Keyboard::Key::R) triggerAction(101);
                    }
                    else if (lid == LEVEL_3_2) {
                        if (kp->code == sf::Keyboard::Key::R) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::L) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Space) triggerAction(3);
                    }
                    else if (lid == LEVEL_3_3) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Space) triggerAction(3);
                    }
                    else if (lid == LEVEL_3_4) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                        if (kp->code == sf::Keyboard::Key::Num4) triggerAction(4);
                        if (kp->code == sf::Keyboard::Key::Num5) triggerAction(5);
                        if (kp->code == sf::Keyboard::Key::Space) triggerAction(10);
                    }
                    else if (lid == LEVEL_3_5) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                        if (kp->code == sf::Keyboard::Key::Num4) triggerAction(4);
                    }
                    else if (lid == LEVEL_4_1) {
                        if (kp->code == sf::Keyboard::Key::S) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::V) triggerAction(2);
                    }
                    else if (lid == LEVEL_4_2) {
                        if (kp->code == sf::Keyboard::Key::D) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::V) triggerAction(2);
                    }
                    else if (lid == LEVEL_4_3) {
                        if (kp->code == sf::Keyboard::Key::E) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::H) triggerAction(2);
                    }
                    else if (lid == LEVEL_4_4) {
                        if (kp->code == sf::Keyboard::Key::E) triggerAction(1);
                    }
                    else if (lid == LEVEL_4_5) {
                        if (kp->code == sf::Keyboard::Key::B) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::E) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Enter) triggerAction(3);
                    }
                }

                if (currentScreen == SCREEN_RESULT) {
                    if (kp->code == sf::Keyboard::Key::R) startLevel(currentLevelIdx);
                    if (kp->code == sf::Keyboard::Key::N) {
                        if (currentLevelIdx + 1 < TOTAL_LEVELS && levels[currentLevelIdx + 1].unlocked) {
                            startLevel(currentLevelIdx + 1);
                        } else {
                            currentScreen = SCREEN_CAMPAIGN_MAP;
                        }
                    }
                }
            }

            // Mouse Click Handling
            if (const auto* mc = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(mc->position.x);
                float my = static_cast<float>(mc->position.y);

                // --- MAIN MENU CLICKS ---
                if (currentScreen == SCREEN_MAIN_MENU) {
                    if (isInside(mx, my, W/2.f - 150.f, 260.f, 300.f, 50.f)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                    }
                    if (isInside(mx, my, W/2.f - 150.f, 330.f, 300.f, 50.f)) {
                        currentScreen = SCREEN_SANDBOX;
                        avl = AVLTree();
                        int init[] = { 40, 20, 60, 10, 30, 50, 70 };
                        for (int v : init) avl.insert(v);
                        int hInit[] = { 90, 75, 80, 60, 50, 40, 70 };
                        activeHeapSize = 7;
                        for (int i = 0; i < 7; ++i) heapArr[i] = hInit[i];
                        sortedCount = 0;
                    }
                }

                // --- CAMPAIGN MAP CLICKS ---
                else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    for (int i = 0; i < TOTAL_LEVELS; ++i) {
                        float bx = (i < 5) ? 60.f : 560.f;
                        float by = 170.f + (i % 5) * 65.f;
                        if (isInside(mx, my, bx, by, 440.f, 52.f) && levels[i].unlocked) {
                            startLevel(i);
                            break;
                        }
                    }
                }

                // --- RESULT SCREEN CLICKS ---
                else if (currentScreen == SCREEN_RESULT) {
                    if (isInside(mx, my, W/2.f - 220.f, 440.f, 200.f, 50.f)) {
                        startLevel(currentLevelIdx);
                    }
                    if (isInside(mx, my, W/2.f + 20.f, 440.f, 200.f, 50.f)) {
                        if (currentLevelIdx + 1 < TOTAL_LEVELS && levels[currentLevelIdx + 1].unlocked) {
                            startLevel(currentLevelIdx + 1);
                        } else {
                            currentScreen = SCREEN_CAMPAIGN_MAP;
                        }
                    }
                }

                // --- LEVEL PLAY CLICKS ---
                else if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    int lid = levels[currentLevelIdx].id;

                    // LEVEL 3.1: Tree node clicks
                    if (lid == LEVEL_3_1) {
                        struct NodeHit { float x; float y; int key; };
                        NodeHit hits[] = {
                            {W/2.f, 115.f, 50},
                            {W/2.f - 210.f, 179.f, 30},
                            {W/2.f - 210.f - 109.f, 243.f, 20},
                            {W/2.f - 210.f - 109.f - 56.f, 307.f, 10},
                            {W/2.f - 210.f - 109.f - 56.f - 29.f, 371.f, 5},
                            {W/2.f + 210.f, 179.f, 70},
                            {W/2.f + 210.f - 109.f, 243.f, 60},
                            {W/2.f + 210.f + 109.f, 243.f, 80},
                            {W/2.f + 210.f + 109.f + 56.f, 307.f, 90}
                        };
                        for (int i = 0; i < 9; ++i) {
                            float dx = mx - hits[i].x, dy = my - hits[i].y;
                            if (dx*dx + dy*dy <= 28.f * 28.f) {
                                clickedKeys31[hits[i].key] = !clickedKeys31[hits[i].key];
                                status = "Toggled node " + std::to_string(hits[i].key) + (clickedKeys31[hits[i].key] ? " (SELECTED)" : " (DESELECTED)") + ". Click Submit when ready.";
                                break;
                            }
                        }
                        // Buttons
                        if (isInside(mx, my, 350.f, 540.f, 180.f, 45.f)) triggerAction(100); // Submit
                        if (isInside(mx, my, 560.f, 540.f, 180.f, 45.f)) triggerAction(101); // Reset
                    }

                    // LEVEL 3.2: Single Rotation Buttons
                    else if (lid == LEVEL_3_2) {
                        if (manualSubPuzzle == 1) {
                            if (isInside(mx, my, 300.f, 530.f, 230.f, 48.f)) triggerAction(1); // Right
                            if (isInside(mx, my, 560.f, 530.f, 230.f, 48.f)) triggerAction(2); // Left
                        } else if (manualSubPuzzle == 2) {
                            if (isInside(mx, my, 400.f, 530.f, 300.f, 48.f)) triggerAction(3); // Next
                        } else if (manualSubPuzzle == 3) {
                            if (isInside(mx, my, 300.f, 530.f, 230.f, 48.f)) triggerAction(2); // Left
                            if (isInside(mx, my, 560.f, 530.f, 230.f, 48.f)) triggerAction(1); // Right
                        }
                    }

                    // LEVEL 3.3: Double Rotation Buttons
                    else if (lid == LEVEL_3_3) {
                        if (manualSubPuzzle == 1) {
                            if (isInside(mx, my, 280.f, 530.f, 260.f, 48.f)) triggerAction(1); // LR
                            if (isInside(mx, my, 560.f, 530.f, 260.f, 48.f)) triggerAction(2); // RL
                        } else if (manualSubPuzzle == 2) {
                            if (isInside(mx, my, 400.f, 530.f, 300.f, 48.f)) triggerAction(3); // Next
                        } else if (manualSubPuzzle == 3) {
                            if (isInside(mx, my, 280.f, 530.f, 260.f, 48.f)) triggerAction(2); // RL
                            if (isInside(mx, my, 560.f, 530.f, 260.f, 48.f)) triggerAction(1); // LR
                        }
                    }

                    // LEVEL 3.4: Prediction Buttons
                    else if (lid == LEVEL_3_4) {
                        if (isInside(mx, my, 150.f, 510.f, 130.f, 42.f)) triggerAction(1); // LL
                        if (isInside(mx, my, 310.f, 510.f, 130.f, 42.f)) triggerAction(2); // RR
                        if (isInside(mx, my, 470.f, 510.f, 130.f, 42.f)) triggerAction(3); // LR
                        if (isInside(mx, my, 630.f, 510.f, 130.f, 42.f)) triggerAction(4); // RL
                        if (isInside(mx, my, 790.f, 510.f, 130.f, 42.f)) triggerAction(5); // None

                        if (isInside(mx, my, 420.f, 565.f, 260.f, 44.f)) triggerAction(10); // Confirm & Insert
                    }

                    // LEVEL 3.5: Boss Query Buttons
                    else if (lid == LEVEL_3_5) {
                        if (isInside(mx, my, 130.f, 530.f, 180.f, 45.f)) triggerAction(1);
                        if (isInside(mx, my, 340.f, 530.f, 180.f, 45.f)) triggerAction(2);
                        if (isInside(mx, my, 550.f, 530.f, 180.f, 45.f)) triggerAction(3);
                        if (isInside(mx, my, 760.f, 530.f, 180.f, 45.f)) triggerAction(4);
                    }

                    // LEVEL 4.1: Build Heap Buttons
                    else if (lid == LEVEL_4_1) {
                        if (isInside(mx, my, 300.f, 535.f, 230.f, 48.f)) triggerAction(1); // Sift-Down
                        if (isInside(mx, my, 560.f, 535.f, 230.f, 48.f)) triggerAction(2); // Validate
                    }

                    // LEVEL 4.2: Repair Locks Buttons
                    else if (lid == LEVEL_4_2) {
                        if (isInside(mx, my, 300.f, 535.f, 230.f, 48.f)) triggerAction(1); // Sift-Down Root
                        if (isInside(mx, my, 560.f, 535.f, 230.f, 48.f)) triggerAction(2); // Validate
                    }

                    // LEVEL 4.3: Extract Max Buttons
                    else if (lid == LEVEL_4_3) {
                        if (isInside(mx, my, 300.f, 535.f, 230.f, 48.f)) triggerAction(1); // Extract
                        if (isInside(mx, my, 560.f, 535.f, 230.f, 48.f)) triggerAction(2); // Heapify
                    }

                    // LEVEL 4.4: Race Button
                    else if (lid == LEVEL_4_4) {
                        if (isInside(mx, my, 400.f, 535.f, 300.f, 48.f)) triggerAction(1); // Extract & Sift
                    }

                    // LEVEL 4.5: Boss Buttons
                    else if (lid == LEVEL_4_5) {
                        if (isInside(mx, my, 180.f, 535.f, 210.f, 48.f)) triggerAction(1); // Build
                        if (isInside(mx, my, 420.f, 535.f, 210.f, 48.f)) triggerAction(2); // Extract
                        if (isInside(mx, my, 660.f, 535.f, 240.f, 48.f)) triggerAction(3); // Unlock
                    }
                }
            }
        }

        // =====================================================================
        // RENDERING
        // =====================================================================
        window.clear(sf::Color(16, 20, 32));

        // 1. MAIN MENU
        if (currentScreen == SCREEN_MAIN_MENU) {
            sf::Text title(font, "VAULT HEIST", 52);
            title.setFillColor(sf::Color(255, 215, 0));
            auto tb = title.getLocalBounds();
            title.setOrigin({tb.position.x + tb.size.x / 2.f, tb.position.y});
            title.setPosition({W / 2.f, 80.f});
            window.draw(title);

            sf::Text sub(font, "The Living Vault", 22);
            sub.setFillColor(sf::Color(200, 200, 220));
            auto sb = sub.getLocalBounds();
            sub.setOrigin({sb.position.x + sb.size.x / 2.f, sb.position.y});
            sub.setPosition({W / 2.f, 150.f});
            window.draw(sub);

            drawButton(window, font, "1. Campaign Mode", W/2.f - 150.f, 260.f, 300.f, 50.f, sf::Color(35, 55, 85), sf::Color::White, sf::Color(90, 150, 230), 17);
            drawButton(window, font, "2. Sandbox Mode",  W/2.f - 150.f, 330.f, 300.f, 50.f, sf::Color(35, 55, 85), sf::Color::White, sf::Color(90, 150, 230), 17);

            sf::Text hint(font, "Click to Select  |  [Esc] Quit", 14);
            hint.setFillColor(sf::Color(150, 160, 180));
            auto hb = hint.getLocalBounds();
            hint.setOrigin({hb.position.x + hb.size.x / 2.f, hb.position.y});
            hint.setPosition({W / 2.f, 450.f});
            window.draw(hint);
        }

        // 2. CAMPAIGN MAP
        if (currentScreen == SCREEN_CAMPAIGN_MAP) {
            sf::Text mapTitle(font, "CAMPAIGN MAP", 28);
            mapTitle.setFillColor(sf::Color(255, 215, 0));
            mapTitle.setPosition({30.f, 20.f});
            window.draw(mapTitle);

            sf::Text a3(font, "ACT 3: THE ARCHITECT'S WING (AVL)", 16);
            a3.setFillColor(sf::Color(120, 200, 255));
            a3.setPosition({60.f, 130.f});
            window.draw(a3);

            sf::Text a4(font, "ACT 4: THE HEAP FORTRESS", 16);
            a4.setFillColor(sf::Color(255, 180, 80));
            a4.setPosition({560.f, 130.f});
            window.draw(a4);

            for (int i = 0; i < TOTAL_LEVELS; ++i) {
                float bx = (i < 5) ? 60.f : 560.f;
                float by = 170.f + (i % 5) * 65.f;
                sf::Color bg = levels[i].unlocked ? sf::Color(35, 55, 85) : sf::Color(25, 25, 35);
                sf::Color tc = levels[i].unlocked ? sf::Color::White : sf::Color(80, 80, 100);

                std::string starStr = "";
                for (int s = 0; s < levels[i].stars; ++s) starStr += "*";
                for (int s = levels[i].stars; s < 3; ++s) starStr += " ";

                std::string label = levels[i].title;
                if (!levels[i].unlocked) label += "  [LOCKED]";
                else if (levels[i].stars > 0) label += "  [" + starStr + "]";

                drawButton(window, font, label, bx, by, 440.f, 52.f, bg, tc, sf::Color(80, 120, 180), 15);
            }

            sf::Text hint(font, "Click any unlocked level to play  |  [Esc] Back to Menu", 13);
            hint.setFillColor(sf::Color(150, 160, 180));
            hint.setPosition({60.f, H - 45.f});
            window.draw(hint);
        }

        // 3. LEVEL PLAY SCREEN
        if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            int lid = levels[currentLevelIdx].id;
            int alarmCap = levels[currentLevelIdx].alarmCapacity;

            // Header
            sf::Text header(font, levels[currentLevelIdx].title, 22);
            header.setFillColor(sf::Color(255, 215, 0));
            header.setPosition({30.f, 10.f});
            window.draw(header);

            sf::Text obj(font, "Objective: " + levels[currentLevelIdx].objective, 13);
            obj.setFillColor(sf::Color(200, 210, 230));
            obj.setPosition({30.f, 38.f});
            window.draw(obj);

            // Alarm Meter
            sf::RectangleShape alarmBg({280.f, 18.f});
            alarmBg.setPosition({780.f, 15.f});
            alarmBg.setFillColor(sf::Color(40, 45, 60));
            alarmBg.setOutlineThickness(1.5f);
            alarmBg.setOutlineColor(sf::Color(100, 110, 130));
            window.draw(alarmBg);

            float ratio = std::min(1.0f, static_cast<float>(alarmSteps) / alarmCap);
            sf::RectangleShape alarmFill({280.f * ratio, 18.f});
            alarmFill.setPosition({780.f, 15.f});
            alarmFill.setFillColor(ratio > 0.8f ? sf::Color(230, 50, 50) : sf::Color(240, 160, 30));
            window.draw(alarmFill);

            sf::Text alarmTxt(font, "Alarm: " + std::to_string(alarmSteps) + " / " + std::to_string(alarmCap) +
                              "  Score: " + std::to_string(score), 13);
            alarmTxt.setFillColor(sf::Color::White);
            alarmTxt.setPosition({780.f, 36.f});
            window.draw(alarmTxt);

            // --- Upper Tree Canvas ---
            if (lid == LEVEL_3_1) {
                drawManualNode(window, font, manualTree, W / 2.f, 115.f, 210.f);
                struct NodePos { float x; float y; int key; };
                NodePos p[] = {
                    {W/2.f, 115.f, 50},
                    {W/2.f - 210.f, 179.f, 30},
                    {W/2.f - 210.f - 109.f, 243.f, 20},
                    {W/2.f - 210.f - 109.f - 56.f, 307.f, 10},
                    {W/2.f - 210.f - 109.f - 56.f - 29.f, 371.f, 5},
                    {W/2.f + 210.f, 179.f, 70},
                    {W/2.f + 210.f - 109.f, 243.f, 60},
                    {W/2.f + 210.f + 109.f, 243.f, 80},
                    {W/2.f + 210.f + 109.f + 56.f, 307.f, 90}
                };
                for (int i = 0; i < 9; ++i) {
                    if (clickedKeys31[p[i].key]) {
                        sf::CircleShape ring(25.f);
                        ring.setOrigin({25.f, 25.f});
                        ring.setPosition({p[i].x, p[i].y});
                        ring.setFillColor(sf::Color::Transparent);
                        ring.setOutlineColor(sf::Color(255, 220, 40));
                        ring.setOutlineThickness(3.5f);
                        window.draw(ring);
                    }
                }
            }
            else if (lid == LEVEL_3_2 || lid == LEVEL_3_3) {
                drawManualNode(window, font, manualTree, W / 2.f, 120.f, 180.f);
            }
            else if (lid == LEVEL_3_4 || lid == LEVEL_3_5) {
                drawAVLNode(window, font, avl.getRoot(), W / 2.f, 115.f, 190.f);
            }
            else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                drawHeapCustom(window, font, heapArr, activeHeapSize, sortedArr, sortedCount, W, hlIndex1, hlIndex2);
            }

            // --- Middle Interactive Action Panel ---
            sf::RectangleShape midPanel({W - 60.f, 160.f});
            midPanel.setPosition({30.f, 470.f});
            midPanel.setFillColor(sf::Color(22, 28, 44));
            midPanel.setOutlineThickness(1.5f);
            midPanel.setOutlineColor(sf::Color(60, 85, 125));
            window.draw(midPanel);

            // LEVEL-SPECIFIC QUESTION & BUTTON RENDERING
            sf::Color btnBg(35, 50, 75);
            sf::Color btnOutline(80, 130, 200);

            if (lid == LEVEL_3_1) {
                sf::Text qTitle(font, "IDENTIFICATION TASK: Click both nodes with RED violation badges (|BF| > 1)", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Submit Selection [Enter]", 350.f, 540.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 14);
                drawButton(window, font, "Reset [R]", 560.f, 540.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 14);
            }
            else if (lid == LEVEL_3_2) {
                if (manualSubPuzzle == 1) {
                    sf::Text qTitle(font, "PUZZLE 1/2: Left-Left (LL) Imbalance! Root 30 has BF = +2. Select rotation:", 16);
                    qTitle.setFillColor(sf::Color(255, 215, 100));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "Right-Rotate 30 [R]", 300.f, 530.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                    drawButton(window, font, "Left-Rotate 30 [L]", 560.f, 530.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                } else if (manualSubPuzzle == 2) {
                    sf::Text qTitle(font, "LL ROTATION SUCCESS! Badges are balanced. Click to proceed:", 16);
                    qTitle.setFillColor(sf::Color(100, 230, 140));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "Next: Puzzle 2 (RR Case) [Space]", 400.f, 530.f, 300.f, 48.f, btnBg, sf::Color::White, sf::Color(100, 170, 240), 15);
                } else if (manualSubPuzzle == 3) {
                    sf::Text qTitle(font, "PUZZLE 2/2: Right-Right (RR) Imbalance! Root 10 has BF = -2. Select rotation:", 16);
                    qTitle.setFillColor(sf::Color(255, 215, 100));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "Left-Rotate 10 [L]", 300.f, 530.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                    drawButton(window, font, "Right-Rotate 10 [R]", 560.f, 530.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                }
            }
            else if (lid == LEVEL_3_3) {
                if (manualSubPuzzle == 1) {
                    sf::Text qTitle(font, "PUZZLE 1/2: Left-Right (LR) Imbalance! Root 30 BF=+2, child 10 is right-heavy.", 16);
                    qTitle.setFillColor(sf::Color(255, 215, 100));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "LR Double Rotation [1]", 280.f, 530.f, 260.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                    drawButton(window, font, "RL Double Rotation [2]", 560.f, 530.f, 260.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                } else if (manualSubPuzzle == 2) {
                    sf::Text qTitle(font, "LR SUCCESS! Tree balanced. Click to proceed:", 16);
                    qTitle.setFillColor(sf::Color(100, 230, 140));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "Next: Puzzle 2 (RL Case) [Space]", 400.f, 530.f, 300.f, 48.f, btnBg, sf::Color::White, sf::Color(100, 170, 240), 15);
                } else if (manualSubPuzzle == 3) {
                    sf::Text qTitle(font, "PUZZLE 2/2: Right-Left (RL) Imbalance! Root 10 BF=-2, child 30 is left-heavy.", 16);
                    qTitle.setFillColor(sf::Color(255, 215, 100));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "RL Double Rotation [2]", 280.f, 530.f, 260.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                    drawButton(window, font, "LR Double Rotation [1]", 560.f, 530.f, 260.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                }
            }
            else if (lid == LEVEL_3_4) {
                sf::Text qTitle(font, "PREDICT ROTATION for incoming key [" + (avlQueuePos < 10 ? std::to_string(avlQueue[avlQueuePos]) : "Done") + "]:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 480.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] LL", 150.f, 510.f, 130.f, 42.f, playerPred == "LL" ? sf::Color(75, 60, 25) : btnBg, sf::Color::White, playerPred == "LL" ? sf::Color(255, 215, 50) : btnOutline);
                drawButton(window, font, "[2] RR", 310.f, 510.f, 130.f, 42.f, playerPred == "RR" ? sf::Color(75, 60, 25) : btnBg, sf::Color::White, playerPred == "RR" ? sf::Color(255, 215, 50) : btnOutline);
                drawButton(window, font, "[3] LR", 470.f, 510.f, 130.f, 42.f, playerPred == "LR" ? sf::Color(75, 60, 25) : btnBg, sf::Color::White, playerPred == "LR" ? sf::Color(255, 215, 50) : btnOutline);
                drawButton(window, font, "[4] RL", 630.f, 510.f, 130.f, 42.f, playerPred == "RL" ? sf::Color(75, 60, 25) : btnBg, sf::Color::White, playerPred == "RL" ? sf::Color(255, 215, 50) : btnOutline);
                drawButton(window, font, "[5] None", 790.f, 510.f, 130.f, 42.f, playerPred == "None" ? sf::Color(75, 60, 25) : btnBg, sf::Color::White, playerPred == "None" ? sf::Color(255, 215, 50) : btnOutline);

                drawButton(window, font, "Confirm & Insert Key [Space]", 420.f, 565.f, 260.f, 44.f, btnBg, sf::Color::White, sf::Color(100, 170, 240), 15);
            }
            else if (lid == LEVEL_3_5) {
                if (bossQueryIdx == 1) {
                    sf::Text qTitle(font, "CHALLENGE 1/2: Find the 3rd smallest treasure (k = 3) using subtree sizes in O(log n):", 16);
                    qTitle.setFillColor(sf::Color(255, 215, 100));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "[1]  Key 20", 130.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                    drawButton(window, font, "[2]  Key 25", 340.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                    drawButton(window, font, "[3]  Key 30", 550.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                    drawButton(window, font, "[4]  Key 40", 760.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                } else if (bossQueryIdx == 2) {
                    sf::Text qTitle(font, "CHALLENGE 2/2: Find rank(40) — How many keys in the tree are <= 40?", 16);
                    qTitle.setFillColor(sf::Color(255, 215, 100));
                    qTitle.setPosition({50.f, 485.f});
                    window.draw(qTitle);

                    drawButton(window, font, "[1]  Rank = 3", 130.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                    drawButton(window, font, "[2]  Rank = 4", 340.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                    drawButton(window, font, "[3]  Rank = 5", 550.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                    drawButton(window, font, "[4]  Rank = 6", 760.f, 530.f, 180.f, 45.f, btnBg, sf::Color::White, btnOutline, 16);
                }
            }
            else if (lid == LEVEL_4_1) {
                sf::Text qTitle(font, "BOTTOM-UP HEAP CONSTRUCTION: Sift down non-leafs from (n/2 - 1) down to 0:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Sift-Down Next [S]", 300.f, 535.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 16);
                drawButton(window, font, "Validate Max Heap [V]", 560.f, 535.f, 230.f, 48.f, btnBg, sf::Color::White, sf::Color(100, 170, 240), 16);
            }
            else if (lid == LEVEL_4_2) {
                sf::Text qTitle(font, "REPAIR SECURITY LOCKS: Sabotaged root 10 violates heap order against child 85:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Sift-Down Root [D]", 300.f, 535.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 16);
                drawButton(window, font, "Validate Locks [V]", 560.f, 535.f, 230.f, 48.f, btnBg, sf::Color::White, sf::Color(100, 170, 240), 16);
            }
            else if (lid == LEVEL_4_3) {
                sf::Text qTitle(font, "HEAP EXTRACTION WORKFLOW: Extract maximum, then restore remaining heap:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Extract Max [E]", 300.f, 535.f, 230.f, 48.f, btnBg, sf::Color::White, btnOutline, 16);
                drawButton(window, font, "Heapify Remainder [H]", 560.f, 535.f, 230.f, 48.f, btnBg, sf::Color::White, sf::Color(100, 170, 240), 16);
            }
            else if (lid == LEVEL_4_4) {
                sf::Text qTitle(font, "RACE AGAINST GUARDS: Extract all treasures before alarm capacity (20) fills:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Extract Max & Sift-Down [E]", 400.f, 535.f, 300.f, 48.f, btnBg, sf::Color::White, btnOutline, 16);
            }
            else if (lid == LEVEL_4_5) {
                sf::Text qTitle(font, "FINAL BOSS HEAP SORT: Build Max Heap -> Extract All -> Unlock Door!", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "1. Build Heap [B]", 180.f, 535.f, 210.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                drawButton(window, font, "2. Extract Next [E]", 420.f, 535.f, 210.f, 48.f, btnBg, sf::Color::White, btnOutline, 15);
                drawButton(window, font, "3. Unlock Vault [Enter]", 660.f, 535.f, 240.f, 48.f, btnBg, sf::Color::White, sf::Color(255, 200, 40), 15);
            }

            // --- Bottom Permanent Status / Feedback Box ---
            sf::RectangleShape box({W - 60.f, 95.f});
            box.setPosition({30.f, H - 120.f});
            box.setFillColor(sf::Color(18, 22, 36));
            box.setOutlineThickness(1.5f);
            box.setOutlineColor(sf::Color(70, 80, 110));
            window.draw(box);

            sf::Text statusTxt(font, status, 15);
            statusTxt.setFillColor(sf::Color(255, 225, 120));
            statusTxt.setPosition({50.f, H - 110.f});
            window.draw(statusTxt);

            sf::Text ctrlTxt(font, levels[currentLevelIdx].controls + "  |  [Esc] Map", 13);
            ctrlTxt.setFillColor(sf::Color(170, 190, 220));
            ctrlTxt.setPosition({50.f, H - 70.f});
            window.draw(ctrlTxt);
        }

        // 4. RESULT SCREEN
        if (currentScreen == SCREEN_RESULT) {
            sf::Text rTitle(font, "LEVEL COMPLETE", 36);
            rTitle.setFillColor(sf::Color(255, 215, 0));
            auto rtb = rTitle.getLocalBounds();
            rTitle.setOrigin({rtb.position.x + rtb.size.x / 2.f, rtb.position.y});
            rTitle.setPosition({W / 2.f, 80.f});
            window.draw(rTitle);

            if (currentLevelIdx >= 0) {
                sf::Text lName(font, levels[currentLevelIdx].title, 20);
                lName.setFillColor(sf::Color::White);
                auto lb = lName.getLocalBounds();
                lName.setOrigin({lb.position.x + lb.size.x / 2.f, lb.position.y});
                lName.setPosition({W / 2.f, 140.f});
                window.draw(lName);

                std::string starDisplay = "";
                int stars = levels[currentLevelIdx].stars;
                for (int i = 0; i < 3; ++i) starDisplay += (i < stars) ? " * " : " . ";
                sf::Text starTxt(font, starDisplay, 40);
                starTxt.setFillColor(sf::Color(255, 220, 50));
                auto stb = starTxt.getLocalBounds();
                starTxt.setOrigin({stb.position.x + stb.size.x / 2.f, stb.position.y});
                starTxt.setPosition({W / 2.f, 190.f});
                window.draw(starTxt);

                sf::Text scoreTxt(font, "Score: " + std::to_string(score) +
                                  "  |  Alarm Steps: " + std::to_string(alarmSteps) +
                                  "  |  Award: " + std::to_string(stars) + "/3 Stars", 18);
                scoreTxt.setFillColor(sf::Color(200, 220, 255));
                auto scb = scoreTxt.getLocalBounds();
                scoreTxt.setOrigin({scb.position.x + scb.size.x / 2.f, scb.position.y});
                scoreTxt.setPosition({W / 2.f, 280.f});
                window.draw(scoreTxt);
            }

            sf::Text statusRes(font, status, 16);
            statusRes.setFillColor(sf::Color(180, 255, 200));
            auto srb = statusRes.getLocalBounds();
            statusRes.setOrigin({srb.position.x + srb.size.x / 2.f, srb.position.y});
            statusRes.setPosition({W / 2.f, 350.f});
            window.draw(statusRes);

            drawButton(window, font, "Retry Level [R]", W/2.f - 220.f, 440.f, 200.f, 50.f, sf::Color(40, 55, 85), sf::Color::White, sf::Color(90, 150, 230), 16);
            drawButton(window, font, "Next Level [N]",  W/2.f + 20.f, 440.f, 200.f, 50.f, sf::Color(40, 80, 50), sf::Color::White, sf::Color(80, 200, 100), 16);

            sf::Text hint(font, "Click to proceed  |  [Esc] Campaign Map", 13);
            hint.setFillColor(sf::Color(150, 160, 180));
            auto hb = hint.getLocalBounds();
            hint.setOrigin({hb.position.x + hb.size.x / 2.f, hb.position.y});
            hint.setPosition({W / 2.f, 520.f});
            window.draw(hint);
        }

        // 5. SANDBOX MODE
        if (currentScreen == SCREEN_SANDBOX) {
            sf::Text header(font, "SANDBOX MODE", 24);
            header.setFillColor(sf::Color(255, 215, 0));
            header.setPosition({30.f, 12.f});
            window.draw(header);

            drawAVLNode(window, font, avl.getRoot(), W / 4.f, 130.f, 130.f);
            drawHeapCustom(window, font, heapArr, activeHeapSize, sortedArr, sortedCount, W);

            sf::RectangleShape box({W - 60.f, 95.f});
            box.setPosition({30.f, H - 125.f});
            box.setFillColor(sf::Color(25, 30, 48));
            box.setOutlineThickness(1.5f);
            box.setOutlineColor(sf::Color(70, 80, 110));
            window.draw(box);

            sf::Text statusTxt(font, status, 15);
            statusTxt.setFillColor(sf::Color(255, 220, 120));
            statusTxt.setPosition({50.f, H - 115.f});
            window.draw(statusTxt);

            sf::Text ctrlTxt(font, "AVL: [I]Insert [D]Delete [K]kth  |  Heap: [E]Extract  |  [R]Reset  |  [Esc]Menu", 12);
            ctrlTxt.setFillColor(sf::Color(170, 190, 220));
            ctrlTxt.setPosition({50.f, H - 75.f});
            window.draw(ctrlTxt);
        }

        window.display();
    }

    deleteManualTree(manualTree);
    return 0;
}
