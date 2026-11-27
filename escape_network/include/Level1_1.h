#pragma once

#include "Graph.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

struct DegreeInfo {
    int vertexId = -1;
    int degree = 0;
    std::vector<int> neighborIds;
};

struct AdjListRow {
    int vertexId = -1;
    std::string label;
    std::vector<int> neighborIds;
};

struct GraphSummary {
    int vertexCount = 0;
    int edgeCount = 0;
    bool connected = false;
    bool hasCycle = false;
};

class Level1_1 {
public:
    static int getVertexCount(const Graph& graph);
    static int getEdgeCount(const Graph& graph);
    static DegreeInfo getDegreeInfo(const Graph& graph, int vertexId);
    static std::vector<int> getNeighborList(const Graph& graph, int vertexId);
    static std::vector<AdjListRow> getAdjacencyListDump(const Graph& graph);
    static GraphSummary getSummary(const Graph& graph);
};

} // namespace EscapeNetwork
