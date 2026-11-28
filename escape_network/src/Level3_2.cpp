#include "Level3_2.h"

#include <cmath>

namespace EscapeNetwork {

namespace {

int findRoot(std::vector<int>& parent, int v) {
    while (parent[v] != v) {
        parent[v] = parent[parent[v]];
        v = parent[v];
    }
    return v;
}

} // namespace

bool Level3_2::loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err) {
    LevelManager manager;
    return manager.loadLevelById(LEVEL_ID, outGraph, outMeta, err);
}

PowerGridReport Level3_2::computePowerGrid(const Graph& graph, int startVertexId) {
    PowerGridReport report;
    const PrimResult result = prim(graph, startVertexId);
    report.mstEdges = result.mstEdges;
    report.graphDisconnected = result.graphDisconnected;
    for (const Edge& e : result.mstEdges) {
        report.totalWeight += e.weight;
    }
    return report;
}

bool Level3_2::verifyPlayerSelection(const Graph& graph, const std::vector<int>& edgeIds) {
    const int n = graph.getVertexCount();
    if (n == 0 || static_cast<int>(edgeIds.size()) != n - 1) {
        return false;
    }
    const PowerGridReport optimal = computePowerGrid(graph, DEFAULT_START_VERTEX);
    if (optimal.graphDisconnected) {
        return false;
    }

    std::vector<int> parent(n);
    for (int i = 0; i < n; ++i) {
        parent[i] = i;
    }
    std::vector<bool> used(graph.getEdgeCount(), false);
    double total = 0.0;
    for (int id : edgeIds) {
        const Edge* e = graph.getEdge(id);
        if (!e || used[id]) {
            return false;
        }
        used[id] = true;
        const int a = findRoot(parent, e->from);
        const int b = findRoot(parent, e->to);
        if (a == b) {
            return false; // would create a cycle
        }
        parent[a] = b;
        total += e->weight;
    }
    // n-1 acyclic edges over n vertices => spanning tree.
    return std::fabs(total - optimal.totalWeight) < 1e-9;
}

} // namespace EscapeNetwork
