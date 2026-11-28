#pragma once

#include "Graph.h"
#include "GraphAlgorithms.h"
#include "LevelManager.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// Act 2 Mission 1 -- "Fastest Escape".
// Uses Dijkstra to compute / verify the shortest escape route. Pure logic.
struct EscapeRouteReport {
    int sourceVertexId = -1;
    int targetVertexId = -1;
    bool rejectedNegativeWeight = false;
    bool reachable = false;
    double shortestDistance = 0.0;   // valid only if reachable
    std::vector<int> path;           // source ... target, empty if unreachable/rejected
};

class Level2_1 {
public:
    static const int LEVEL_ID = 21;
    static const int DEFAULT_SOURCE_VERTEX = 0;
    static const int DEFAULT_TARGET_VERTEX = 5;

    // Loads the predefined level_2_1.json graph.
    static bool loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err);

    static EscapeRouteReport computeFastestEscape(const Graph& graph, int sourceVertexId, int targetVertexId);

    // Cost of following `path` edge by edge (cheapest parallel edge, direction
    // respected). Returns false if the path is empty or uses a missing edge.
    static bool pathCost(const Graph& graph, const std::vector<int>& path, double& outCost);

    // True if `path` is a valid source->target route whose cost equals the
    // Dijkstra shortest distance. Always false on negative-weight graphs.
    static bool verifyPlayerPath(const Graph& graph,
                                 int sourceVertexId,
                                 int targetVertexId,
                                 const std::vector<int>& path);
};

} // namespace EscapeNetwork
