#include "Graph.h"
#include "LevelManager.h"
#include "Renderer.h"
#include "ScoreManager.h"
#include "Validator.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>
#include <string>

using namespace EscapeNetwork;

namespace {

// Screen state constants (no enum / enum class allowed)
static const int SCREEN_MAIN_MENU    = 0;
static const int SCREEN_INSTRUCTIONS = 1;
static const int SCREEN_CAMPAIGN_MAP = 2;
static const int SCREEN_LEVEL_PLAY   = 3;

struct CampaignEntry {
    int levelId;
    int act;
    const char* title;
    const char* conceptText;
    bool completed;
    int stars;
};

bool loadFontWithFallback(sf::Font& font) {
    const char* candidates[] = {
        "assets/fonts/arial.ttf",
        "../assets/fonts/arial.ttf",
        "../../assets/fonts/arial.ttf",
        "../../../assets/fonts/arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
    };
    for (const char* path : candidates) {
        if (font.openFromFile(path)) return true;
    }
    return false;
}

} // namespace

int main() {
    constexpr unsigned int WIN_W = 1280;
    constexpr unsigned int WIN_H = 800;

    sf::RenderWindow window(sf::VideoMode({WIN_W, WIN_H}), "Vault Heist: The Escape Network");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!loadFontWithFallback(font)) {
        std::cerr << "Warning: Could not load a UI font; text may not render.\n";
    }

    Renderer renderer;
    LevelManager levelManager;
    ScoreManager scoreManager;
    Validator validator;

    static const int CAMPAIGN_COUNT = 10;
    CampaignEntry campaign[CAMPAIGN_COUNT] = {
        { 11, 1, "1.1 Map the City",        "Representations & Degrees",     false, 0 },
        { 12, 1, "1.2 Scout the Districts", "BFS, DFS, Components & Cycles", false, 0 },
        { 21, 2, "2.1 Fastest Escape",      "Dijkstra (Binary Heap)",        false, 0 },
        { 22, 2, "2.2 Toll Trap",           "Bellman-Ford & Neg. Cycles",    false, 0 },
        { 31, 3, "3.1 Build the Tunnel",    "Kruskal + Union-Find MST",      false, 0 },
        { 32, 3, "3.2 Power the District",  "Prim's MST (Binary Heap)",      false, 0 },
        { 41, 4, "4.1 Schedule the Heist",  "Kahn's Topological Sort",       false, 0 },
        { 42, 4, "4.2 Crew Assignment",     "Bipartite BFS Two-Colouring",   false, 0 },
        { 51, 5, "5.1 Find the Weak Point", "Bridges & Articulation Points", false, 0 },
        { 52, 5, "5.2 Escape Capacity",     "Edmonds-Karp Max Flow",         false, 0 }
    };

    int currentScreen = SCREEN_MAIN_MENU;
    int activeCampaignIdx = -1;
    Graph activeGraph;
    LevelMetadata activeMeta;
    GraphVisuals activeVisuals;
    int selectedVertexId = -1;
    std::string statusText = "Select a mission from the Campaign Map to begin.";

    auto openMission = [&](int idx) {
        if (idx < 0 || idx >= CAMPAIGN_COUNT) return;
        Graph loadedGraph;
        LevelMetadata loadedMeta;
        std::string err;
        if (!levelManager.loadLevelById(campaign[idx].levelId, loadedGraph, loadedMeta, err)) {
            statusText = "Error loading level: " + err;
            return;
        }
        activeCampaignIdx = idx;
        activeGraph = loadedGraph;
        activeMeta = loadedMeta;
        activeVisuals.resizeFor(activeGraph);
        selectedVertexId = -1;
        scoreManager.reset();
        validator.reset();
        statusText = activeMeta.startStatus.empty() ? activeMeta.objective : activeMeta.startStatus;
        currentScreen = SCREEN_LEVEL_PLAY;
    };

    while (window.isOpen()) {
        sf::Vector2i mp = sf::Mouse::getPosition(window);
        float mx = static_cast<float>(mp.x);
        float my = static_cast<float>(mp.y);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Escape) {
                    if (currentScreen == SCREEN_LEVEL_PLAY) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                    } else if (currentScreen == SCREEN_CAMPAIGN_MAP || currentScreen == SCREEN_INSTRUCTIONS) {
                        currentScreen = SCREEN_MAIN_MENU;
                    } else {
                        window.close();
                    }
                }
            }

            if (const auto* mc = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mc->button != sf::Mouse::Button::Left) continue;
                float cx = static_cast<float>(mc->position.x);
                float cy = static_cast<float>(mc->position.y);

                if (currentScreen == SCREEN_MAIN_MENU) {
                    float bx = WIN_W * 0.5f - 180.f;
                    if (Renderer::isInside(cx, cy, bx, 310.f, 360.f, 54.f)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                    } else if (Renderer::isInside(cx, cy, bx, 384.f, 360.f, 54.f)) {
                        currentScreen = SCREEN_INSTRUCTIONS;
                    } else if (Renderer::isInside(cx, cy, bx, 458.f, 360.f, 54.f)) {
                        window.close();
                    }
                } else if (currentScreen == SCREEN_INSTRUCTIONS) {
                    if (Renderer::isInside(cx, cy, WIN_W * 0.5f - 120.f, 680.f, 240.f, 48.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                } else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    if (Renderer::isInside(cx, cy, 1050.f, 28.f, 190.f, 40.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                    for (int i = 0; i < CAMPAIGN_COUNT; ++i) {
                        int actIdx = i / 2;
                        int colIdx = i % 2;
                        float rowY = 120.f + static_cast<float>(actIdx) * 124.f;
                        float cardX = (colIdx == 0) ? 50.f : 655.f;
                        if (Renderer::isInside(cx, cy, cardX, rowY + 34.f, 575.f, 66.f)) {
                            openMission(i);
                            break;
                        }
                    }
                } else if (currentScreen == SCREEN_LEVEL_PLAY) {
                    if (Renderer::isInside(cx, cy, 1060.f, 22.f, 185.f, 38.f)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                    } else if (Renderer::isInside(cx, cy, 860.f, 22.f, 180.f, 38.f)) {
                        openMission(activeCampaignIdx);
                    } else {
                        int hitV = renderer.hitTestVertex(activeGraph, cx, cy, 40.f, 110.f);
                        if (hitV >= 0) {
                            selectedVertexId = hitV;
                            activeVisuals.resizeFor(activeGraph);
                            if (activeVisuals.vertexStates && hitV < activeVisuals.vertexCount) {
                                activeVisuals.vertexStates[hitV] = VS_CURRENT;
                            }
                            const Vertex* vPtr = activeGraph.getVertex(hitV);
                            const auto& nbrs = activeGraph.getNeighbors(hitV);
                            statusText = "Selected [" + std::to_string(hitV) + "] " +
                                         (vPtr ? vPtr->label : "") +
                                         " | Out-degree / neighbors: " + std::to_string(nbrs.size());
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(11, 17, 30));

        // =====================================================================
        // 1. MAIN MENU
        // =====================================================================
        if (currentScreen == SCREEN_MAIN_MENU) {
            sf::Text title(font, "VAULT HEIST: THE ESCAPE NETWORK", 38);
            title.setFillColor(sf::Color(245, 195, 55));
            auto tb = title.getLocalBounds();
            title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y});
            title.setPosition({WIN_W * 0.5f, 120.f});
            window.draw(title);

            sf::Text sub(font, "Deliverable 02 // Graph Algorithms Campaign (5 Acts, 10 Missions)", 18);
            sub.setFillColor(sf::Color(165, 190, 225));
            auto sb = sub.getLocalBounds();
            sub.setOrigin({sb.position.x + sb.size.x * 0.5f, sb.position.y});
            sub.setPosition({WIN_W * 0.5f, 185.f});
            window.draw(sub);

            float bx = WIN_W * 0.5f - 180.f;
            Renderer::drawButton(window, font, "Start Campaign", bx, 310.f, 360.f, 54.f,
                                 Renderer::isInside(mx, my, bx, 310.f, 360.f, 54.f),
                                 sf::Color(28, 72, 118), sf::Color(70, 175, 245), 17);
            Renderer::drawButton(window, font, "Instructions", bx, 384.f, 360.f, 54.f,
                                 Renderer::isInside(mx, my, bx, 384.f, 360.f, 54.f),
                                 sf::Color(30, 48, 76), sf::Color(90, 145, 220), 17);
            Renderer::drawButton(window, font, "Exit", bx, 458.f, 360.f, 54.f,
                                 Renderer::isInside(mx, my, bx, 458.f, 360.f, 54.f),
                                 sf::Color(85, 32, 38), sf::Color(220, 85, 90), 17);

            sf::Text footer(font, "Click an option or press [Esc] to quit", 13);
            footer.setFillColor(sf::Color(125, 145, 175));
            auto fb = footer.getLocalBounds();
            footer.setOrigin({fb.position.x + fb.size.x * 0.5f, fb.position.y});
            footer.setPosition({WIN_W * 0.5f, 560.f});
            window.draw(footer);
        }

        // =====================================================================
        // 2. INSTRUCTIONS
        // =====================================================================
        else if (currentScreen == SCREEN_INSTRUCTIONS) {
            Renderer::drawPanel(window, 140.f, 80.f, 1000.f, 560.f,
                                sf::Color(17, 25, 42), sf::Color(70, 140, 220));
            sf::Text hdr(font, "MISSION BRIEFING & CONTROLS", 26);
            hdr.setFillColor(sf::Color(245, 195, 55));
            hdr.setPosition({180.f, 110.f});
            window.draw(hdr);

            std::string body =
                "1. Navigate 5 Acts (10 missions) from Vault District to the Airfield.\n"
                "2. Click vertices/edges on the graph canvas to inspect or execute decisions.\n"
                "3. Use Run, Step, and Restart controls during missions.\n"
                "4. Visual Cues: Grey = Unvisited | Gold (*) = Current | Blue (?) = Frontier\n"
                "   Teal (~) = Visited | Green (OK) = Accepted | Red (X) = Rejected.\n"
                "5. Heat & Stars: Minimize mistakes and path cost to earn 3 stars!\n"
                "6. Press [Esc] anytime to return to the previous menu.";
            sf::Text txt(font, body, 16);
            txt.setFillColor(sf::Color(215, 225, 242));
            txt.setPosition({180.f, 180.f});
            txt.setLineSpacing(1.5f);
            window.draw(txt);

            Renderer::drawButton(window, font, "Back to Menu [Esc]", WIN_W * 0.5f - 120.f, 680.f, 240.f, 48.f,
                                 Renderer::isInside(mx, my, WIN_W * 0.5f - 120.f, 680.f, 240.f, 48.f));
        }

        // =====================================================================
        // 3. CAMPAIGN MAP (5 ACTS, 10 MISSIONS)
        // =====================================================================
        else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
            sf::Text hdr(font, "ESCAPE NETWORK // CAMPAIGN MAP (5 ACTS, 10 MISSIONS)", 24);
            hdr.setFillColor(sf::Color(245, 195, 55));
            hdr.setPosition({50.f, 26.f});
            window.draw(hdr);

            Renderer::drawButton(window, font, "Back to Menu [Esc]", 1050.f, 28.f, 190.f, 40.f,
                                 Renderer::isInside(mx, my, 1050.f, 28.f, 190.f, 40.f));

            const char* actTitles[5] = {
                "ACT 1: RECONNAISSANCE (Representations, BFS/DFS, Components, Cycles)",
                "ACT 2: SHORTEST ESCAPE (Dijkstra & Bellman-Ford)",
                "ACT 3: INFRASTRUCTURE MST (Kruskal + Union-Find & Prim)",
                "ACT 4: COORDINATION (Topological Sort & Bipartite Two-Colouring)",
                "ACT 5: BOTTLENECKS & FLOW (Bridges/Articulation & Edmonds-Karp)"
            };

            for (int a = 0; a < 5; ++a) {
                float rowY = 120.f + static_cast<float>(a) * 124.f;
                sf::Text actHdr(font, actTitles[a], 14);
                actHdr.setFillColor(sf::Color(80, 185, 245));
                actHdr.setPosition({50.f, rowY + 6.f});
                window.draw(actHdr);

                for (int c = 0; c < 2; ++c) {
                    int idx = a * 2 + c;
                    const CampaignEntry& entry = campaign[idx];
                    float cardX = (c == 0) ? 50.f : 655.f;
                    bool hov = Renderer::isInside(mx, my, cardX, rowY + 34.f, 575.f, 66.f);

                    std::string badge = entry.completed
                        ? ("[" + std::to_string(entry.stars) + "/3 *]")
                        : "[READY]";
                    std::string label = std::string(entry.title) + "  -  " + entry.conceptText + "   " + badge;

                    Renderer::drawButton(window, font, label, cardX, rowY + 34.f, 575.f, 66.f, hov,
                                         sf::Color(22, 34, 56), sf::Color(70, 130, 200), 14);
                }
            }
        }

        // =====================================================================
        // 4. LEVEL PLAY SHELL (Renderer + StatePanel + Controls)
        // =====================================================================
        else if (currentScreen == SCREEN_LEVEL_PLAY) {
            sf::Text hdr(font, activeMeta.title + "  //  " + activeMeta.conceptText, 20);
            hdr.setFillColor(sf::Color(245, 195, 55));
            hdr.setPosition({40.f, 22.f});
            window.draw(hdr);

            sf::Text obj(font, activeMeta.objective, 13);
            obj.setFillColor(sf::Color(195, 210, 235));
            obj.setPosition({40.f, 54.f});
            window.draw(obj);

            Renderer::drawButton(window, font, "Restart", 860.f, 22.f, 180.f, 38.f,
                                 Renderer::isInside(mx, my, 860.f, 22.f, 180.f, 38.f));
            Renderer::drawButton(window, font, "Campaign Map [Esc]", 1060.f, 22.f, 185.f, 38.f,
                                 Renderer::isInside(mx, my, 1060.f, 22.f, 185.f, 38.f));

            // Graph Canvas Panel
            Renderer::drawPanel(window, 40.f, 110.f, 840.f, 520.f,
                                sf::Color(16, 23, 38), sf::Color(60, 110, 175));
            int hovV = renderer.hitTestVertex(activeGraph, mx, my, 40.f, 110.f);
            renderer.drawGraph(window, font, activeGraph, activeVisuals, 40.f, 110.f, hovV);

            // State Panel (Right)
            Renderer::drawPanel(window, 900.f, 110.f, 345.f, 520.f,
                                sf::Color(18, 26, 44), sf::Color(80, 135, 205));
            sf::Text spHdr(font, "ALGORITHM STATE PANEL", 15);
            spHdr.setFillColor(sf::Color(80, 195, 245));
            spHdr.setPosition({920.f, 128.f});
            window.draw(spHdr);

            std::string stats = "Vertices (V): " + std::to_string(activeGraph.getVertexCount()) +
                                "   Edges (E): " + std::to_string(activeGraph.getEdgeCount()) +
                                "\nHeat Used: " + std::to_string(scoreManager.getHeatUsed()) +
                                "   Mistakes: " + std::to_string(scoreManager.getMistakeCount());
            if (selectedVertexId >= 0) {
                stats += "\nSelected Vertex: " + std::to_string(selectedVertexId) + "\nNeighbors: ";
                const auto& nbrs = activeGraph.getNeighbors(selectedVertexId);
                for (std::size_t i = 0; i < nbrs.size(); ++i) {
                    if (i > 0) stats += ", ";
                    stats += std::to_string(nbrs[i].to);
                }
            }
            sf::Text spTxt(font, stats, 13);
            spTxt.setFillColor(sf::Color(220, 230, 248));
            spTxt.setPosition({920.f, 165.f});
            spTxt.setLineSpacing(1.4f);
            window.draw(spTxt);

            // Bottom Status & Controls Bar
            Renderer::drawPanel(window, 40.f, 650.f, 1205.f, 120.f,
                                sf::Color(18, 25, 42), sf::Color(70, 95, 140));
            sf::Text stTxt(font, statusText, 15);
            stTxt.setFillColor(sf::Color(250, 215, 100));
            stTxt.setPosition({62.f, 670.f});
            window.draw(stTxt);

            sf::Text ctTxt(font, "Controls: " + activeMeta.controls + "  |  [Esc] Campaign Map", 13);
            ctTxt.setFillColor(sf::Color(165, 185, 215));
            ctTxt.setPosition({62.f, 718.f});
            window.draw(ctTxt);
        }

        window.display();
    }

    return 0;
}
