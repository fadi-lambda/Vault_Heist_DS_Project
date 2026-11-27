#pragma once

#include "Graph.h"
#include <SFML/Graphics.hpp>
#include <string>

namespace EscapeNetwork {

// Visual state constants (no enum / enum class allowed)
static const int VS_UNVISITED = 0; // Neutral grey
static const int VS_CURRENT   = 1; // Gold highlight + thick ring
static const int VS_FRONTIER  = 2; // Blue highlight
static const int VS_VISITED   = 3; // Muted teal/blue
static const int VS_ACCEPTED  = 4; // Green + [OK] / thick outline
static const int VS_REJECTED  = 5; // Red + [X] / warning outline

struct GraphVisuals {
    int* vertexStates;
    int* edgeStates;
    double* edgeFlow;
    int vertexCount;
    int edgeCount;

    GraphVisuals()
        : vertexStates(nullptr), edgeStates(nullptr), edgeFlow(nullptr),
          vertexCount(0), edgeCount(0) {}

    ~GraphVisuals() {
        clear();
    }

    GraphVisuals(const GraphVisuals&) = delete;
    GraphVisuals& operator=(const GraphVisuals&) = delete;

    void clear() {
        delete[] vertexStates;
        delete[] edgeStates;
        delete[] edgeFlow;
        vertexStates = nullptr;
        edgeStates = nullptr;
        edgeFlow = nullptr;
        vertexCount = 0;
        edgeCount = 0;
    }

    void resizeFor(const Graph& g) {
        clear();
        vertexCount = g.getVertexCount();
        edgeCount = g.getEdgeCount();
        if (vertexCount > 0) {
            vertexStates = new int[vertexCount];
            for (int i = 0; i < vertexCount; ++i) vertexStates[i] = VS_UNVISITED;
        }
        if (edgeCount > 0) {
            edgeStates = new int[edgeCount];
            edgeFlow = new double[edgeCount];
            for (int i = 0; i < edgeCount; ++i) {
                edgeStates[i] = VS_UNVISITED;
                edgeFlow[i] = 0.0;
            }
        }
    }
};

class Renderer {
public:
    Renderer() = default;

    // Draws the graph inside a canvas offset (canvasX, canvasY)
    void drawGraph(sf::RenderWindow& window,
                   const sf::Font& font,
                   const Graph& graph,
                   const GraphVisuals& visuals,
                   float canvasX,
                   float canvasY,
                   int hoveredVertexId = -1) const;

    // Returns vertex id under mouse (mx, my), or -1 if none
    int hitTestVertex(const Graph& graph,
                      float mx,
                      float my,
                      float canvasX,
                      float canvasY,
                      float radius = 24.f) const;

    // Reusable UI helpers
    static void drawPanel(sf::RenderWindow& window,
                          float x, float y, float w, float h,
                          sf::Color fill, sf::Color outline, float thickness = 2.f);

    static void drawButton(sf::RenderWindow& window,
                           const sf::Font& font,
                           const std::string& label,
                           float x, float y, float w, float h,
                           bool hovered,
                           sf::Color fill = sf::Color(30, 48, 76),
                           sf::Color outline = sf::Color(80, 145, 220),
                           unsigned int charSize = 15);

    static bool isInside(float mx, float my, float x, float y, float w, float h);

private:
    static sf::Color vertexFillColor(int state);
    static sf::Color vertexOutlineColor(int state);
    static sf::Color edgeColor(int state);
    static std::string stateTag(int state);
};

} // namespace EscapeNetwork
