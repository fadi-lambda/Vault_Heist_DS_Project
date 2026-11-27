#pragma once

#include <string>
#include <vector>

namespace EscapeNetwork {

class Graph;

struct ValidationResult {
    bool valid = false;
    std::string message;
};

// Structurally separate from Graph/algorithm implementations and any GUI code.
// Per-algorithm validation logic will be added as each algorithm is implemented.
class Validator {
public:
    Validator() = default;

    ValidationResult validateStep(const Graph& graph, int vertexId) const;
    ValidationResult validateSubmission(const Graph& graph,
                                        const std::vector<int>& sequence) const;
    void reset();
};

} // namespace EscapeNetwork
