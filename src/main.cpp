// =============================================================================
// Vault Heist: The Living Vault — Main Game Source
// Acts 3 & 4 (Member 2): AVL Wing (3.1–3.5) + Heap Fortress (4.1–4.5)
// =============================================================================

#include <SFML/Graphics.hpp>
#include "structures/MaxHeap.h"
#include "structures/AVLTree.h"
#include "structures/BinaryTree.h"
#include "structures/BST.h"
#include "structures/TreeNode.h"
#include <optional>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Hand-written replacements for <algorithm> — functions, not global variables.
static inline void swapInt(int& a, int& b) { int t = a; a = b; b = t; }
static inline float minFloat(float a, float b) { return (a < b) ? a : b; }
static inline int minInt(int a, int b) { return (a < b) ? a : b; }
static inline int maxInt(int a, int b) { return (a > b) ? a : b; }

using namespace VaultHeist;

// Screen State Constants (no enum allowed)
static const int SCREEN_SPLASH        = 0;
static const int SCREEN_MAIN_MENU     = 1;
static const int SCREEN_CAMPAIGN_MAP  = 2;
static const int SCREEN_LEVEL_PLAY    = 3;
static const int SCREEN_RESULT        = 4;
static const int SCREEN_SANDBOX       = 5;
static const int SCREEN_ACT1_DEMO     = 6;
static const int SCREEN_ACT2_DEMO     = 7;
static const int SCREEN_LEADERBOARD   = 8;
static const int SCREEN_SETTINGS      = 9;

// Level ID Constants (no enum allowed)
static const int LEVEL_1_1 = 11;  // Traversals Protocol
static const int LEVEL_1_2 = 12;  // Security Laser Mirror
static const int LEVEL_1_3 = 13;  // Blueprint Reconstruction
static const int LEVEL_1_4 = 14;  // Height & Diameter
static const int LEVEL_1_5 = 15;  // Boss: LCA & Escape Path

static const int LEVEL_2_1 = 21;  // Duplicate Rejection Gateway
static const int LEVEL_2_2 = 22;  // Firewall Disarm (3 Cases)
static const int LEVEL_2_3 = 23;  // Infiltration Sweep (Min/Max/Search)
static const int LEVEL_2_4 = 24;  // Key Recovery (Succ & Pred)
static const int LEVEL_2_5 = 25;  // Boss: Range Query & kth-Smallest

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
                unsigned int charSize = 14,
                bool isHovered = false)
{
    sf::RectangleShape btn({w, h});
    btn.setPosition({x, y});
    if (isHovered) {
        sf::Color hBg(static_cast<std::uint8_t>(minInt(255, bgColor.r + 25)),
                      static_cast<std::uint8_t>(minInt(255, bgColor.g + 30)),
                      static_cast<std::uint8_t>(minInt(255, bgColor.b + 40)));
        sf::Color hOt(static_cast<std::uint8_t>(minInt(255, outlineColor.r + 35)),
                      static_cast<std::uint8_t>(minInt(255, outlineColor.g + 40)),
                      static_cast<std::uint8_t>(minInt(255, outlineColor.b + 50)));
        btn.setFillColor(hBg);
        btn.setOutlineThickness(2.2f);
        btn.setOutlineColor(hOt);
    } else {
        btn.setFillColor(bgColor);
        btn.setOutlineThickness(1.8f);
        btn.setOutlineColor(outlineColor);
    }
    window.draw(btn);

    sf::Text txt(font, label, charSize);
    txt.setFillColor(isHovered ? sf::Color(255, 255, 255) : txtColor);
    auto b = txt.getLocalBounds();
    txt.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    txt.setPosition({x + w / 2.f, y + h / 2.f});
    window.draw(txt);

    return false;
}

bool isInside(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

void drawBackgroundGrid(sf::RenderWindow& window, unsigned int W, unsigned int H) {
    sf::Color gridColor(20, 32, 54, 120);
    for (float x = 0; x <= W; x += 40.f) {
        sf::Vertex line[] = {
            sf::Vertex({x, 0.f}, gridColor),
            sf::Vertex({x, static_cast<float>(H)}, gridColor)
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
    for (float y = 0; y <= H; y += 40.f) {
        sf::Vertex line[] = {
            sf::Vertex({0.f, y}, gridColor),
            sf::Vertex({static_cast<float>(W), y}, gridColor)
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void drawTreeWatermark(sf::RenderWindow& window, float rootX, float rootY) {
    sf::Color branchColor(24, 40, 64, 130);
    sf::Color nodeFill(13, 22, 38, 160);
    sf::Color nodeOutline(26, 46, 74, 170);

    float lx = rootX - 54.f, ly = rootY + 70.f;
    float rx = rootX + 54.f, ry = rootY + 70.f;

    sf::Vertex lBranch[] = { sf::Vertex({rootX, rootY}, branchColor), sf::Vertex({lx, ly}, branchColor) };
    sf::Vertex rBranch[] = { sf::Vertex({rootX, rootY}, branchColor), sf::Vertex({rx, ry}, branchColor) };
    window.draw(lBranch, 2, sf::PrimitiveType::Lines);
    window.draw(rBranch, 2, sf::PrimitiveType::Lines);

    float r = 27.f;
    auto drawWMNode = [&](float x, float y) {
        sf::CircleShape c(r);
        c.setOrigin({r, r});
        c.setPosition({x, y});
        c.setFillColor(nodeFill);
        c.setOutlineThickness(2.2f);
        c.setOutlineColor(nodeOutline);
        window.draw(c);
    };

    drawWMNode(rootX, rootY);
    drawWMNode(lx, ly);
    drawWMNode(rx, ry);
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

void drawTreeNode(sf::RenderWindow& window, const sf::Font& font,
                  TreeNode* node, float x, float y, float hSpacing,
                  int highlightVal = -9999, int highlightVal2 = -9999)
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
        drawTreeNode(window, font, node->left, cx, cy, hSpacing * 0.52f, highlightVal, highlightVal2);
    }
    if (node->right) {
        float cx = x + hSpacing, cy = y + vSp;
        sf::Vertex line[] = {
            sf::Vertex({x, y}, sf::Color(100, 140, 180)),
            sf::Vertex({cx, cy}, sf::Color(100, 140, 180))
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
        drawTreeNode(window, font, node->right, cx, cy, hSpacing * 0.52f, highlightVal, highlightVal2);
    }

    bool isHl = (node->value == highlightVal || node->value == highlightVal2);
    sf::CircleShape circle(r);
    circle.setOrigin({r, r});
    circle.setPosition({x, y});
    circle.setFillColor(isHl ? sf::Color(110, 75, 20) : sf::Color(30, 50, 80));
    circle.setOutlineColor(isHl ? sf::Color(255, 220, 50) : sf::Color(90, 160, 240));
    circle.setOutlineThickness(isHl ? 3.5f : 2.5f);
    window.draw(circle);

    sf::Text val(font, std::to_string(node->value), 15);
    val.setFillColor(sf::Color::White);
    auto b = val.getLocalBounds();
    val.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    val.setPosition({x, y});
    window.draw(val);

    sf::Text idTxt(font, "#" + std::to_string(node->id), 10);
    idTxt.setFillColor(sf::Color(150, 180, 220));
    auto ib = idTxt.getLocalBounds();
    idTxt.setOrigin({ib.position.x + ib.size.x / 2.f, ib.position.y});
    idTxt.setPosition({x, y + r + 3.f});
    window.draw(idTxt);
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
    sf::RenderWindow window(sf::VideoMode({W, H}), "Vault Heist: The Living Vault");
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("assets/fonts/arial.ttf") ||
                      font.openFromFile("../../assets/fonts/arial.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) std::cerr << "Warning: Arial Font could not be loaded!\n";

    // 20 Campaign Levels across 4 Wings (5 levels per Wing)
    const int TOTAL_LEVELS = 20;
    LevelInfo levels[20];

    // --- WING 1: THE BLUEPRINT WING (ACT 1: GENERAL BINARY TREE) ---
    levels[0] = LevelInfo(LEVEL_1_1, 1, "1.1 Traversals Protocol",
        "Inspect the blueprint tree and identify the correct Inorder traversal sequence!",
        "Click the correct traversal option [1-3] or press [1-3]", 15, true, 0);

    levels[1] = LevelInfo(LEVEL_1_2, 1, "1.2 Security Laser Mirror",
        "Reflect the binary tree branches so they align with the laser sensors!",
        "Click [Mirror Tree] then click [Confirm Alignment]", 15, false, 0);

    levels[2] = LevelInfo(LEVEL_1_3, 1, "1.3 Blueprint Reconstruction",
        "Recover the shredded tree using Preorder {10,5,2,7,15,12,20} and Inorder {2,5,7,10,12,15,20}!",
        "Click [Reconstruct Pre+In] then click [Validate Blueprint]", 15, false, 0);

    levels[3] = LevelInfo(LEVEL_1_4, 1, "1.4 Structural Analysis",
        "Determine the longest signal path (Tree Diameter) and tree Height!",
        "Select the correct tree diameter [1-3] or press [1-3]", 15, false, 0);

    levels[4] = LevelInfo(LEVEL_1_5, 1, "1.5 Boss: The Vault Nexus",
        "Find the Lowest Common Ancestor (LCA) of Node 20 and 40 to open the Wing 1 Gate!",
        "Click your answer [1-3] or press [1-3] to disarm the boss lock", 15, false, 0);

    // --- WING 2: THE SORTED WING (ACT 2: BINARY SEARCH TREE) ---
    levels[5] = LevelInfo(LEVEL_2_1, 2, "2.1 Duplicate Rejection Gateway",
        "Guards are injecting duplicate keys! Verify duplicate rejection and insert unique key.",
        "Click [Insert Duplicate 50] then [Insert Key 25] -> [Submit]", 15, false, 0);

    levels[6] = LevelInfo(LEVEL_2_2, 2, "2.2 Firewall Disarm (3 Cases)",
        "Disarm 3 firewall nodes in order: Case 1 (Leaf 80), Case 2 (Node 20), Case 3 (Node 30)!",
        "Click [Delete Leaf] -> [Delete 1-Child] -> [Delete 2-Child]", 20, false, 0);

    levels[7] = LevelInfo(LEVEL_2_3, 2, "2.3 Infiltration Sweep",
        "Locate the minimum key, maximum key, and search for security frequency 70!",
        "Click [Find Min], [Find Max], [Search 70] -> [Confirm Sweep]", 15, false, 0);

    levels[8] = LevelInfo(LEVEL_2_4, 2, "2.4 Key Recovery (Succ & Pred)",
        "Compromised node 40! Identify its Inorder Predecessor and Inorder Successor.",
        "Select the correct Predecessor/Successor pair [1-3]", 15, false, 0);

    levels[9] = LevelInfo(LEVEL_2_5, 2, "2.5 Boss: Range Vault Heist",
        "Extract all keys in Range [25..75] and isolate the 4th smallest key (k=4)!",
        "Click [Range Query] -> [Extract 4th-Smallest] -> [Unlock Gate]", 20, false, 0);

    // --- WING 3: THE ARCHITECT'S WING (ACT 3: AVL TREE) ---
    levels[10] = LevelInfo(LEVEL_3_1, 3, "3.1 Read the Badges",
        "Click EVERY unbalanced node (|BF| > 1). Red badge = violation!",
        "Click node circles on tree  |  [Enter] Submit  |  [R] Reset", 15, false, 0);

    levels[11] = LevelInfo(LEVEL_3_2, 3, "3.2 Single Rotations (LL / RR)",
        "Repair LL / RR imbalance. Click the correct rotation button below!",
        "Click buttons below  |  [R] Right-Rotate (LL)  |  [L] Left-Rotate (RR)", 20, false, 0);

    levels[12] = LevelInfo(LEVEL_3_3, 3, "3.3 Double Rotations (LR / RL)",
        "Identify and perform LR / RL double rotations.",
        "Click buttons below  |  [1] LR Double Rotation  |  [2] RL Double Rotation", 22, false, 0);

    levels[13] = LevelInfo(LEVEL_3_4, 3, "3.4 Auto Mode and Prediction",
        "Predict which rotation occurs before each incoming AVL insertion!",
        "Click prediction [1-5]  |  Click [Confirm & Insert]", 25, false, 0);

    levels[14] = LevelInfo(LEVEL_3_5, 3, "3.5 Boss: Order-Statistic Heist",
        "Steal the target treasure using subtree sizes in O(log n) steps!",
        "Click your answer button [1-4] or press [1-4]", 20, false, 0);

    // --- WING 4: THE HEAP FORTRESS (ACT 4: MAX HEAP) ---
    levels[15] = LevelInfo(LEVEL_4_1, 4, "4.1 Activate the Vault",
        "Build a Max Heap bottom-up! Red edges = violations. Sift-down non-leafs.",
        "Click [Sift-Down Next] or press [S]  |  Click [Validate]", 25, false, 0);

    levels[16] = LevelInfo(LEVEL_4_2, 4, "4.2 Repair the Security Locks",
        "Guards sabotaged the heap! Use Sift-Down to restore order.",
        "Click [Sift-Down Root] or press [D]  |  Click [Validate]", 20, false, 0);

    levels[17] = LevelInfo(LEVEL_4_3, 4, "4.3 Extract the Greatest Treasure",
        "Extract root maximum to sorted region, then Heapify remainder.",
        "Click [Extract Max] or [E]  |  Click [Heapify] or [H]", 25, false, 0);

    levels[18] = LevelInfo(LEVEL_4_4, 4, "4.4 Race Against the Guards",
        "Complete multiple Heap Sort iterations before the alarm meter fills!",
        "Click [Extract & Heapify] or press [E]", 20, false, 0);

    levels[19] = LevelInfo(LEVEL_4_5, 4, "4.5 Final Vault Boss",
        "Full Heap Sort! Build Max Heap [B], then Extract All [E] to unlock vault!",
        "Click buttons: [Build Max Heap] -> [Extract All] -> [Unlock Vault]", 30, false, 0);

    // Initial unlock: ONLY LEVEL 1.1 IS UNLOCKED AT THE START!
    levels[0].unlocked = true;

    // Puzzle state variables for Wing 1 and Wing 2
    bool act1Reconstructed = false;
    int act2Step21 = 0;
    int act2DelStage = 0;
    int act2BossStage = 0;

    // Game state
    int currentScreen = SCREEN_SPLASH;
    int currentLevelIdx = -1;
    std::string status = "Welcome to Vault Heist!";
    int alarmSteps = 0, score = 1000;
    float splashTimer = 0.0f;
    float dialAngle = 0.0f;
    bool soundEnabled = true;
    sf::Clock deltaClock;

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

    // Act 1 (General Binary Tree) State
    BinaryTree act1Tree;
    int act1Hl1 = -9999, act1Hl2 = -9999;
    std::string act1TraversalStr = "";
    auto resetAct1 = [&]() {
        act1Tree = BinaryTree();
        auto n50 = act1Tree.addNode(50, nullptr, true);
        auto n30 = act1Tree.addNode(30, n50, true);
        auto n70 = act1Tree.addNode(70, n50, false);
        act1Tree.addNode(20, n30, true);
        act1Tree.addNode(40, n30, false);
        act1Tree.addNode(60, n70, true);
        act1Tree.addNode(80, n70, false);
        act1Hl1 = -9999;
        act1Hl2 = -9999;
        act1TraversalStr = "";
    };
    resetAct1();

    // Act 2 (BST) State
    BST act2Bst;
    int act2Hl1 = -9999, act2Hl2 = -9999;
    std::string act2ExtraInfo = "";
    auto resetAct2 = [&]() {
        act2Bst = BST();
        int keys[] = { 50, 30, 70, 20, 40, 60, 80 };
        for (int k : keys) act2Bst.insert(k);
        act2Hl1 = -9999;
        act2Hl2 = -9999;
        act2ExtraInfo = "";
    };
    resetAct2();

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
        act1Reconstructed = false;
        act2Step21 = 0;
        act2DelStage = 0;
        act2BossStage = 0;

        // --- WING 1 (ACT 1: GENERAL BINARY TREE) ---
        if (lid == LEVEL_1_1) {
            resetAct1();
            status = "Level 1.1: Examine the Blueprint Tree! Identify the correct Inorder traversal sequence [1-3].";
        }
        else if (lid == LEVEL_1_2) {
            resetAct1();
            status = "Level 1.2: Security Laser Mirror. Click [Mirror Tree [M]], then click [Confirm Alignment [Enter]]!";
        }
        else if (lid == LEVEL_1_3) {
            act1Tree = BinaryTree();
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Level 1.3: Blueprint shredded! Click [Reconstruct Pre+In [R]], then click [Validate Blueprint [V]]!";
        }
        else if (lid == LEVEL_1_4) {
            resetAct1();
            status = "Level 1.4: Corridor analysis. What is the longest transmission path (Tree Diameter)? Select [1-3]!";
        }
        else if (lid == LEVEL_1_5) {
            resetAct1();
            act1Hl1 = 20; act1Hl2 = 40;
            status = "Level 1.5 BOSS: Find the Lowest Common Ancestor (LCA) of Node 20 and 40 [1-3] to unlock Wing 2!";
        }

        // --- WING 2 (ACT 2: BINARY SEARCH TREE) ---
        else if (lid == LEVEL_2_1) {
            resetAct2();
            act2Step21 = 0;
            status = "Level 2.1: Duplicate Rejection Gateway. Step 1: Click [Insert Duplicate 50] to test firewall!";
        }
        else if (lid == LEVEL_2_2) {
            resetAct2();
            act2DelStage = 0;
            status = "Level 2.2: Disarm 3 keys in order! Step 1: Click [Delete Leaf 80].";
        }
        else if (lid == LEVEL_2_3) {
            resetAct2();
            status = "Level 2.3: Infiltration Sweep. Click [Find Min], [Find Max], and [Search 70], then [Confirm Sweep]!";
        }
        else if (lid == LEVEL_2_4) {
            resetAct2();
            act2Hl1 = 40;
            status = "Level 2.4: Target node 40 is compromised! Select its Inorder Predecessor and Successor [1-3].";
        }
        else if (lid == LEVEL_2_5) {
            resetAct2();
            act2BossStage = 0;
            status = "Level 2.5 BOSS: Execute [Range Query [25..75]], then [Extract 4th-Smallest], then [Unlock Master Vault]!";
        }

        else if (lid == LEVEL_3_1) {
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

        // =====================================================================
        // --- WING 1: THE BLUEPRINT WING (ACT 1: GENERAL BINARY TREE) ---
        // =====================================================================
        if (lid == LEVEL_1_1) {
            if (actionCode == 1) { // Correct Inorder: 20 30 40 50 60 70 80
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                currentScreen = SCREEN_RESULT;
                status = "CORRECT! Inorder sequence (L, Root, R) decoded the security protocol!";
            } else {
                alarmSteps += 3;
                status = "WRONG! Inorder visits Left -> Root -> Right: 20 30 40 50 60 70 80. Alarm raised!";
            }
        }
        else if (lid == LEVEL_1_2) {
            if (actionCode == 1) { // Mirror
                act1Tree.mirror();
                act1Hl1 = -9999; act1Hl2 = -9999;
                status = "Tree mirrored! Children swapped recursively. Now click [Confirm Alignment [Enter]]!";
            } else if (actionCode == 2) { // Confirm
                bool isMirrored = (act1Tree.getRoot() && act1Tree.getRoot()->left && act1Tree.getRoot()->left->value == 70);
                if (isMirrored) {
                    levels[currentLevelIdx].stars = 3;
                    if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "MIRROR ALIGNED! Mirrored branches bypassed the laser grid!";
                } else {
                    alarmSteps += 2;
                    status = "Not aligned! Click [Mirror Tree [M]] first to reflect the branches!";
                }
            }
        }
        else if (lid == LEVEL_1_3) {
            if (actionCode == 1) { // Reconstruct
                int preArr[] = { 10, 5, 2, 7, 15, 12, 20 };
                int inArr[]  = { 2, 5, 7, 10, 12, 15, 20 };
                act1Tree.buildFromPreorderInorder(preArr, inArr, 7);
                act1Hl1 = 10;
                act1Reconstructed = true;
                status = "Reconstructed tree from Preorder + Inorder! Root is 10. Click [Validate Blueprint [V]]!";
            } else if (actionCode == 2) { // Validate
                if (act1Reconstructed && act1Tree.getRoot() && act1Tree.getRoot()->value == 10) {
                    levels[currentLevelIdx].stars = 3;
                    if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                    currentScreen = SCREEN_RESULT;
                    status = "BLUEPRINT RESTORED! Tree reconstruction from Pre+In verified in O(n)!";
                } else {
                    alarmSteps += 2;
                    status = "Shredded blueprint! Click [Reconstruct Pre+In [R]] first!";
                }
            }
        }
        else if (lid == LEVEL_1_4) {
            if (actionCode == 1) { // Correct: Diameter = 5 nodes
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                currentScreen = SCREEN_RESULT;
                status = "CORRECT! Diameter = 5 nodes (longest path between leaves). Depth Height = 3!";
            } else {
                alarmSteps += 3;
                status = "WRONG! Longest path traverses 5 nodes (e.g. 20 -> 30 -> 50 -> 70 -> 80). Alarm raised!";
            }
        }
        else if (lid == LEVEL_1_5) { // Boss Wing 1
            if (actionCode == 1) { // Node 30 is LCA of 20 and 40
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true; // Unlocks Level 2.1 (Wing 2)!
                currentScreen = SCREEN_RESULT;
                status = "ACT 1 BOSS CONQUERED! LCA(20, 40) = 30! WING 2 (ACT 2: BST) UNLOCKED!";
            } else {
                alarmSteps += 3;
                status = "WRONG! Node 30 is the deepest node that is ancestor to both 20 and 40!";
            }
        }

        // =====================================================================
        // --- WING 2: THE SORTED WING (ACT 2: BINARY SEARCH TREE) ---
        // =====================================================================
        else if (lid == LEVEL_2_1) {
            if (actionCode == 1) { // Insert Duplicate 50
                bool ok = act2Bst.insert(50);
                act2Hl1 = 50;
                act2Step21 = 1;
                status = "Duplicate key 50 REJECTED! (Course rule: duplicates banned). Now click [Insert Key 25].";
            } else if (actionCode == 2 && act2Step21 >= 1) { // Insert Key 25
                act2Bst.insert(25);
                act2Hl1 = 25;
                act2Step21 = 2;
                status = "Key 25 inserted! Valid BST: " + std::string(act2Bst.isValidBST() ? "YES" : "NO") + ". Click [Submit Gateway [Enter]].";
            } else if (actionCode == 3 && act2Step21 == 2) { // Submit
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                currentScreen = SCREEN_RESULT;
                status = "GATEWAY SECURED! Duplicate rejection and BST insertion properties verified!";
            } else {
                alarmSteps += 2;
                status = "Follow instructions: Click [Insert Duplicate 50], then [Insert Key 25], then [Submit]!";
            }
        }
        else if (lid == LEVEL_2_2) {
            if (actionCode == 1 && act2DelStage == 0) { // Delete Leaf 80
                act2Bst.deleteKey(80);
                act2Hl1 = -9999;
                act2DelStage = 1;
                status = "Case 1: Leaf 80 deleted cleanly! Next: Click [Delete Single-Child 20].";
            } else if (actionCode == 2 && act2DelStage == 1) { // Delete 20
                act2Bst.deleteKey(20);
                act2DelStage = 2;
                status = "Case 2: Node 20 deleted, child spliced! Next: Click [Delete Two-Child 30].";
            } else if (actionCode == 3 && act2DelStage == 2) { // Delete 30
                act2Bst.deleteKey(30);
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                currentScreen = SCREEN_RESULT;
                status = "ALL 3 DELETION CASES EXECUTED! (Leaf, 1 Child, 2 Children with Successor replacement)!";
            } else {
                alarmSteps += 2;
                status = "Delete in order: Step 1 (Leaf 80) -> Step 2 (Node 20) -> Step 3 (Node 30)!";
            }
        }
        else if (lid == LEVEL_2_3) {
            if (actionCode == 1) { // Min
                act2Hl1 = act2Bst.findMin();
                status = "findMin() = " + std::to_string(act2Hl1) + " (leftmost node highlighted)!";
            } else if (actionCode == 2) { // Max
                act2Hl2 = act2Bst.findMax();
                status = "findMax() = " + std::to_string(act2Hl2) + " (rightmost node highlighted)!";
            } else if (actionCode == 3) { // Search 70
                bool f = act2Bst.search(70);
                act2Hl1 = 70;
                status = "search(70) = FOUND in O(h) time! Click [Confirm Sweep [Enter]] to finish.";
            } else if (actionCode == 4) { // Confirm
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                currentScreen = SCREEN_RESULT;
                status = "SWEEP SUCCESS! BST search, findMin and findMax executed in O(log n)!";
            }
        }
        else if (lid == LEVEL_2_4) {
            if (actionCode == 1) { // Correct: Pred=30, Succ=50
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
                currentScreen = SCREEN_RESULT;
                status = "CORRECT! Node 40: Inorder Predecessor = 30, Inorder Successor = 50!";
            } else {
                alarmSteps += 3;
                status = "WRONG! Inorder sorted order is: ... 30, 40, 50 ...";
            }
        }
        else if (lid == LEVEL_2_5) { // Boss Wing 2
            if (actionCode == 1) { // Range
                int count = 0;
                int* rq = act2Bst.rangeQuery(25, 75, count);
                std::string s = "Range [25..75] (" + std::to_string(count) + " keys): ";
                for (int i = 0; i < count; ++i) s += std::to_string(rq[i]) + " ";
                delete[] rq;
                act2BossStage = 1;
                status = s + " | Next: Click [Extract 4th-Smallest].";
            } else if (actionCode == 2 && act2BossStage >= 1) { // kth-Smallest (k=4)
                int val = act2Bst.kthSmallest(4);
                act2Hl1 = val;
                act2BossStage = 2;
                status = "kthSmallest(k=4) = " + std::to_string(val) + "! Now click [Unlock Master Vault]!";
            } else if (actionCode == 3 && act2BossStage >= 2) { // Unlock
                levels[currentLevelIdx].stars = 3;
                if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true; // Unlocks Level 3.1 (Wing 3: AVL)!
                currentScreen = SCREEN_RESULT;
                status = "ACT 2 BOSS CONQUERED! RANGE & KTH EXTRACTED! WING 3 (ACT 3: AVL) UNLOCKED!";
            } else {
                alarmSteps += 2;
                status = "Click [Range Query], then [Extract 4th-Smallest], then [Unlock Master Vault]!";
            }
        }

        // =====================================================================
        // --- WING 3: THE ARCHITECT'S WING (ACT 3: AVL TREE) ---
        // =====================================================================
        else if (lid == LEVEL_3_1) {
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
                    swapInt(heapArr[2], heapArr[6]);
                    alarmSteps++;
                    buildStepIdx = 1;
                    hlIndex1 = 1; hlIndex2 = 3;
                    status = "Step 1: Swapped 25 with 80! Next: Sift-down node [1] (val 60). Click [Sift-Down Next]!";
                } else if (buildStepIdx == 1) {
                    swapInt(heapArr[1], heapArr[3]);
                    alarmSteps++;
                    buildStepIdx = 0;
                    hlIndex1 = 0; hlIndex2 = 3;
                    status = "Step 2: Swapped 60 with 90! Next: Sift-down root [0] (val 15). Click [Sift-Down Next]!";
                } else if (buildStepIdx == 0) {
                    swapInt(heapArr[0], heapArr[1]);
                    swapInt(heapArr[1], heapArr[4]);
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
                swapInt(heapArr[0], heapArr[1]);
                swapInt(heapArr[1], heapArr[3]);
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
                swapInt(heapArr[0], heapArr[activeHeapSize - 1]);
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
                        swapInt(heapArr[curr], heapArr[largest]);
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
                swapInt(heapArr[0], heapArr[activeHeapSize - 1]);
                sortedArr[sortedCount++] = maxVal;
                activeHeapSize--;
                int curr = 0;
                while (true) {
                    int l = 2*curr+1, r = 2*curr+2, largest = curr;
                    if (l < activeHeapSize && heapArr[l] > heapArr[largest]) largest = l;
                    if (r < activeHeapSize && heapArr[r] > heapArr[largest]) largest = r;
                    if (largest != curr) {
                        swapInt(heapArr[curr], heapArr[largest]);
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
                            swapInt(heapArr[curr], heapArr[largest]);
                            curr = largest;
                        } else break;
                    }
                }
                alarmSteps += 4;
                status = "Max Heap Built! Phase 2: Click [Extract Next Max] repeatedly!";
            } else if (actionCode == 2 && activeHeapSize > 0) { // Extract
                int maxVal = heapArr[0];
                swapInt(heapArr[0], heapArr[activeHeapSize - 1]);
                sortedArr[sortedCount++] = maxVal;
                activeHeapSize--;
                int curr = 0;
                while (true) {
                    int l = 2*curr+1, r = 2*curr+2, largest = curr;
                    if (l < activeHeapSize && heapArr[l] > heapArr[largest]) largest = l;
                    if (r < activeHeapSize && heapArr[r] > heapArr[largest]) largest = r;
                    if (largest != curr) {
                        swapInt(heapArr[curr], heapArr[largest]);
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

    auto triggerAct1Action = [&](int actionCode) {
        if (actionCode == 1) { // Traversals
            int cPre = 0, cIn = 0, cPost = 0, cLvl = 0;
            int* pre = act1Tree.preorderIterative(cPre);
            int* in  = act1Tree.inorderIterative(cIn);
            int* post = act1Tree.postorderIterative(cPost);
            int* lvl = act1Tree.levelOrder(cLvl);

            std::string preStr = "Pre: ", inStr = "In: ", postStr = "Post: ", lvlStr = "Level: ";
            for (int i = 0; i < cPre; ++i) preStr += std::to_string(pre[i]) + " ";
            for (int i = 0; i < cIn; ++i) inStr += std::to_string(in[i]) + " ";
            for (int i = 0; i < cPost; ++i) postStr += std::to_string(post[i]) + " ";
            for (int i = 0; i < cLvl; ++i) lvlStr += std::to_string(lvl[i]) + " ";

            act1TraversalStr = preStr + "\n" + inStr + "\n" + postStr + "\n" + lvlStr;
            status = "Traversals computed! (Preorder, Inorder, Postorder, Level-Order). Iterative matches Recursive!";
            delete[] pre; delete[] in; delete[] post; delete[] lvl;
        }
        else if (actionCode == 2) { // Mirror
            act1Tree.mirror();
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Tree mirrored! Children swapped recursively. Symmetric: " + std::string(act1Tree.isSymmetric() ? "YES" : "NO");
        }
        else if (actionCode == 3) { // Reconstruct
            int preArr[] = { 10, 5, 2, 7, 15, 12, 20 };
            int inArr[]  = { 2, 5, 7, 10, 12, 15, 20 };
            act1Tree.buildFromPreorderInorder(preArr, inArr, 7);
            act1Hl1 = 10; act1Hl2 = -9999;
            status = "Reconstructed tree from Preorder {10,5,2,7,15,12,20} + Inorder {2,5,7,10,12,15,20}!";
        }
        else if (actionCode == 4) { // Height & Diameter
            int h = act1Tree.height();
            int d = act1Tree.diameter();
            status = "Height = " + std::to_string(h) + " (leaf=1 convention)  |  Diameter = " + std::to_string(d) + " nodes";
        }
        else if (actionCode == 5) { // LCA & Path
            int a = 20, b = 40;
            if (!act1Tree.getRoot()) { status = "Tree is empty!"; return; }
            int lcaVal = act1Tree.lca(a, b);
            if (lcaVal == -1) {
                a = act1Tree.getRoot()->value;
                b = act1Tree.getRoot()->left ? act1Tree.getRoot()->left->value : a;
                lcaVal = act1Tree.lca(a, b);
            }
            act1Hl1 = a; act1Hl2 = b;
            int pLen = 0;
            int* path = act1Tree.pathTo(a, pLen);
            std::string pStr = "Path to " + std::to_string(a) + ": ";
            if (path) {
                for (int i = 0; i < pLen; ++i) pStr += std::to_string(path[i]) + (i+1 < pLen ? " -> " : "");
                delete[] path;
            }
            status = "LCA(" + std::to_string(a) + ", " + std::to_string(b) + ") = " + std::to_string(lcaVal) + "  |  " + pStr;
        }
        else if (actionCode == 6) { // Symmetric tree demo
            act1Tree = BinaryTree();
            auto r1 = act1Tree.addNode(1, nullptr, true);
            auto r2L = act1Tree.addNode(2, r1, true);
            auto r2R = act1Tree.addNode(2, r1, false);
            act1Tree.addNode(3, r2L, true);
            act1Tree.addNode(4, r2L, false);
            act1Tree.addNode(4, r2R, true);
            act1Tree.addNode(3, r2R, false);
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Loaded Symmetric Binary Tree! isSymmetric() = " + std::string(act1Tree.isSymmetric() ? "TRUE" : "FALSE");
        }
        else if (actionCode == 7) { // Reset
            resetAct1();
            status = "Reset to default Act 1 General Binary Tree.";
        }
    };

    auto triggerAct2Action = [&](int actionCode) {
        if (actionCode == 1) { // Insert
            int candidates[] = { 35, 65, 85, 25, 45, 50, 15 };
            static int insStep = 0;
            int key = candidates[insStep % 7];
            insStep++;
            bool ok = act2Bst.insert(key);
            act2Hl1 = key; act2Hl2 = -9999;
            if (ok) {
                status = "Inserted key " + std::to_string(key) + "! Height=" + std::to_string(act2Bst.getHeight()) + " Size=" + std::to_string(act2Bst.getSize()) + " ValidBST=" + (act2Bst.isValidBST() ? "YES" : "NO");
            } else {
                status = "Duplicate key " + std::to_string(key) + " REJECTED! (Course Rule: no duplicates)";
            }
        }
        else if (actionCode == 2) { // Delete
            static int delStep = 0;
            int delKey = -1;
            if (delStep == 0 && act2Bst.search(80)) delKey = 80;       // Case 1: Leaf
            else if (delStep == 1 && act2Bst.search(20)) delKey = 20;  // Case 2: 1 child
            else if (delStep == 2 && act2Bst.search(30)) delKey = 30;  // Case 3: 2 children
            else {
                delKey = act2Bst.findMin();
            }
            delStep = (delStep + 1) % 3;
            if (delKey != -1) {
                act2Bst.deleteKey(delKey);
                act2Hl1 = -9999; act2Hl2 = -9999;
                status = "Deleted key " + std::to_string(delKey) + "! Remaining size=" + std::to_string(act2Bst.getSize()) + ", ValidBST=" + (act2Bst.isValidBST() ? "YES" : "NO");
            } else {
                status = "BST is empty! Press [Z] to reset.";
            }
        }
        else if (actionCode == 3) { // Search
            int searchKeys[] = { 40, 99, 70, 25, 88 };
            static int sIdx = 0;
            int sk = searchKeys[sIdx % 5];
            sIdx++;
            bool found = act2Bst.search(sk);
            act2Hl1 = found ? sk : -9999;
            status = "search(" + std::to_string(sk) + ") = " + (found ? "FOUND (Node highlighted!)" : "NOT FOUND");
        }
        else if (actionCode == 4) { // Min / Max
            int mn = act2Bst.findMin();
            int mx = act2Bst.findMax();
            act2Hl1 = mn; act2Hl2 = mx;
            status = "findMin() = " + std::to_string(mn) + " (leftmost)  |  findMax() = " + std::to_string(mx) + " (rightmost)";
        }
        else if (actionCode == 5) { // Succ / Pred
            int target = 40;
            if (!act2Bst.search(target) && act2Bst.getRoot()) target = act2Bst.getRoot()->value;
            if (target != -1) {
                int sc = act2Bst.successor(target);
                int pd = act2Bst.predecessor(target);
                act2Hl1 = target;
                status = "Node " + std::to_string(target) + " -> Predecessor: " + (pd != -1 ? std::to_string(pd) : "None") + " | Successor: " + (sc != -1 ? std::to_string(sc) : "None");
            } else {
                status = "Tree is empty!";
            }
        }
        else if (actionCode == 6) { // Range Query
            int count = 0;
            int* rq = act2Bst.rangeQuery(25, 75, count);
            std::string rqStr = "Range [25, 75] (" + std::to_string(count) + " keys): ";
            if (rq) {
                for (int i = 0; i < count; ++i) rqStr += std::to_string(rq[i]) + (i+1 < count ? ", " : "");
                delete[] rq;
            } else {
                rqStr += "None";
            }
            status = rqStr;
            act2ExtraInfo = rqStr;
        }
        else if (actionCode == 7) { // kthSmallest
            static int k = 1;
            int ans = act2Bst.kthSmallest(k);
            act2Hl1 = ans;
            status = "kthSmallest(k = " + std::to_string(k) + ") = " + (ans != -1 ? std::to_string(ans) : "Out of bounds");
            k++;
            if (k > act2Bst.getSize()) k = 1;
        }
        else if (actionCode == 8) { // Degenerate chain
            act2Bst = BST();
            for (int i = 1; i <= 7; ++i) act2Bst.insert(i * 10);
            act2Hl1 = -9999; act2Hl2 = -9999;
            status = "Constructed Degenerate BST Chain (10..70)! Height = " + std::to_string(act2Bst.getHeight()) + " == Size = " + std::to_string(act2Bst.getSize()) + " | ValidBST = " + (act2Bst.isValidBST() ? "YES" : "NO");
        }
        else if (actionCode == 9) { // Reset
            resetAct2();
            status = "Reset to default Act 2 BST.";
        }
    };

    // =========================================================================
    // MAIN LOOP
    // =========================================================================
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;
        dialAngle += 50.f * dt;

        if (currentScreen == SCREEN_SPLASH) {
            splashTimer += dt;
            if (splashTimer >= 3.8f) {
                currentScreen = SCREEN_MAIN_MENU;
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        float mx = static_cast<float>(mPos.x);
        float my = static_cast<float>(mPos.y);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                } else if (kp->code == sf::Keyboard::Key::Escape) {
                    if (currentScreen == SCREEN_LEVEL_PLAY || currentScreen == SCREEN_RESULT) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                    } else if (currentScreen == SCREEN_CAMPAIGN_MAP || currentScreen == SCREEN_SANDBOX || currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO || currentScreen == SCREEN_LEADERBOARD || currentScreen == SCREEN_SETTINGS) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else {
                        window.close();
                    }
                }

                // Keyboard Shortcuts mapped to Level Actions
                if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    int lid = levels[currentLevelIdx].id;

                    // --- WING 1 (ACT 1: BINARY TREE) ---
                    if (lid == LEVEL_1_1) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }
                    else if (lid == LEVEL_1_2) {
                        if (kp->code == sf::Keyboard::Key::M) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Enter) triggerAction(2);
                    }
                    else if (lid == LEVEL_1_3) {
                        if (kp->code == sf::Keyboard::Key::R) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::V) triggerAction(2);
                    }
                    else if (lid == LEVEL_1_4) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }
                    else if (lid == LEVEL_1_5) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }

                    // --- WING 2 (ACT 2: BINARY SEARCH TREE) ---
                    else if (lid == LEVEL_2_1) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }
                    else if (lid == LEVEL_2_2) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }
                    else if (lid == LEVEL_2_3) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                        if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Num4) triggerAction(4);
                    }
                    else if (lid == LEVEL_2_4) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }
                    else if (lid == LEVEL_2_5) {
                        if (kp->code == sf::Keyboard::Key::Num1) triggerAction(1);
                        if (kp->code == sf::Keyboard::Key::Num2) triggerAction(2);
                        if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Num3) triggerAction(3);
                    }

                    // --- WING 3 (ACT 3: AVL TREE) ---
                    else if (lid == LEVEL_3_1) {
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

                    // --- WING 4 (ACT 4: MAX HEAP) ---
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

                if (currentScreen == SCREEN_MAIN_MENU) {
                    if (kp->code == sf::Keyboard::Key::Num1) { currentScreen = SCREEN_CAMPAIGN_MAP; status = "Heist Wings Overview"; }
                    if (kp->code == sf::Keyboard::Key::Num2) { status = "ENDLESS VAULT: Protocol Locked! Complete Campaign Wings 1-4 first."; }
                    if (kp->code == sf::Keyboard::Key::Num3) { currentScreen = SCREEN_SANDBOX; }
                    if (kp->code == sf::Keyboard::Key::Num4) { currentScreen = SCREEN_LEADERBOARD; }
                    if (kp->code == sf::Keyboard::Key::Num5) { currentScreen = SCREEN_SETTINGS; }
                    if (kp->code == sf::Keyboard::Key::Num6) { window.close(); }
                }

                if (currentScreen == SCREEN_ACT1_DEMO) {
                    if (kp->code == sf::Keyboard::Key::T) triggerAct1Action(1);
                    if (kp->code == sf::Keyboard::Key::M) triggerAct1Action(2);
                    if (kp->code == sf::Keyboard::Key::R) triggerAct1Action(3);
                    if (kp->code == sf::Keyboard::Key::H) triggerAct1Action(4);
                    if (kp->code == sf::Keyboard::Key::L) triggerAct1Action(5);
                    if (kp->code == sf::Keyboard::Key::S) triggerAct1Action(6);
                    if (kp->code == sf::Keyboard::Key::Z) triggerAct1Action(7);
                }

                if (currentScreen == SCREEN_ACT2_DEMO) {
                    if (kp->code == sf::Keyboard::Key::I) triggerAct2Action(1);
                    if (kp->code == sf::Keyboard::Key::D) triggerAct2Action(2);
                    if (kp->code == sf::Keyboard::Key::S) triggerAct2Action(3);
                    if (kp->code == sf::Keyboard::Key::M) triggerAct2Action(4);
                    if (kp->code == sf::Keyboard::Key::P) triggerAct2Action(5);
                    if (kp->code == sf::Keyboard::Key::R) triggerAct2Action(6);
                    if (kp->code == sf::Keyboard::Key::K) triggerAct2Action(7);
                    if (kp->code == sf::Keyboard::Key::C) triggerAct2Action(8);
                    if (kp->code == sf::Keyboard::Key::Z) triggerAct2Action(9);
                }
            }

            // Mouse Click Handling
            if (const auto* mc = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(mc->position.x);
                float my = static_cast<float>(mc->position.y);

                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                }
                // --- MAIN MENU CLICKS ---
                else if (currentScreen == SCREEN_MAIN_MENU) {
                    float btnX = W / 2.f - 180.f, btnW = 360.f, btnH = 43.f, startY = 225.f, gap = 50.f;

                    // Sound Toggle
                    if (isInside(mx, my, W - 140.f, 20.f, 110.f, 32.f)) {
                        soundEnabled = !soundEnabled;
                    }
                    // Button 0: Campaign // Heist Wings
                    if (isInside(mx, my, btnX, startY + 0 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Heist Wings Overview";
                    }
                    // Button 1: Endless Vault
                    if (isInside(mx, my, btnX, startY + 1 * gap, btnW, btnH)) {
                        status = "ENDLESS VAULT: Protocol Locked! Complete Campaign Wings 1-4 first.";
                    }
                    // Button 2: Sandbox Simulator
                    if (isInside(mx, my, btnX, startY + 2 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_SANDBOX;
                        avl = AVLTree();
                        int init[] = { 40, 20, 60, 10, 30, 50, 70 };
                        for (int v : init) avl.insert(v);
                        int hInit[] = { 90, 75, 80, 60, 50, 40, 70 };
                        activeHeapSize = 7;
                        for (int i = 0; i < 7; ++i) heapArr[i] = hInit[i];
                        sortedCount = 0;
                    }
                    // Button 3: Operator Leaderboard
                    if (isInside(mx, my, btnX, startY + 3 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_LEADERBOARD;
                    }
                    // Button 4: System Settings
                    if (isInside(mx, my, btnX, startY + 4 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_SETTINGS;
                    }
                    // Button 5: Quit Terminal
                    if (isInside(mx, my, btnX, startY + 5 * gap, btnW, btnH)) {
                        window.close();
                    }
                }

                // --- CAMPAIGN MAP CLICKS ---
                else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    // Back button
                    if (isInside(mx, my, W - 220.f, 20.f, 170.f, 36.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    }

                    // Wing 1 (Act 1 BT) Levels 0..4
                    for (int i = 0; i < 5; ++i) {
                        float bx = 60.f;
                        float by = 125.f + i * 52.f;
                        if (isInside(mx, my, bx, by, 455.f, 44.f)) {
                            if (levels[i].unlocked) startLevel(i);
                            else status = "Security Lock Active: Clear previous level to decrypt access!";
                            break;
                        }
                    }
                    // Wing 2 (Act 2 BST) Levels 5..9
                    for (int i = 5; i < 10; ++i) {
                        float bx = 585.f;
                        float by = 125.f + (i - 5) * 52.f;
                        if (isInside(mx, my, bx, by, 455.f, 44.f)) {
                            if (levels[i].unlocked) startLevel(i);
                            else status = "Wing 2 Locked: Conquer Wing 1 Boss (1.5) to decrypt access!";
                            break;
                        }
                    }
                    // Wing 3 (Act 3 AVL) Levels 10..14
                    for (int i = 10; i < 15; ++i) {
                        float bx = 60.f;
                        float by = 465.f + (i - 10) * 52.f;
                        if (isInside(mx, my, bx, by, 455.f, 44.f)) {
                            if (levels[i].unlocked) startLevel(i);
                            else status = "Wing 3 Locked: Conquer Wing 2 Boss (2.5) to decrypt access!";
                            break;
                        }
                    }
                    // Wing 4 (Act 4 Heap) Levels 15..19
                    for (int i = 15; i < 20; ++i) {
                        float bx = 585.f;
                        float by = 465.f + (i - 15) * 52.f;
                        if (isInside(mx, my, bx, by, 455.f, 44.f)) {
                            if (levels[i].unlocked) startLevel(i);
                            else status = "Wing 4 Locked: Conquer Wing 3 Boss (3.5) to decrypt access!";
                            break;
                        }
                    }
                }

                // --- LEADERBOARD CLICKS ---
                else if (currentScreen == SCREEN_LEADERBOARD) {
                    if (isInside(mx, my, W / 2.f - 130.f, 510.f, 260.f, 44.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                }

                // --- SETTINGS CLICKS ---
                else if (currentScreen == SCREEN_SETTINGS) {
                    if (isInside(mx, my, W / 2.f - 130.f, 420.f, 260.f, 40.f)) {
                        soundEnabled = !soundEnabled;
                    }
                    if (isInside(mx, my, W / 2.f - 130.f, 510.f, 260.f, 44.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
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

                    // --- WING 1 (ACT 1: BINARY TREE) ---
                    if (lid == LEVEL_1_1) {
                        if (isInside(mx, my, 60.f, 535.f, 310.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 395.f, 535.f, 310.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 310.f, 48.f)) triggerAction(3);
                    }
                    else if (lid == LEVEL_1_2) {
                        if (isInside(mx, my, 280.f, 535.f, 250.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 570.f, 535.f, 250.f, 48.f)) triggerAction(2);
                    }
                    else if (lid == LEVEL_1_3) {
                        if (isInside(mx, my, 280.f, 535.f, 250.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 570.f, 535.f, 250.f, 48.f)) triggerAction(2);
                    }
                    else if (lid == LEVEL_1_4) {
                        if (isInside(mx, my, 80.f, 535.f, 290.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 405.f, 535.f, 290.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 290.f, 48.f)) triggerAction(3);
                    }
                    else if (lid == LEVEL_1_5) {
                        if (isInside(mx, my, 80.f, 535.f, 290.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 405.f, 535.f, 290.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 290.f, 48.f)) triggerAction(3);
                    }

                    // --- WING 2 (ACT 2: BINARY SEARCH TREE) ---
                    else if (lid == LEVEL_2_1) {
                        if (isInside(mx, my, 80.f, 535.f, 290.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 405.f, 535.f, 290.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 290.f, 48.f)) triggerAction(3);
                    }
                    else if (lid == LEVEL_2_2) {
                        if (isInside(mx, my, 80.f, 535.f, 290.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 405.f, 535.f, 290.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 290.f, 48.f)) triggerAction(3);
                    }
                    else if (lid == LEVEL_2_3) {
                        if (isInside(mx, my, 50.f, 535.f, 230.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 305.f, 535.f, 230.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 560.f, 535.f, 230.f, 48.f)) triggerAction(3);
                        if (isInside(mx, my, 815.f, 535.f, 235.f, 48.f)) triggerAction(4);
                    }
                    else if (lid == LEVEL_2_4) {
                        if (isInside(mx, my, 80.f, 535.f, 290.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 405.f, 535.f, 290.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 290.f, 48.f)) triggerAction(3);
                    }
                    else if (lid == LEVEL_2_5) {
                        if (isInside(mx, my, 80.f, 535.f, 290.f, 48.f)) triggerAction(1);
                        if (isInside(mx, my, 405.f, 535.f, 290.f, 48.f)) triggerAction(2);
                        if (isInside(mx, my, 730.f, 535.f, 290.f, 48.f)) triggerAction(3);
                    }

                    // LEVEL 3.1: Tree node clicks
                    else if (lid == LEVEL_3_1) {
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

                // --- ACT 1 EXPLORER CLICKS ---
                else if (currentScreen == SCREEN_ACT1_DEMO) {
                    if (isInside(mx, my, 40.f,  495.f, 190.f, 40.f)) triggerAct1Action(1);
                    if (isInside(mx, my, 250.f, 495.f, 190.f, 40.f)) triggerAct1Action(2);
                    if (isInside(mx, my, 460.f, 495.f, 190.f, 40.f)) triggerAct1Action(3);
                    if (isInside(mx, my, 670.f, 495.f, 190.f, 40.f)) triggerAct1Action(4);
                    if (isInside(mx, my, 880.f, 495.f, 180.f, 40.f)) triggerAct1Action(5);
                    if (isInside(mx, my, 250.f, 545.f, 190.f, 40.f)) triggerAct1Action(6);
                    if (isInside(mx, my, 460.f, 545.f, 190.f, 40.f)) triggerAct1Action(7);
                    if (isInside(mx, my, 670.f, 545.f, 190.f, 40.f) || isInside(mx, my, W - 200.f, 15.f, 170.f, 34.f)) { currentScreen = SCREEN_CAMPAIGN_MAP; status = "Heist Wings Overview"; }
                }

                // --- ACT 2 EXPLORER CLICKS ---
                else if (currentScreen == SCREEN_ACT2_DEMO) {
                    if (isInside(mx, my, 40.f,  495.f, 190.f, 40.f)) triggerAct2Action(1);
                    if (isInside(mx, my, 250.f, 495.f, 190.f, 40.f)) triggerAct2Action(2);
                    if (isInside(mx, my, 460.f, 495.f, 190.f, 40.f)) triggerAct2Action(3);
                    if (isInside(mx, my, 670.f, 495.f, 190.f, 40.f)) triggerAct2Action(4);
                    if (isInside(mx, my, 880.f, 495.f, 180.f, 40.f)) triggerAct2Action(5);
                    if (isInside(mx, my, 40.f,  545.f, 190.f, 40.f)) triggerAct2Action(6);
                    if (isInside(mx, my, 250.f, 545.f, 190.f, 40.f)) triggerAct2Action(7);
                    if (isInside(mx, my, 460.f, 545.f, 190.f, 40.f)) triggerAct2Action(8);
                    if (isInside(mx, my, 670.f, 545.f, 190.f, 40.f)) triggerAct2Action(9);
                    if (isInside(mx, my, 880.f, 545.f, 180.f, 40.f) || isInside(mx, my, W - 200.f, 15.f, 170.f, 34.f)) { currentScreen = SCREEN_CAMPAIGN_MAP; status = "Heist Wings Overview"; }
                }
            }
        }

        // =====================================================================
        // RENDERING
        // =====================================================================
        window.clear(sf::Color(10, 16, 28));
        drawBackgroundGrid(window, W, H);

        // 0. SPLASH SCREEN (IMAGE 1)
        if (currentScreen == SCREEN_SPLASH) {
            float cx = W / 2.f, cy = 210.f;

            // Outer cyan circle
            sf::CircleShape outerRing(58.f);
            outerRing.setOrigin({58.f, 58.f});
            outerRing.setPosition({cx, cy});
            outerRing.setFillColor(sf::Color::Transparent);
            outerRing.setOutlineColor(sf::Color(56, 189, 248));
            outerRing.setOutlineThickness(2.2f);
            window.draw(outerRing);

            // Inner cyan circle
            sf::CircleShape innerRing(22.f);
            innerRing.setOrigin({22.f, 22.f});
            innerRing.setPosition({cx, cy});
            innerRing.setFillColor(sf::Color::Transparent);
            innerRing.setOutlineColor(sf::Color(56, 189, 248));
            innerRing.setOutlineThickness(2.0f);
            window.draw(innerRing);

            // Rotating radial spokes between inner and mid
            for (int i = 0; i < 8; ++i) {
                float rad = (-dialAngle * 0.8f + i * 45.f) * (3.14159265f / 180.f);
                float x1 = cx + std::cos(rad) * 22.f;
                float y1 = cy + std::sin(rad) * 22.f;
                float x2 = cx + std::cos(rad) * 31.f;
                float y2 = cy + std::sin(rad) * 31.f;
                sf::Vertex sp[] = { sf::Vertex({x1, y1}, sf::Color(56, 189, 248)), sf::Vertex({x2, y2}, sf::Color(56, 189, 248)) };
                window.draw(sp, 2, sf::PrimitiveType::Lines);
            }

            // 10 Rotating Gold Dots on dial perimeter
            for (int i = 0; i < 10; ++i) {
                float rad = (dialAngle + i * 36.f) * (3.14159265f / 180.f);
                float px = cx + std::cos(rad) * 45.f;
                float py = cy + std::sin(rad) * 45.f;
                sf::CircleShape dot(3.8f);
                dot.setOrigin({3.8f, 3.8f});
                dot.setPosition({px, py});
                dot.setFillColor(sf::Color(245, 185, 40));
                window.draw(dot);
            }

            // Central cyan hub
            sf::CircleShape hub(3.f);
            hub.setOrigin({3.f, 3.f});
            hub.setPosition({cx, cy});
            hub.setFillColor(sf::Color(56, 189, 248));
            window.draw(hub);

            // Left vertical accent bar
            sf::RectangleShape bar({4.f, 44.f});
            bar.setFillColor(sf::Color(56, 189, 248));
            bar.setPosition({W / 2.f - 170.f, 305.f});
            window.draw(bar);

            // Title: VAULT HEIST
            sf::Text vh(font, "VAULT HEIST", 44);
            vh.setFillColor(sf::Color(245, 185, 40));
            vh.setStyle(sf::Text::Bold);
            vh.setPosition({W / 2.f - 150.f, 300.f});
            window.draw(vh);

            // Typewriter subtitle: "The Living Vault"
            std::string targetSub = "The Living Vault";
            int charsToShow = minInt(16, static_cast<int>(splashTimer * 6.5f));
            std::string typed = targetSub.substr(0, charsToShow);
            if (charsToShow < 16 && (static_cast<int>(splashTimer * 4.0f) % 2 == 0)) typed += "_";
            sf::Text sub(font, typed, 20);
            sub.setFillColor(sf::Color(230, 240, 255));
            sub.setPosition({W / 2.f - 150.f, 355.f});
            window.draw(sub);

            // Progress bar
            sf::RectangleShape pTrack({260.f, 6.f});
            pTrack.setPosition({W / 2.f - 130.f, 415.f});
            pTrack.setFillColor(sf::Color(22, 35, 55));
            window.draw(pTrack);

            float prog = minFloat(1.0f, splashTimer / 3.2f);
            sf::RectangleShape pFill({260.f * prog, 6.f});
            pFill.setPosition({W / 2.f - 130.f, 415.f});
            pFill.setFillColor(sf::Color(56, 189, 248));
            window.draw(pFill);

            // Subtitle below progress bar
            sf::Text subProg(font, "Cracking security protocols...", 12);
            subProg.setFillColor(sf::Color(140, 170, 200));
            auto pb = subProg.getLocalBounds();
            subProg.setOrigin({pb.position.x + pb.size.x / 2.f, pb.position.y});
            subProg.setPosition({W / 2.f, 435.f});
            window.draw(subProg);

            // Skip hint
            sf::Text skipTxt(font, "[Press any key or click to skip]", 12);
            skipTxt.setFillColor(sf::Color(95, 120, 150));
            auto skb = skipTxt.getLocalBounds();
            skipTxt.setOrigin({skb.position.x + skb.size.x / 2.f, skb.position.y});
            skipTxt.setPosition({W / 2.f, 650.f});
            window.draw(skipTxt);
        }

        // 1. MAIN MENU (IMAGE 2)
        if (currentScreen == SCREEN_MAIN_MENU) {
            // Watermarks
            drawTreeWatermark(window, 160.f, 410.f);
            drawTreeWatermark(window, 940.f, 410.f);

            // Top center sub-header
            sf::Text topHeader(font, "SECURE OPERATIONAL INTERFACE // V 2.4", 13);
            topHeader.setFillColor(sf::Color(56, 189, 248));
            auto thb = topHeader.getLocalBounds();
            topHeader.setOrigin({thb.position.x + thb.size.x / 2.f, thb.position.y});
            topHeader.setPosition({W / 2.f, 25.f});
            window.draw(topHeader);

            // Top right sound button
            drawButton(window, font, soundEnabled ? "SOUND ON" : "SOUND OFF",
                       W - 140.f, 20.f, 110.f, 32.f,
                       sf::Color(16, 26, 42), sf::Color(200, 230, 255), sf::Color(70, 130, 200), 12,
                       isInside(mx, my, W - 140.f, 20.f, 110.f, 32.f));

            // Main Title
            sf::Text title(font, "VAULT HEIST", 48);
            title.setFillColor(sf::Color(245, 185, 40));
            title.setStyle(sf::Text::Bold);
            auto tb = title.getLocalBounds();
            title.setOrigin({tb.position.x + tb.size.x / 2.f, tb.position.y});
            title.setPosition({W / 2.f, 95.f});
            window.draw(title);

            sf::Text sub(font, "THE LIVING VAULT", 19);
            sub.setFillColor(sf::Color::White);
            sub.setStyle(sf::Text::Bold);
            auto sb = sub.getLocalBounds();
            sub.setOrigin({sb.position.x + sb.size.x / 2.f, sb.position.y});
            sub.setPosition({W / 2.f, 158.f});
            window.draw(sub);

            // Underline
            sf::RectangleShape uLine({220.f, 2.5f});
            uLine.setPosition({W / 2.f - 110.f, 188.f});
            uLine.setFillColor(sf::Color(56, 189, 248));
            window.draw(uLine);

            // Stacked Terminal Buttons
            float btnX = W / 2.f - 180.f, btnW = 360.f, btnH = 43.f, startY = 225.f, gap = 50.f;

            drawButton(window, font, "CAMPAIGN // HEIST WINGS", btnX, startY + 0 * gap, btnW, btnH,
                       sf::Color(24, 28, 44), sf::Color::White, sf::Color(245, 185, 40), 15,
                       isInside(mx, my, btnX, startY + 0 * gap, btnW, btnH));

            drawButton(window, font, "ENDLESS VAULT", btnX, startY + 1 * gap, btnW, btnH,
                       sf::Color(18, 26, 42), sf::Color(200, 220, 245), sf::Color(70, 130, 200), 15,
                       isInside(mx, my, btnX, startY + 1 * gap, btnW, btnH));

            drawButton(window, font, "SANDBOX SIMULATOR", btnX, startY + 2 * gap, btnW, btnH,
                       sf::Color(18, 28, 46), sf::Color::White, sf::Color(59, 130, 246), 15,
                       isInside(mx, my, btnX, startY + 2 * gap, btnW, btnH));

            drawButton(window, font, "OPERATOR LEADERBOARD", btnX, startY + 3 * gap, btnW, btnH,
                       sf::Color(18, 26, 42), sf::Color::White, sf::Color(80, 140, 210), 15,
                       isInside(mx, my, btnX, startY + 3 * gap, btnW, btnH));

            drawButton(window, font, "SYSTEM SETTINGS", btnX, startY + 4 * gap, btnW, btnH,
                       sf::Color(18, 26, 42), sf::Color::White, sf::Color(80, 140, 210), 15,
                       isInside(mx, my, btnX, startY + 4 * gap, btnW, btnH));

            drawButton(window, font, "QUIT TERMINAL", btnX, startY + 5 * gap, btnW, btnH,
                       sf::Color(32, 20, 26), sf::Color(255, 180, 180), sf::Color(230, 60, 60), 15,
                       isInside(mx, my, btnX, startY + 5 * gap, btnW, btnH));

            sf::Text hint(font, "[Click buttons or press 1-6]  |  [Esc] Quit", 13);
            hint.setFillColor(sf::Color(140, 160, 190));
            auto hb = hint.getLocalBounds();
            hint.setOrigin({hb.position.x + hb.size.x / 2.f, hb.position.y});
            hint.setPosition({W / 2.f, 560.f});
            window.draw(hint);
        }

        // 2. CAMPAIGN MAP (HEIST WINGS)
        if (currentScreen == SCREEN_CAMPAIGN_MAP) {
            sf::Text mapTitle(font, "HEIST CAMPAIGN // 4 OPERATIONAL WINGS", 24);
            mapTitle.setFillColor(sf::Color(245, 185, 40));
            mapTitle.setStyle(sf::Text::Bold);
            mapTitle.setPosition({40.f, 15.f});
            window.draw(mapTitle);

            sf::Text mapSub(font, "SEQUENTIAL INFILTRATION: Complete levels in order to breach and decrypt subsequent wings!", 12);
            mapSub.setFillColor(sf::Color(100, 190, 240));
            mapSub.setPosition({40.f, 46.f});
            window.draw(mapSub);

            drawButton(window, font, "[Esc] Back to Menu", W - 220.f, 16.f, 170.f, 34.f,
                       sf::Color(24, 28, 44), sf::Color::White, sf::Color(100, 140, 200), 13,
                       isInside(mx, my, W - 220.f, 16.f, 170.f, 34.f));

            // --- WING 1 CARD: GENERAL BINARY TREE (Levels 0..4) ---
            sf::RectangleShape c1({495.f, 325.f});
            c1.setPosition({40.f, 75.f});
            c1.setFillColor(sf::Color(14, 24, 40));
            c1.setOutlineThickness(1.8f);
            c1.setOutlineColor(sf::Color(56, 189, 248));
            window.draw(c1);

            sf::Text t1(font, "WING 1: THE BLUEPRINT WING (ACT 1: BINARY TREE)", 15);
            t1.setFillColor(sf::Color(56, 189, 248));
            t1.setStyle(sf::Text::Bold);
            t1.setPosition({60.f, 88.f});
            window.draw(t1);

            for (int i = 0; i < 5; ++i) {
                float bx = 60.f;
                float by = 125.f + i * 52.f;
                bool unl = levels[i].unlocked;
                sf::Color bg = unl ? sf::Color(20, 38, 62) : sf::Color(16, 20, 30);
                sf::Color tc = unl ? sf::Color::White : sf::Color(90, 105, 125);
                sf::Color ot = unl ? sf::Color(56, 189, 248) : sf::Color(40, 50, 65);
                std::string starS = "";
                for (int s = 0; s < levels[i].stars; ++s) starS += " *";
                std::string lbl = levels[i].title + (unl ? starS : "  [LOCKED]");
                drawButton(window, font, lbl, bx, by, 455.f, 44.f, bg, tc, ot, 13, isInside(mx, my, bx, by, 455.f, 44.f));
            }

            // --- WING 2 CARD: BINARY SEARCH TREE (Levels 5..9) ---
            sf::RectangleShape c2({495.f, 325.f});
            c2.setPosition({565.f, 75.f});
            c2.setFillColor(sf::Color(22, 18, 38));
            c2.setOutlineThickness(1.8f);
            c2.setOutlineColor(sf::Color(168, 85, 247));
            window.draw(c2);

            sf::Text t2(font, "WING 2: THE SORTED WING (ACT 2: BST)", 15);
            t2.setFillColor(sf::Color(168, 85, 247));
            t2.setStyle(sf::Text::Bold);
            t2.setPosition({585.f, 88.f});
            window.draw(t2);

            for (int i = 5; i < 10; ++i) {
                float bx = 585.f;
                float by = 125.f + (i - 5) * 52.f;
                bool unl = levels[i].unlocked;
                sf::Color bg = unl ? sf::Color(35, 25, 58) : sf::Color(16, 20, 30);
                sf::Color tc = unl ? sf::Color::White : sf::Color(90, 105, 125);
                sf::Color ot = unl ? sf::Color(168, 85, 247) : sf::Color(40, 50, 65);
                std::string starS = "";
                for (int s = 0; s < levels[i].stars; ++s) starS += " *";
                std::string lbl = levels[i].title + (unl ? starS : "  [LOCKED]");
                drawButton(window, font, lbl, bx, by, 455.f, 44.f, bg, tc, ot, 13, isInside(mx, my, bx, by, 455.f, 44.f));
            }

            // --- WING 3 CARD: THE ARCHITECT'S WING (AVL) (Levels 10..14) ---
            sf::RectangleShape c3({495.f, 325.f});
            c3.setPosition({40.f, 415.f});
            c3.setFillColor(sf::Color(14, 22, 38));
            c3.setOutlineThickness(1.8f);
            c3.setOutlineColor(sf::Color(80, 160, 240));
            window.draw(c3);

            sf::Text t3(font, "WING 3: ARCHITECT'S WING (ACT 3: AVL)", 15);
            t3.setFillColor(sf::Color(80, 160, 240));
            t3.setStyle(sf::Text::Bold);
            t3.setPosition({60.f, 428.f});
            window.draw(t3);

            for (int i = 10; i < 15; ++i) {
                float bx = 60.f;
                float by = 465.f + (i - 10) * 52.f;
                bool unl = levels[i].unlocked;
                sf::Color bg = unl ? sf::Color(25, 45, 75) : sf::Color(16, 20, 30);
                sf::Color tc = unl ? sf::Color::White : sf::Color(90, 105, 125);
                sf::Color ot = unl ? sf::Color(80, 150, 230) : sf::Color(40, 50, 65);
                std::string starS = "";
                for (int s = 0; s < levels[i].stars; ++s) starS += " *";
                std::string lbl = levels[i].title + (unl ? starS : "  [LOCKED]");
                drawButton(window, font, lbl, bx, by, 455.f, 44.f, bg, tc, ot, 13, isInside(mx, my, bx, by, 455.f, 44.f));
            }

            // --- WING 4 CARD: THE HEAP FORTRESS (ACT 4: MAX HEAP) (Levels 15..19) ---
            sf::RectangleShape c4({495.f, 325.f});
            c4.setPosition({565.f, 415.f});
            c4.setFillColor(sf::Color(32, 24, 18));
            c4.setOutlineThickness(1.8f);
            c4.setOutlineColor(sf::Color(245, 160, 40));
            window.draw(c4);

            sf::Text t4(font, "WING 4: THE HEAP FORTRESS (ACT 4: HEAP)", 15);
            t4.setFillColor(sf::Color(245, 160, 40));
            t4.setStyle(sf::Text::Bold);
            t4.setPosition({585.f, 428.f});
            window.draw(t4);

            for (int i = 15; i < 20; ++i) {
                float bx = 585.f;
                float by = 465.f + (i - 15) * 52.f;
                bool unl = levels[i].unlocked;
                sf::Color bg = unl ? sf::Color(45, 35, 24) : sf::Color(16, 20, 30);
                sf::Color tc = unl ? sf::Color::White : sf::Color(90, 105, 125);
                sf::Color ot = unl ? sf::Color(230, 150, 40) : sf::Color(40, 50, 65);
                std::string starS = "";
                for (int s = 0; s < levels[i].stars; ++s) starS += " *";
                std::string lbl = levels[i].title + (unl ? starS : "  [LOCKED]");
                drawButton(window, font, lbl, bx, by, 455.f, 44.f, bg, tc, ot, 13, isInside(mx, my, bx, by, 455.f, 44.f));
            }
        }

        // 8. LEADERBOARD SCREEN
        if (currentScreen == SCREEN_LEADERBOARD) {
            sf::RectangleShape modal({540.f, 440.f});
            modal.setPosition({W / 2.f - 270.f, 150.f});
            modal.setFillColor(sf::Color(14, 22, 38));
            modal.setOutlineThickness(2.2f);
            modal.setOutlineColor(sf::Color(245, 185, 40));
            window.draw(modal);

            sf::Text lbTitle(font, "OPERATOR LEADERBOARD // TOP HEISTS", 18);
            lbTitle.setFillColor(sf::Color(245, 185, 40));
            lbTitle.setStyle(sf::Text::Bold);
            auto lbt = lbTitle.getLocalBounds();
            lbTitle.setOrigin({lbt.position.x + lbt.size.x / 2.f, lbt.position.y});
            lbTitle.setPosition({W / 2.f, 180.f});
            window.draw(lbTitle);

            sf::Text lbEntries(font,
                "1. CIPHER_SHADOW   - 9,850 pts  [ACT 4: HEAP MASTER]\n"
                "2. ZERO_COOL       - 8,420 pts  [ACT 3: AVL ARCHITECT]\n"
                "3. GHOST_IN_SHELL  - 7,650 pts  [ACT 2: BST INFILTRATOR]\n"
                "4. BLUEPRINT_CHIEF - 6,900 pts  [ACT 1: BT SPECIALIST]\n"
                "5. CURRENT_PLAYER  - 1,000 pts  [ACTIVE HEIST SESSION]", 15);
            lbEntries.setFillColor(sf::Color(210, 230, 255));
            lbEntries.setPosition({W / 2.f - 230.f, 250.f});
            window.draw(lbEntries);

            drawButton(window, font, "[RETURN TO TERMINAL [Esc]]", W / 2.f - 130.f, 510.f, 260.f, 44.f,
                       sf::Color(25, 40, 65), sf::Color::White, sf::Color(245, 185, 40), 14,
                       isInside(mx, my, W / 2.f - 130.f, 510.f, 260.f, 44.f));
        }

        // 9. SETTINGS SCREEN
        if (currentScreen == SCREEN_SETTINGS) {
            sf::RectangleShape modal({540.f, 440.f});
            modal.setPosition({W / 2.f - 270.f, 150.f});
            modal.setFillColor(sf::Color(14, 22, 38));
            modal.setOutlineThickness(2.2f);
            modal.setOutlineColor(sf::Color(56, 189, 248));
            window.draw(modal);

            sf::Text stTitle(font, "SYSTEM SETTINGS // CONFIGURATION", 18);
            stTitle.setFillColor(sf::Color(56, 189, 248));
            stTitle.setStyle(sf::Text::Bold);
            auto stt = stTitle.getLocalBounds();
            stTitle.setOrigin({stt.position.x + stt.size.x / 2.f, stt.position.y});
            stTitle.setPosition({W / 2.f, 180.f});
            window.draw(stTitle);

            sf::Text stEntries(font,
                "Tactical Display: Cyber Blueprint High-Contrast\n"
                "Resolution: 1100 x 780 (Fixed 60 FPS Engine)\n"
                "Academic DS Policy: 100% Compliant (No STL Containers)\n"
                "Algorithms: In-house Pointer Manipulations & Traversals\n\n"
                "Audio FX Feedback:", 14);
            stEntries.setFillColor(sf::Color(200, 225, 250));
            stEntries.setPosition({W / 2.f - 230.f, 240.f});
            window.draw(stEntries);

            drawButton(window, font, soundEnabled ? "[AUDIO FX: ENABLED]" : "[AUDIO FX: MUTED]",
                       W / 2.f - 130.f, 420.f, 260.f, 40.f,
                       sf::Color(22, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 14,
                       isInside(mx, my, W / 2.f - 130.f, 420.f, 260.f, 40.f));

            drawButton(window, font, "[RETURN TO TERMINAL [Esc]]", W / 2.f - 130.f, 510.f, 260.f, 44.f,
                       sf::Color(25, 40, 65), sf::Color::White, sf::Color(56, 189, 248), 14,
                       isInside(mx, my, W / 2.f - 130.f, 510.f, 260.f, 44.f));
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

            float ratio = minFloat(1.0f, static_cast<float>(alarmSteps) / alarmCap);
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
            if (lid >= LEVEL_1_1 && lid <= LEVEL_1_5) {
                drawTreeNode(window, font, act1Tree.getRoot(), W / 2.f, 115.f, 200.f, act1Hl1, act1Hl2);
            }
            else if (lid >= LEVEL_2_1 && lid <= LEVEL_2_5) {
                drawTreeNode(window, font, act2Bst.getRoot(), W / 2.f, 115.f, 200.f, act2Hl1, act2Hl2);
            }
            else if (lid == LEVEL_3_1) {
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

            // --- WING 1 (ACT 1: BINARY TREE) ---
            if (lid == LEVEL_1_1) {
                sf::Text qTitle(font, "PUZZLE: Select the correct Inorder Traversal sequence (Left -> Root -> Right):", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] 20  30  40  50  60  70  80", 60.f, 535.f, 310.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 60.f, 535.f, 310.f, 48.f));
                drawButton(window, font, "[2] 50  30  20  40  70  60  80", 395.f, 535.f, 310.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 395.f, 535.f, 310.f, 48.f));
                drawButton(window, font, "[3] 20  40  30  60  80  70  50", 730.f, 535.f, 310.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 730.f, 535.f, 310.f, 48.f));
            }
            else if (lid == LEVEL_1_2) {
                sf::Text qTitle(font, "SECURITY LASER MIRROR: Reflect tree branches recursively to align with receiver:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Mirror Tree [M]", 280.f, 535.f, 250.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 280.f, 535.f, 250.f, 48.f));
                drawButton(window, font, "Confirm Alignment [Enter]", 570.f, 535.f, 250.f, 48.f, sf::Color(25, 55, 45), sf::Color::White, sf::Color(100, 220, 140), 15, isInside(mx, my, 570.f, 535.f, 250.f, 48.f));
            }
            else if (lid == LEVEL_1_3) {
                sf::Text qTitle(font, "SHREDDED BLUEPRINT: Pre: 10,5,2,7,15,12,20  |  In: 2,5,7,10,12,15,20. Rebuild in O(n):", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "Reconstruct Tree [R]", 280.f, 535.f, 250.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 280.f, 535.f, 250.f, 48.f));
                drawButton(window, font, "Validate Blueprint [V]", 570.f, 535.f, 250.f, 48.f, sf::Color(25, 55, 45), sf::Color::White, sf::Color(100, 220, 140), 15, isInside(mx, my, 570.f, 535.f, 250.f, 48.f));
            }
            else if (lid == LEVEL_1_4) {
                sf::Text qTitle(font, "TRANSMISSION PATH: What is the tree diameter (longest path between any 2 leaves)?", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Diameter = 5 nodes", 80.f, 535.f, 290.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 80.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[2] Diameter = 3 nodes", 405.f, 535.f, 290.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 405.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[3] Diameter = 7 nodes", 730.f, 535.f, 290.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 730.f, 535.f, 290.f, 48.f));
            }
            else if (lid == LEVEL_1_5) {
                sf::Text qTitle(font, "ACT 1 BOSS: Target nodes are 20 and 40 (gold). Find Lowest Common Ancestor (LCA):", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Node 30 (LCA)", 80.f, 535.f, 290.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 80.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[2] Node 50 (Root)", 405.f, 535.f, 290.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 405.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[3] Node 20", 730.f, 535.f, 290.f, 48.f, sf::Color(20, 38, 62), sf::Color::White, sf::Color(56, 189, 248), 15, isInside(mx, my, 730.f, 535.f, 290.f, 48.f));
            }

            // --- WING 2 (ACT 2: BINARY SEARCH TREE) ---
            else if (lid == LEVEL_2_1) {
                sf::Text qTitle(font, "GATEWAY PROTOCOL: Step " + std::to_string(act2Step21 + 1) + "/3. Test duplicate rejection firewall:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Insert Duplicate 50", 80.f, 535.f, 290.f, 48.f, act2Step21 >= 1 ? sf::Color(25, 55, 40) : sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 80.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[2] Insert Valid Key 25", 405.f, 535.f, 290.f, 48.f, act2Step21 >= 2 ? sf::Color(25, 55, 40) : sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 405.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[3] Submit Gateway [Enter]", 730.f, 535.f, 290.f, 48.f, sf::Color(45, 30, 70), sf::Color::White, sf::Color(245, 185, 40), 15, isInside(mx, my, 730.f, 535.f, 290.f, 48.f));
            }
            else if (lid == LEVEL_2_2) {
                sf::Text qTitle(font, "3-CASE DELETION: Stage " + std::to_string(act2DelStage + 1) + "/3. Disarm keys in order:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Delete Leaf 80", 80.f, 535.f, 290.f, 48.f, act2DelStage >= 1 ? sf::Color(25, 55, 40) : sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 80.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[2] Delete Single-Child 20", 405.f, 535.f, 290.f, 48.f, act2DelStage >= 2 ? sf::Color(25, 55, 40) : sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 405.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[3] Delete Two-Child 30", 730.f, 535.f, 290.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 730.f, 535.f, 290.f, 48.f));
            }
            else if (lid == LEVEL_2_3) {
                sf::Text qTitle(font, "DATA SWEEP: Probe BST extremes and target keys in O(h) time:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Find Min", 50.f, 535.f, 230.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 50.f, 535.f, 230.f, 48.f));
                drawButton(window, font, "[2] Find Max", 305.f, 535.f, 230.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 305.f, 535.f, 230.f, 48.f));
                drawButton(window, font, "[3] Search 70", 560.f, 535.f, 230.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 560.f, 535.f, 230.f, 48.f));
                drawButton(window, font, "[4] Confirm Sweep [Enter]", 815.f, 535.f, 235.f, 48.f, sf::Color(45, 30, 70), sf::Color::White, sf::Color(245, 185, 40), 14, isInside(mx, my, 815.f, 535.f, 235.f, 48.f));
            }
            else if (lid == LEVEL_2_4) {
                sf::Text qTitle(font, "TARGET KEY 40 (Gold): Find its Inorder Predecessor and Inorder Successor:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Pred = 30, Succ = 50", 80.f, 535.f, 290.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 80.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[2] Pred = 20, Succ = 60", 405.f, 535.f, 290.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 405.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[3] Pred = 50, Succ = 30", 730.f, 535.f, 290.f, 48.f, sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 730.f, 535.f, 290.f, 48.f));
            }
            else if (lid == LEVEL_2_5) {
                sf::Text qTitle(font, "ACT 2 BOSS: Stage " + std::to_string(act2BossStage + 1) + "/3. Extract data to breach vault:", 16);
                qTitle.setFillColor(sf::Color(255, 215, 100));
                qTitle.setPosition({50.f, 485.f});
                window.draw(qTitle);

                drawButton(window, font, "[1] Range Query [25..75]", 80.f, 535.f, 290.f, 48.f, act2BossStage >= 1 ? sf::Color(25, 55, 40) : sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 80.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[2] Extract 4th-Smallest", 405.f, 535.f, 290.f, 48.f, act2BossStage >= 2 ? sf::Color(25, 55, 40) : sf::Color(35, 25, 58), sf::Color::White, sf::Color(168, 85, 247), 15, isInside(mx, my, 405.f, 535.f, 290.f, 48.f));
                drawButton(window, font, "[3] Unlock Master Vault [Enter]", 730.f, 535.f, 290.f, 48.f, sf::Color(45, 30, 70), sf::Color::White, sf::Color(245, 185, 40), 14, isInside(mx, my, 730.f, 535.f, 290.f, 48.f));
            }
            else if (lid == LEVEL_3_1) {
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

        // 6. ACT 1 DEMO SCREEN (GENERAL BINARY TREE)
        if (currentScreen == SCREEN_ACT1_DEMO) {
            sf::Text header(font, "ACT 1: GENERAL BINARY TREE (THE BLUEPRINT WING)", 24);
            header.setFillColor(sf::Color(120, 220, 255));
            header.setPosition({30.f, 15.f});
            window.draw(header);

            drawButton(window, font, "[Esc] Heist Wings", W - 200.f, 15.f, 170.f, 34.f,
                       sf::Color(20, 36, 60), sf::Color::White, sf::Color(56, 189, 248), 13,
                       isInside(mx, my, W - 200.f, 15.f, 170.f, 34.f));

            sf::Text subhead(font, "Traversals (Iterative + Recursive) | Reconstruction | Height & Diameter | LCA & Paths | Mirror", 13);
            subhead.setFillColor(sf::Color(170, 190, 220));
            subhead.setPosition({30.f, 48.f});
            window.draw(subhead);

            std::string symStr = act1Tree.isSymmetric() ? "YES" : "NO";
            sf::Text metrics(font, "Nodes: " + std::to_string(act1Tree.getSize()) +
                                   "  |  Height: " + std::to_string(act1Tree.height()) + " (leaf=1)" +
                                   "  |  Diameter: " + std::to_string(act1Tree.diameter()) + " nodes" +
                                   "  |  Symmetric: " + symStr, 14);
            metrics.setFillColor(sf::Color(255, 215, 80));
            metrics.setPosition({30.f, 75.f});
            window.draw(metrics);

            drawTreeNode(window, font, act1Tree.getRoot(), W / 2.f, 130.f, 210.f, act1Hl1, act1Hl2);

            drawButton(window, font, "[T] Traversals",     40.f,  495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 40.f,  495.f, 190.f, 40.f));
            drawButton(window, font, "[M] Mirror Tree",    250.f, 495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 250.f, 495.f, 190.f, 40.f));
            drawButton(window, font, "[R] Rebuild Pre+In", 460.f, 495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 460.f, 495.f, 190.f, 40.f));
            drawButton(window, font, "[H] Height & Diam",  670.f, 495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 670.f, 495.f, 190.f, 40.f));
            drawButton(window, font, "[L] LCA & Path",     880.f, 495.f, 180.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 880.f, 495.f, 180.f, 40.f));

            drawButton(window, font, "[S] Symmetry Demo",  250.f, 545.f, 190.f, 40.f, sf::Color(25, 55, 65), sf::Color::White, sf::Color(70, 180, 200), 14, isInside(mx, my, 250.f, 545.f, 190.f, 40.f));
            drawButton(window, font, "[Z] Reset Tree",     460.f, 545.f, 190.f, 40.f, sf::Color(45, 35, 65), sf::Color::White, sf::Color(170, 110, 220), 14, isInside(mx, my, 460.f, 545.f, 190.f, 40.f));
            drawButton(window, font, "[Esc] Heist Wings",  670.f, 545.f, 190.f, 40.f, sf::Color(35, 35, 45), sf::Color::White, sf::Color(120, 120, 140), 14, isInside(mx, my, 670.f, 545.f, 190.f, 40.f));

            sf::RectangleShape box({W - 60.f, 140.f});
            box.setPosition({30.f, H - 170.f});
            box.setFillColor(sf::Color(14, 20, 34));
            box.setOutlineThickness(1.5f);
            box.setOutlineColor(sf::Color(70, 90, 130));
            window.draw(box);

            sf::Text statusTxt(font, status, 15);
            statusTxt.setFillColor(sf::Color(255, 220, 120));
            statusTxt.setPosition({45.f, H - 160.f});
            window.draw(statusTxt);

            if (!act1TraversalStr.empty()) {
                sf::Text travTxt(font, act1TraversalStr, 12);
                travTxt.setFillColor(sf::Color(180, 230, 255));
                travTxt.setPosition({45.f, H - 128.f});
                window.draw(travTxt);
            }

            sf::Text cHint(font, "Click buttons above or use keyboard shortcuts [T, M, R, H, L, S, Z]  |  [Esc] Back", 12);
            cHint.setFillColor(sf::Color(160, 180, 210));
            cHint.setPosition({45.f, H - 28.f});
            window.draw(cHint);
        }

        // 7. ACT 2 DEMO SCREEN (BINARY SEARCH TREE)
        if (currentScreen == SCREEN_ACT2_DEMO) {
            sf::Text header(font, "ACT 2: BINARY SEARCH TREE (THE SORTED WING)", 24);
            header.setFillColor(sf::Color(180, 140, 255));
            header.setPosition({30.f, 15.f});
            window.draw(header);

            drawButton(window, font, "[Esc] Heist Wings", W - 200.f, 15.f, 170.f, 34.f,
                       sf::Color(32, 22, 54), sf::Color::White, sf::Color(168, 85, 247), 13,
                       isInside(mx, my, W - 200.f, 15.f, 170.f, 34.f));

            sf::Text subhead(font, "Insert (No Dups) | 3 Delete Cases | Search | Min/Max | Successor/Predecessor | Range Query | kthSmallest", 13);
            subhead.setFillColor(sf::Color(170, 190, 220));
            subhead.setPosition({30.f, 48.f});
            window.draw(subhead);

            std::string validBstStr = act2Bst.isValidBST() ? "YES" : "NO";
            sf::Text metrics(font, "Nodes: " + std::to_string(act2Bst.getSize()) +
                                   "  |  Height: " + std::to_string(act2Bst.getHeight()) + " (leaf=1)" +
                                   "  |  Valid BST: " + validBstStr +
                                   "  |  Min: " + std::to_string(act2Bst.findMin()) +
                                   "  |  Max: " + std::to_string(act2Bst.findMax()), 14);
            metrics.setFillColor(sf::Color(255, 215, 80));
            metrics.setPosition({30.f, 75.f});
            window.draw(metrics);

            drawTreeNode(window, font, act2Bst.getRoot(), W / 2.f, 130.f, 210.f, act2Hl1, act2Hl2);

            drawButton(window, font, "[I] Insert Key",       40.f,  495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 40.f,  495.f, 190.f, 40.f));
            drawButton(window, font, "[D] Delete (3 Cases)", 250.f, 495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 250.f, 495.f, 190.f, 40.f));
            drawButton(window, font, "[S] Search Key",       460.f, 495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 460.f, 495.f, 190.f, 40.f));
            drawButton(window, font, "[M] Min & Max",        670.f, 495.f, 190.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 670.f, 495.f, 190.f, 40.f));
            drawButton(window, font, "[P] Succ & Pred",      880.f, 495.f, 180.f, 40.f, sf::Color(25, 45, 75), sf::Color::White, sf::Color(90, 150, 230), 14, isInside(mx, my, 880.f, 495.f, 180.f, 40.f));

            drawButton(window, font, "[R] Range [25..75]",   40.f,  545.f, 190.f, 40.f, sf::Color(25, 55, 65), sf::Color::White, sf::Color(70, 180, 200), 14, isInside(mx, my, 40.f,  545.f, 190.f, 40.f));
            drawButton(window, font, "[K] kthSmallest",      250.f, 545.f, 190.f, 40.f, sf::Color(25, 55, 65), sf::Color::White, sf::Color(70, 180, 200), 14, isInside(mx, my, 250.f, 545.f, 190.f, 40.f));
            drawButton(window, font, "[C] Degenerate",       460.f, 545.f, 190.f, 40.f, sf::Color(45, 35, 65), sf::Color::White, sf::Color(170, 110, 220), 14, isInside(mx, my, 460.f, 545.f, 190.f, 40.f));
            drawButton(window, font, "[Z] Reset Tree",       670.f, 545.f, 190.f, 40.f, sf::Color(45, 35, 65), sf::Color::White, sf::Color(170, 110, 220), 14, isInside(mx, my, 670.f, 545.f, 190.f, 40.f));
            drawButton(window, font, "[Esc] Heist Wings",    880.f, 545.f, 180.f, 40.f, sf::Color(35, 35, 45), sf::Color::White, sf::Color(120, 120, 140), 14, isInside(mx, my, 880.f, 545.f, 180.f, 40.f));

            sf::RectangleShape box({W - 60.f, 140.f});
            box.setPosition({30.f, H - 170.f});
            box.setFillColor(sf::Color(14, 20, 34));
            box.setOutlineThickness(1.5f);
            box.setOutlineColor(sf::Color(70, 90, 130));
            window.draw(box);

            sf::Text statusTxt(font, status, 15);
            statusTxt.setFillColor(sf::Color(255, 220, 120));
            statusTxt.setPosition({45.f, H - 160.f});
            window.draw(statusTxt);

            if (!act2ExtraInfo.empty()) {
                sf::Text extraTxt(font, act2ExtraInfo, 13);
                extraTxt.setFillColor(sf::Color(180, 255, 210));
                extraTxt.setPosition({45.f, H - 128.f});
                window.draw(extraTxt);
            }

            sf::Text cHint(font, "Click buttons above or use keyboard shortcuts [I, D, S, M, P, R, K, C, Z]  |  [Esc] Back", 12);
            cHint.setFillColor(sf::Color(160, 180, 210));
            cHint.setPosition({45.f, H - 28.f});
            window.draw(cHint);
        }

        window.display();
    }

    deleteManualTree(manualTree);
    return 0;
}
