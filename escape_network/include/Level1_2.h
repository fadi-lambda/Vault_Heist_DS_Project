#pragma once

#include "Graph.h"
#include "GraphAlgorithms.h"
#include "LevelManager.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// Act 1 Mission 2 -- "Scout the Districts".
// Wraps BFS/DFS + Graph::isConnected/getConnectedComponents/hasCycle into the
// mission's checks. Pure logic, no rendering.
struct ScoutReport {
    int startVertexId = -1;
    std::vector<int> bfsOrder;
    std::vector<int> dfsOrder;
    std::vector<int> unreachableVertexIds;          // not reachable from start
    std::vector<std::vector<int>> components;
    int componentCount = 0;
    bool connected = false;
    bool hasCycle = false;
};

class Level1_2 {
public:
    static const int LEVEL_ID = 12;
    static const int DEFAULT_START_VERTEX = 0;

    // Loads the predefined level_1_2.json graph.
    static bool loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err);

    static std::vector<int> runBfs(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps);
    static std::vector<int> runDfs(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps);

    static std::vector<int> getUnreachableFrom(const Graph& graph, int startVertexId);
    static ScoutReport scout(const Graph& graph, int startVertexId);

    // Mission checks (compare a player's answer to the ground truth).
    static bool verifyBfsOrder(const Graph& graph, int startVertexId, const std::vector<int>& answer);
    static bool verifyDfsOrder(const Graph& graph, int startVertexId, const std::vector<int>& answer);
    static bool verifyComponentCount(const Graph& graph, int answer);
    static bool verifyCycleAnswer(const Graph& graph, bool answer);
};

} // namespace EscapeNetwork
