#include "Level2_1.h"

#include <cmath>
#include <limits>

namespace EscapeNetwork {

bool Level2_1::loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err) {
    LevelManager manager;
    return manager.loadLevelById(LEVEL_ID, outGraph, outMeta, err);
}

EscapeRouteReport Level2_1::computeFastestEscape(const Graph& graph, int sourceVertexId, int targetVertexId) {
    EscapeRouteReport report;
    report.sourceVertexId = sourceVertexId;
    report.targetVertexId = targetVertexId;

    const DijkstraResult result = dijkstra(graph, sourceVertexId);
    if (result.rejectedNegativeWeight) {
        report.rejectedNegativeWeight = true;
        return report;
    }

    report.path = extractPath(result, targetVertexId);
    report.reachable = !report.path.empty();
    if (report.reachable) {
        report.shortestDistance = result.dist[targetVertexId];
    }
    return report;
}

bool Level2_1::pathCost(const Graph& graph, const std::vector<int>& path, double& outCost) {
    outCost = 0.0;
    if (path.empty()) {
        return false;
    }
    for (int v : path) {
        if (!graph.hasVertex(v)) {
            return false;
        }
    }
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        const int u = path[i];
        const int v = path[i + 1];
        double best = std::numeric_limits<double>::infinity();
        for (const AdjEntry& entry : graph.getNeighbors(u)) {
            if (entry.to != v) {
                continue;
            }
            const Edge* edge = graph.getEdge(entry.edgeId);
            if (edge && edge->weight < best) {
                best = edge->weight;
            }
        }
        if (best == std::numeric_limits<double>::infinity()) {
            return false;
        }
        outCost += best;
    }
    return true;
}

bool Level2_1::verifyPlayerPath(const Graph& graph,
                                int sourceVertexId,
                                int targetVertexId,
                                const std::vector<int>& path) {
    if (path.empty() || path.front() != sourceVertexId || path.back() != targetVertexId) {
        return false;
    }
    const EscapeRouteReport report = computeFastestEscape(graph, sourceVertexId, targetVertexId);
    if (report.rejectedNegativeWeight || !report.reachable) {
        return false;
    }
    double cost = 0.0;
    if (!pathCost(graph, path, cost)) {
        return false;
    }
    return std::fabs(cost - report.shortestDistance) < 1e-9;
}

} // namespace EscapeNetwork
