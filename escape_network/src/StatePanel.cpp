#include "StatePanel.h"

namespace EscapeNetwork {

void StatePanel::reset() {
    data = StatePanelData();
}

void StatePanel::setAlgorithm(const std::string& algorithmName, int totalSteps) {
    reset();
    data.algorithmName = algorithmName;
    data.totalSteps = totalSteps;
}

void StatePanel::applyTraversalStep(const TraversalStep& step, int stepIndex) {
    data.stepIndex = stepIndex;
    data.currentVertexId = step.currentVertexId;
    data.frontierVertexIds = step.frontierVertexIds;
    data.visitedVertexIds = step.visitedVertexIds;
    data.label = step.label;
}

void StatePanel::setDistances(const std::vector<double>& distances) {
    data.distances = distances;
}

const StatePanelData& StatePanel::getData() const {
    return data;
}

} // namespace EscapeNetwork
