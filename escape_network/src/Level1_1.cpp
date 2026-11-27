#include "Level1_1.h"

namespace EscapeNetwork {

int Level1_1::getVertexCount(const Graph& graph) {
    return graph.getVertexCount();
}

int Level1_1::getEdgeCount(const Graph& graph) {
    return graph.getEdgeCount();
}

DegreeInfo Level1_1::getDegreeInfo(const Graph& graph, int vertexId) {
    DegreeInfo info;
    info.vertexId = vertexId;
    if (!graph.hasVertex(vertexId)) {
        return info;
    }

    const std::vector<AdjEntry>& neighbors = graph.getNeighbors(vertexId);
    info.degree = static_cast<int>(neighbors.size());
    info.neighborIds.reserve(neighbors.size());
    for (const AdjEntry& entry : neighbors) {
        info.neighborIds.push_back(entry.to);
    }
    return info;
}

std::vector<int> Level1_1::getNeighborList(const Graph& graph, int vertexId) {
    return getDegreeInfo(graph, vertexId).neighborIds;
}

std::vector<AdjListRow> Level1_1::getAdjacencyListDump(const Graph& graph) {
    const std::vector<Vertex>& vertices = graph.getVertices();
    std::vector<AdjListRow> dump;
    dump.reserve(vertices.size());

    for (const Vertex& v : vertices) {
        AdjListRow row;
        row.vertexId = v.id;
        row.label = v.label;
        row.neighborIds = getNeighborList(graph, v.id);
        dump.push_back(row);
    }
    return dump;
}

GraphSummary Level1_1::getSummary(const Graph& graph) {
    GraphSummary summary;
    summary.vertexCount = graph.getVertexCount();
    summary.edgeCount = graph.getEdgeCount();
    summary.connected = graph.isConnected();
    summary.hasCycle = graph.hasCycle();
    return summary;
}

} // namespace EscapeNetwork
