#pragma once

#include "GraphAlgorithms.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// Pure data model for the state panel (no rendering). Field names match
// TraversalStep in GraphAlgorithms.h so a step can be copied in directly.
struct StatePanelData {
    std::string algorithmName;               // e.g. "BFS", "DFS", "Dijkstra"
    int stepIndex = 0;                       // 0-based index of the shown step
    int totalSteps = 0;

    int currentVertexId = -1;
    std::vector<int> frontierVertexIds;
    std::vector<int> visitedVertexIds;
    std::string label;                       // current step description

    std::vector<double> distances;           // per-vertex cost so far (empty if N/A; +inf = unreached)
};

class StatePanel {
public:
    StatePanel() = default;

    void reset();
    void setAlgorithm(const std::string& algorithmName, int totalSteps);
    void applyTraversalStep(const TraversalStep& step, int stepIndex);
    void setDistances(const std::vector<double>& distances);

    const StatePanelData& getData() const;

private:
    StatePanelData data;
};

} // namespace EscapeNetwork
