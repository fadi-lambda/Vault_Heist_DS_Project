#include "Level2_2.h"

#include "Level2_1.h"

#include <cmath>

namespace EscapeNetwork {

bool Level2_2::loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err) {
    LevelManager manager;
    return manager.loadLevelById(LEVEL_ID, outGraph, outMeta, err);
}

TollRouteReport Level2_2::computeTollRoute(const Graph& graph, int sourceVertexId, int targetVertexId) {
    TollRouteReport report;
    report.sourceVertexId = sourceVertexId;
    report.targetVertexId = targetVertexId;

    const BellmanFordResult result = bellmanFord(graph, sourceVertexId);
    if (result.hasNegativeCycleReachableFromSource) {
        report.hasNegativeCycleReachableFromSource = true;
        return report;
    }

    report.path = extractPath(result, targetVertexId);
    report.reachable = !report.path.empty();
    if (report.reachable) {
        report.shortestDistance = result.dist[targetVertexId];
    }
    return report;
}

bool Level2_2::verifyPlayerPath(const Graph& graph,
                                int sourceVertexId,
                                int targetVertexId,
                                const std::vector<int>& path) {
    if (path.empty() || path.front() != sourceVertexId || path.back() != targetVertexId) {
        return false;
    }
    const TollRouteReport report = computeTollRoute(graph, sourceVertexId, targetVertexId);
    if (report.hasNegativeCycleReachableFromSource || !report.reachable) {
        return false;
    }
    double cost = 0.0;
    if (!Level2_1::pathCost(graph, path, cost)) { // read-only edge-by-edge sum
        return false;
    }
    return std::fabs(cost - report.shortestDistance) < 1e-9;
}

int Level2_2::computeBellmanFordHeatPenalty(int wrongRelaxations, int hintsUsed) {
    const int wrong = wrongRelaxations > 0 ? wrongRelaxations : 0;
    const int hints = hintsUsed > 0 ? hintsUsed : 0;
    return wrong * HEAT_PER_WRONG_RELAXATION + hints * HEAT_PER_HINT_USED;
}

} // namespace EscapeNetwork
