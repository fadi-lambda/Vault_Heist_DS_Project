#pragma once

#include "Graph.h"
#include "GraphAlgorithms.h"
#include "LevelManager.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// Act 2 Mission 2 -- "Toll Trap".
// Uses Bellman-Ford on a graph with negative (toll-rebate) edges. Pure logic.
// The main mission graph (level_2_2.json) has negative edges but NO negative
// cycle; negative-cycle cases live in the unit tests only.
struct TollRouteReport {
    int sourceVertexId = -1;
    int targetVertexId = -1;
    bool hasNegativeCycleReachableFromSource = false;
    bool reachable = false;
    double shortestDistance = 0.0;   // valid only if reachable and no neg. cycle
    std::vector<int> path;           // source ... target, empty otherwise
};

class Level2_2 {
public:
    static const int LEVEL_ID = 22;
    static const int DEFAULT_SOURCE_VERTEX = 0;
    static const int DEFAULT_TARGET_VERTEX = 4;

    static const int HEAT_PER_WRONG_RELAXATION = 10;
    static const int HEAT_PER_HINT_USED = 5;

    // Loads the predefined level_2_2.json graph.
    static bool loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err);

    // Runs Bellman-Ford on the REAL edge weights (never modified).
    static TollRouteReport computeTollRoute(const Graph& graph, int sourceVertexId, int targetVertexId);

    // True if `path` is a valid source->target route whose cost equals the
    // Bellman-Ford shortest distance. False if a reachable negative cycle exists.
    static bool verifyPlayerPath(const Graph& graph,
                                 int sourceVertexId,
                                 int targetVertexId,
                                 const std::vector<int>& path);

    // Gameplay heat penalty for Bellman-Ford missions. Completely ISOLATED from
    // the algorithm: takes its own non-negative counters (negative inputs are
    // clamped to 0), never reads or writes edge weights, dist or prev.
    static int computeBellmanFordHeatPenalty(int wrongRelaxations, int hintsUsed);
};

} // namespace EscapeNetwork
