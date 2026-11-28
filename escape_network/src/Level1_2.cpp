#include "Level1_2.h"

namespace EscapeNetwork {

bool Level1_2::loadPredefinedGraph(Graph& outGraph, LevelMetadata& outMeta, std::string& err) {
    LevelManager manager;
    return manager.loadLevelById(LEVEL_ID, outGraph, outMeta, err);
}

std::vector<int> Level1_2::runBfs(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps) {
    return bfsOrder(graph, startVertexId, outSteps);
}

std::vector<int> Level1_2::runDfs(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps) {
    return dfsOrder(graph, startVertexId, outSteps);
}

std::vector<int> Level1_2::getUnreachableFrom(const Graph& graph, int startVertexId) {
    std::vector<int> unreachable;
    const int n = graph.getVertexCount();
    std::vector<bool> reached(n, false);
    for (int v : bfsOrder(graph, startVertexId)) {
        reached[v] = true;
    }
    for (int v = 0; v < n; ++v) {
        if (!reached[v]) {
            unreachable.push_back(v);
        }
    }
    return unreachable;
}

ScoutReport Level1_2::scout(const Graph& graph, int startVertexId) {
    ScoutReport report;
    report.startVertexId = startVertexId;
    report.bfsOrder = bfsOrder(graph, startVertexId);
    report.dfsOrder = dfsOrder(graph, startVertexId);
    report.unreachableVertexIds = getUnreachableFrom(graph, startVertexId);
    report.components = graph.getConnectedComponents();
    report.componentCount = static_cast<int>(report.components.size());
    report.connected = graph.isConnected();
    report.hasCycle = graph.hasCycle();
    return report;
}

bool Level1_2::verifyBfsOrder(const Graph& graph, int startVertexId, const std::vector<int>& answer) {
    return answer == bfsOrder(graph, startVertexId);
}

bool Level1_2::verifyDfsOrder(const Graph& graph, int startVertexId, const std::vector<int>& answer) {
    return answer == dfsOrder(graph, startVertexId);
}

bool Level1_2::verifyComponentCount(const Graph& graph, int answer) {
    return answer == static_cast<int>(graph.getConnectedComponents().size());
}

bool Level1_2::verifyCycleAnswer(const Graph& graph, bool answer) {
    return answer == graph.hasCycle();
}

} // namespace EscapeNetwork
