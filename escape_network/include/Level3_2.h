#pragma once

#include "Graph.h"
#include "GraphAlgorithms.h"
#include "LevelManager.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// Act 3 Mission 2 -- "Power the District".
// Uses Prim's MST on the predefined connected graph (level_3_2.json). Pure
// logic. Disconnected-graph behavior follows the rule documented on PrimResult
// in GraphAlgorithms.h (flag + spanning forest); exercised in unit tests only.
struct PowerGridReport {
    std::vector<Edge> mstEdges;
    double totalWeight = 0.0;
    bool graphDisconnected = false;
};

class Level3_2 {
public:
    static const int LEVEL_ID = 32;
    static const int DEFAULT_START_VERTEX = 0;

    // Loads the predefined level_3_2.json graph.
    static bool loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err);

    static PowerGridReport computePowerGrid(const Graph& graph, int startVertexId);

    // True if the player's chosen edge ids form a spanning tree (V-1 distinct,
    // valid edges, acyclic, connecting every vertex) whose total weight equals
    // the Prim MST weight. Always false on a disconnected graph.
    static bool verifyPlayerSelection(const Graph& graph, const std::vector<int>& edgeIds);
};

} // namespace EscapeNetwork
