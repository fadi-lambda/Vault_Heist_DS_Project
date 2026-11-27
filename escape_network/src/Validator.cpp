#include "Validator.h"

namespace EscapeNetwork {

ValidationResult Validator::validateStep(const Graph& /*graph*/, int /*vertexId*/) const {
    return ValidationResult{false, "Validation not yet implemented"};
}

ValidationResult Validator::validateSubmission(const Graph& /*graph*/,
                                               const std::vector<int>& /*sequence*/) const {
    return ValidationResult{false, "Validation not yet implemented"};
}

void Validator::reset() {
}

} // namespace EscapeNetwork
