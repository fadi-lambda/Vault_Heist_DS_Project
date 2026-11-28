#pragma once

#include "Graph.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// -----------------------------------------------------------------------------
// Traversal step state (feeds the step-by-step animation / StatePanel later).
// Deliberately simple: a snapshot of current / frontier / visited + a label.
// -----------------------------------------------------------------------------
struct TraversalStep {
    int currentVertexId = -1;              // vertex being processed (-1 = initial step)
    std::vector<int> frontierVertexIds;    // BFS: queue (front first) | DFS: stack (top first)
    std::vector<int> visitedVertexIds;     // vertices already processed, in visitation order
    std::string label;                     // human-readable description of the step
};

// Visitation order only.
std::vector<int> bfsOrder(const Graph& graph, int startVertexId);
std::vector<int> dfsOrder(const Graph& graph, int startVertexId);

// Visitation order + per-step snapshots (outSteps is cleared first).
std::vector<int> bfsOrder(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps);
std::vector<int> dfsOrder(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps);

// -----------------------------------------------------------------------------
// Dijkstra (binary heap, O((V + E) log V)).
// -----------------------------------------------------------------------------
struct DijkstraResult {
    std::vector<double> dist;        // +infinity for unreachable vertices
    std::vector<int> prev;           // -1 for source / unreachable vertices
    bool rejectedNegativeWeight = false;
};

// Scans every edge weight first; if any is negative, returns immediately with
// rejectedNegativeWeight = true and empty dist/prev (no partial run).
DijkstraResult dijkstra(const Graph& graph, int sourceVertexId);

// Rebuilds source -> target path from a DijkstraResult. Empty if unreachable,
// rejected, or target out of range.
std::vector<int> extractPath(const DijkstraResult& result, int targetVertexId);

} // namespace EscapeNetwork
