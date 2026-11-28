#pragma once

#include "Graph.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

// -----------------------------------------------------------------------------
// Traversal step state (feeds the step-by-step animation / StatePanel later).
// Deliberately simple: a snapshot of current / frontier / visited + a label.
// -----------------------------------------------------------------------------
struct TraversalStep {
    int currentVertexId = -1;              // vertex being processed (-1 = initial step)
    std::vector<int> frontierVertexIds;    // BFS: queue (front first) | DFS: stack (top first)
    std::vector<int> visitedVertexIds;     // vertices already processed, in visitation order
    std::string label;                     // human-readable description of the step
};

// Visitation order only.
std::vector<int> bfsOrder(const Graph& graph, int startVertexId);
std::vector<int> dfsOrder(const Graph& graph, int startVertexId);

// Visitation order + per-step snapshots (outSteps is cleared first).
std::vector<int> bfsOrder(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps);
std::vector<int> dfsOrder(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps);

// -----------------------------------------------------------------------------
// Dijkstra (binary heap, O((V + E) log V)).
// -----------------------------------------------------------------------------
struct DijkstraResult {
    std::vector<double> dist;        // +infinity for unreachable vertices
    std::vector<int> prev;           // -1 for source / unreachable vertices
    bool rejectedNegativeWeight = false;
};

// Scans every edge weight first; if any is negative, returns immediately with
// rejectedNegativeWeight = true and empty dist/prev (no partial run).
DijkstraResult dijkstra(const Graph& graph, int sourceVertexId);

// Rebuilds source -> target path from a DijkstraResult. Empty if unreachable,
// rejected, or target out of range.
std::vector<int> extractPath(const DijkstraResult& result, int targetVertexId);

// -----------------------------------------------------------------------------
// Bellman-Ford (O(V * E)). Negative edge weights are fully supported -- there is
// deliberately NO negative-weight rejection here (unlike Dijkstra).
// -----------------------------------------------------------------------------
struct BellmanFordResult {
    std::vector<double> dist;        // +infinity for unreachable vertices
    std::vector<int> prev;           // -1 for source / unreachable vertices
    bool hasNegativeCycleReachableFromSource = false;
};

// Runs V-1 relaxation passes, then one extra pass. If any vertex reachable
// from the source still improves in that extra pass, a negative cycle
// reachable from the source exists and the flag is set. Negative cycles that
// are NOT reachable from the source are never flagged (their vertices keep
// dist = +infinity and are never relaxed from). Edge weights are read-only.
BellmanFordResult bellmanFord(const Graph& graph, int sourceVertexId);

// Rebuilds source -> target path from a BellmanFordResult. Empty if the
// target is unreachable / out of range, or if a reachable negative cycle was
// detected (shortest paths are undefined in that case).
std::vector<int> extractPath(const BellmanFordResult& result, int targetVertexId);

// -----------------------------------------------------------------------------
// Prim's MST (binary heap, O(E log V)). Every edge is treated as undirected.
//
// DISCONNECTED-GRAPH BEHAVIOR (decided once; Kruskal MUST use the same rule):
//   graphDisconnected = true AND mstEdges holds the minimum spanning FOREST,
//   i.e. the union of one MST per connected component (V - C edges for C
//   components). For a connected graph graphDisconnected = false and mstEdges
//   holds exactly V - 1 edges. An empty graph yields no edges, flag false.
// -----------------------------------------------------------------------------
struct PrimResult {
    std::vector<Edge> mstEdges;      // in the order Prim added them
    bool graphDisconnected = false;
};

// Grows the tree from startVertexId first (if valid, otherwise vertex 0),
// then from each remaining unvisited vertex in id order to cover the forest.
PrimResult prim(const Graph& graph, int startVertexId = 0);

} // namespace EscapeNetwork
