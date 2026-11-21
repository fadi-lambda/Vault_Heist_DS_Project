#pragma once

#include <string>

namespace VaultHeist {

/**
 * @brief Headless Benchmark Harness (Project.pdf Section 10).
 *
 * Runs all Section 10 empirical complexity experiments using the real
 * BST, AVLTree, and MaxHeap structures (plain non-event overloads) plus
 * a hand-rolled Merge Sort for sorting comparison, and writes the full
 * formatted report to `reportPath`.
 *
 * Course constraints respected: no std::vector, no <algorithm>, no enum,
 * no mutable global variables.
 */
class Benchmark {
public:
    static bool runAll(const std::string& reportPath = "benchmark_report.txt",
                       unsigned int seed = 20261119u);
};

} // namespace VaultHeist
