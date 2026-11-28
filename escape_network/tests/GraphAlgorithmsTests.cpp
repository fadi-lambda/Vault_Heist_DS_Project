#include <gtest/gtest.h>

#include "Graph.h"
#include "GraphAlgorithms.h"
#include "Level1_2.h"
#include "Level2_1.h"
#include "Level2_2.h"
#include "Level3_2.h"
#include "LevelManager.h"
#include "StatePanel.h"

#include <limits>
#include <string>
#include <vector>

using namespace EscapeNetwork;

namespace {

Graph makeGraph(int vertexCount) {
    Graph g;
    for (int i = 0; i < vertexCount; ++i) {
        g.addVertex("V" + std::to_string(i), static_cast<float>(i * 100), 0.0f);
    }
    return g;
}

// 0-1, 0-2, 1-3, 1-4, 2-5 (undirected tree, connected)
Graph makeConnectedTree() {
    Graph g = makeGraph(6);
    g.addEdge(0, 1, 1.0);
    g.addEdge(0, 2, 1.0);
    g.addEdge(1, 3, 1.0);
    g.addEdge(1, 4, 1.0);
    g.addEdge(2, 5, 1.0);
    return g;
}

// {0,1}, {2,3}, {4}
Graph makeDisconnected() {
    Graph g = makeGraph(5);
    g.addEdge(0, 1, 1.0);
    g.addEdge(2, 3, 1.0);
    return g;
}

const double INF = std::numeric_limits<double>::infinity();

} // namespace

// =============================================================================
// BFS / DFS
// =============================================================================

TEST(EscapeNetworkTraversalTest, BfsDfsOnConnectedGraph) {
    Graph g = makeConnectedTree();
    EXPECT_EQ(bfsOrder(g, 0), std::vector<int>({0, 1, 2, 3, 4, 5}));
    EXPECT_EQ(dfsOrder(g, 0), std::vector<int>({0, 1, 3, 4, 2, 5}));
}

TEST(EscapeNetworkTraversalTest, BfsDfsOnDisconnectedGraph) {
    Graph g = makeDisconnected();
    EXPECT_EQ(bfsOrder(g, 0), std::vector<int>({0, 1}));
    EXPECT_EQ(dfsOrder(g, 2), std::vector<int>({2, 3}));
    EXPECT_EQ(bfsOrder(g, 4), std::vector<int>({4}));
    EXPECT_EQ(dfsOrder(g, 4), std::vector<int>({4}));
}

TEST(EscapeNetworkTraversalTest, BfsDfsOnSingleVertexGraph) {
    Graph g = makeGraph(1);
    std::vector<TraversalStep> steps;
    EXPECT_EQ(bfsOrder(g, 0, steps), std::vector<int>({0}));
    ASSERT_EQ(steps.size(), 2u);
    EXPECT_TRUE(steps.back().frontierVertexIds.empty());
    EXPECT_EQ(dfsOrder(g, 0, steps), std::vector<int>({0}));
    ASSERT_EQ(steps.size(), 2u);
    EXPECT_EQ(steps.back().visitedVertexIds, std::vector<int>({0}));
}

TEST(EscapeNetworkTraversalTest, InvalidStartReturnsEmpty) {
    Graph g = makeGraph(2);
    std::vector<TraversalStep> steps;
    EXPECT_TRUE(bfsOrder(g, 7, steps).empty());
    EXPECT_TRUE(steps.empty());
    EXPECT_TRUE(dfsOrder(g, -1).empty());
}

TEST(EscapeNetworkTraversalTest, BfsStepStateTracksQueue) {
    Graph g = makeConnectedTree();
    std::vector<TraversalStep> steps;
    const std::vector<int> order = bfsOrder(g, 0, steps);
    ASSERT_EQ(steps.size(), order.size() + 1);

    EXPECT_EQ(steps[0].currentVertexId, -1);
    EXPECT_EQ(steps[0].frontierVertexIds, std::vector<int>({0}));
    EXPECT_TRUE(steps[0].visitedVertexIds.empty());
    EXPECT_FALSE(steps[0].label.empty());

    EXPECT_EQ(steps[1].currentVertexId, 0);
    EXPECT_EQ(steps[1].frontierVertexIds, std::vector<int>({1, 2}));
    EXPECT_EQ(steps[1].visitedVertexIds, std::vector<int>({0}));

    EXPECT_TRUE(steps.back().frontierVertexIds.empty());
    EXPECT_EQ(steps.back().visitedVertexIds, order);
}

TEST(EscapeNetworkTraversalTest, DfsStepStateTracksStack) {
    Graph g = makeConnectedTree();
    std::vector<TraversalStep> steps;
    const std::vector<int> order = dfsOrder(g, 0, steps);
    ASSERT_EQ(steps.size(), order.size() + 1);

    EXPECT_EQ(steps[1].currentVertexId, 0);
    EXPECT_EQ(steps[1].frontierVertexIds, std::vector<int>({1, 2}));   // top first
    EXPECT_EQ(steps[2].currentVertexId, 1);
    EXPECT_EQ(steps[2].frontierVertexIds, std::vector<int>({3, 4, 2}));
    EXPECT_TRUE(steps.back().frontierVertexIds.empty());
    EXPECT_EQ(steps.back().visitedVertexIds, order);
}

// =============================================================================
// Dijkstra
// =============================================================================

TEST(EscapeNetworkDijkstraTest, MultiPathGraphPicksShortest) {
    Graph g = makeGraph(4);
    g.addEdge(0, 1, 4.0);
    g.addEdge(0, 2, 1.0);
    g.addEdge(2, 1, 2.0);
    g.addEdge(1, 3, 1.0);
    g.addEdge(2, 3, 5.0);

    DijkstraResult r = dijkstra(g, 0);
    EXPECT_FALSE(r.rejectedNegativeWeight);
    ASSERT_EQ(r.dist.size(), 4u);
    EXPECT_DOUBLE_EQ(r.dist[0], 0.0);
    EXPECT_DOUBLE_EQ(r.dist[1], 3.0);
    EXPECT_DOUBLE_EQ(r.dist[2], 1.0);
    EXPECT_DOUBLE_EQ(r.dist[3], 4.0);
    EXPECT_EQ(r.prev[0], -1);
    EXPECT_EQ(extractPath(r, 3), std::vector<int>({0, 2, 1, 3}));
}

TEST(EscapeNetworkDijkstraTest, UnreachableVertex) {
    Graph g = makeGraph(3);
    g.addEdge(0, 1, 2.0, 0.0, true);
    g.addEdge(2, 0, 1.0, 0.0, true);   // 2 -> 0 only, so 2 unreachable from 0

    DijkstraResult r = dijkstra(g, 0);
    EXPECT_FALSE(r.rejectedNegativeWeight);
    EXPECT_DOUBLE_EQ(r.dist[1], 2.0);
    EXPECT_EQ(r.dist[2], INF);
    EXPECT_EQ(r.prev[2], -1);
    EXPECT_TRUE(extractPath(r, 2).empty());
}

TEST(EscapeNetworkDijkstraTest, ZeroWeightEdges) {
    Graph g = makeGraph(3);
    g.addEdge(0, 1, 0.0);
    g.addEdge(1, 2, 0.0);
    g.addEdge(0, 2, 1.0);

    DijkstraResult r = dijkstra(g, 0);
    EXPECT_FALSE(r.rejectedNegativeWeight);
    EXPECT_DOUBLE_EQ(r.dist[1], 0.0);
    EXPECT_DOUBLE_EQ(r.dist[2], 0.0);
    EXPECT_EQ(extractPath(r, 2), std::vector<int>({0, 1, 2}));
}

TEST(EscapeNetworkDijkstraTest, NegativeWeightIsRejectedWithoutRunning) {
    Graph g = makeGraph(3);
    g.addEdge(0, 1, 2.0);
    g.addEdge(1, 2, -1.0);

    DijkstraResult r = dijkstra(g, 0);
    EXPECT_TRUE(r.rejectedNegativeWeight);
    EXPECT_TRUE(r.dist.empty());   // no partial run
    EXPECT_TRUE(r.prev.empty());
    EXPECT_TRUE(extractPath(r, 1).empty());
}

// =============================================================================
// Level 1.2 -- Scout the Districts
// =============================================================================

TEST(EscapeNetworkLevel1_2Test, ScoutsPredefinedGraph) {
    Graph g;
    LevelMetadata meta;
    std::string err;
    ASSERT_TRUE(Level1_2::loadPredefinedGraph(g, meta, err)) << err;
    EXPECT_EQ(meta.levelId, 12);

    ScoutReport report = Level1_2::scout(g, Level1_2::DEFAULT_START_VERTEX);
    EXPECT_EQ(report.bfsOrder, std::vector<int>({0, 1, 2, 3}));
    EXPECT_EQ(report.dfsOrder, std::vector<int>({0, 1, 3, 2}));
    EXPECT_EQ(report.unreachableVertexIds, std::vector<int>({4, 5}));
    EXPECT_EQ(report.componentCount, 2);
    EXPECT_FALSE(report.connected);
    EXPECT_TRUE(report.hasCycle);
}

TEST(EscapeNetworkLevel1_2Test, VerifiesPlayerAnswers) {
    Graph g;
    LevelMetadata meta;
    std::string err;
    ASSERT_TRUE(Level1_2::loadPredefinedGraph(g, meta, err)) << err;

    EXPECT_TRUE(Level1_2::verifyBfsOrder(g, 0, {0, 1, 2, 3}));
    EXPECT_FALSE(Level1_2::verifyBfsOrder(g, 0, {0, 1, 3, 2}));
    EXPECT_TRUE(Level1_2::verifyDfsOrder(g, 0, {0, 1, 3, 2}));
    EXPECT_TRUE(Level1_2::verifyComponentCount(g, 2));
    EXPECT_FALSE(Level1_2::verifyComponentCount(g, 1));
    EXPECT_TRUE(Level1_2::verifyCycleAnswer(g, true));
    EXPECT_FALSE(Level1_2::verifyCycleAnswer(g, false));

    std::vector<TraversalStep> steps;
    EXPECT_EQ(Level1_2::runBfs(g, 0, steps).size(), 4u);
    EXPECT_EQ(steps.size(), 5u);
}

// =============================================================================
// Level 2.1 -- Fastest Escape
// =============================================================================

TEST(EscapeNetworkLevel2_1Test, ComputesFastestEscapeOnPredefinedGraph) {
    Graph g;
    LevelMetadata meta;
    std::string err;
    ASSERT_TRUE(Level2_1::loadPredefinedGraph(g, meta, err)) << err;
    EXPECT_EQ(meta.levelId, 21);

    EscapeRouteReport report = Level2_1::computeFastestEscape(
        g, Level2_1::DEFAULT_SOURCE_VERTEX, Level2_1::DEFAULT_TARGET_VERTEX);
    EXPECT_FALSE(report.rejectedNegativeWeight);
    EXPECT_TRUE(report.reachable);
    EXPECT_DOUBLE_EQ(report.shortestDistance, 11.0);
    EXPECT_EQ(report.path, std::vector<int>({0, 2, 1, 3, 4, 5}));
}

TEST(EscapeNetworkLevel2_1Test, VerifiesPlayerPath) {
    Graph g;
    LevelMetadata meta;
    std::string err;
    ASSERT_TRUE(Level2_1::loadPredefinedGraph(g, meta, err)) << err;

    EXPECT_TRUE(Level2_1::verifyPlayerPath(g, 0, 5, {0, 2, 1, 3, 4, 5}));
    EXPECT_FALSE(Level2_1::verifyPlayerPath(g, 0, 5, {0, 1, 3, 5}));   // valid but cost 13
    EXPECT_FALSE(Level2_1::verifyPlayerPath(g, 0, 5, {0, 5}));         // no such edge
    EXPECT_FALSE(Level2_1::verifyPlayerPath(g, 0, 5, {5, 4, 3}));      // wrong endpoints

    double cost = 0.0;
    ASSERT_TRUE(Level2_1::pathCost(g, {0, 1, 3, 5}, cost));
    EXPECT_DOUBLE_EQ(cost, 13.0);
    EXPECT_FALSE(Level2_1::pathCost(g, {1, 0}, cost));                 // against direction
}

TEST(EscapeNetworkLevel2_1Test, RejectsNegativeWeightGraph) {
    Graph g = makeGraph(3);
    g.addEdge(0, 1, 1.0, 0.0, true);
    g.addEdge(1, 2, -3.0, 0.0, true);

    EscapeRouteReport report = Level2_1::computeFastestEscape(g, 0, 2);
    EXPECT_TRUE(report.rejectedNegativeWeight);
    EXPECT_FALSE(report.reachable);
    EXPECT_TRUE(report.path.empty());
    EXPECT_FALSE(Level2_1::verifyPlayerPath(g, 0, 2, {0, 1, 2}));
}

// =============================================================================
// LevelManager -- new levels by ID
// =============================================================================

TEST(EscapeNetworkLevelManagerTest, LoadsLevel1_2And2_1ById) {
    LevelManager manager;
    Graph g;
    LevelMetadata meta;
    std::string err;

    ASSERT_TRUE(manager.loadLevelById(12, g, meta, err)) << err;
    EXPECT_EQ(meta.act, 1);
    EXPECT_EQ(g.getVertexCount(), 6);
    EXPECT_EQ(g.getEdgeCount(), 5);

    ASSERT_TRUE(manager.loadLevelById(21, g, meta, err)) << err;
    EXPECT_EQ(meta.act, 2);
    EXPECT_EQ(g.getVertexCount(), 6);
    EXPECT_EQ(g.getEdgeCount(), 8);
    for (const Edge& e : g.getEdges()) {
        EXPECT_GE(e.weight, 0.0);
    }
}

// =============================================================================
// StatePanel data model
// =============================================================================

TEST(EscapeNetworkStatePanelTest, AppliesTraversalStepAndDistances) {
    Graph g = makeConnectedTree();
    std::vector<TraversalStep> steps;
    bfsOrder(g, 0, steps);

    StatePanel panel;
    panel.setAlgorithm("BFS", static_cast<int>(steps.size()));
    panel.applyTraversalStep(steps[1], 1);

    const StatePanelData& d = panel.getData();
    EXPECT_EQ(d.algorithmName, "BFS");
    EXPECT_EQ(d.totalSteps, 7);
    EXPECT_EQ(d.stepIndex, 1);
    EXPECT_EQ(d.currentVertexId, 0);
    EXPECT_EQ(d.frontierVertexIds, std::vector<int>({1, 2}));
    EXPECT_EQ(d.visitedVertexIds, std::vector<int>({0}));
    EXPECT_EQ(d.label, steps[1].label);
    EXPECT_TRUE(d.distances.empty());

    panel.setDistances({0.0, 1.0, INF});
    EXPECT_EQ(panel.getData().distances.size(), 3u);

    panel.reset();
    EXPECT_EQ(panel.getData().currentVertexId, -1);
    EXPECT_TRUE(panel.getData().algorithmName.empty());
}

// =============================================================================
// Bellman-Ford
// =============================================================================

namespace {

std::vector<double> edgeWeights(const Graph& g) {
    std::vector<double> w;
    for (const Edge& e : g.getEdges()) {
        w.push_back(e.weight);
    }
    return w;
}

} // namespace

TEST(EscapeNetworkBellmanFordTest, NegativeWeightsNoCycle) {
    Graph g = makeGraph(5);
    g.addEdge(0, 1, 6.0, 0.0, true);
    g.addEdge(0, 2, 7.0, 0.0, true);
    g.addEdge(1, 3, 5.0, 0.0, true);
    g.addEdge(2, 1, -3.0, 0.0, true);
    g.addEdge(2, 4, 9.0, 0.0, true);
    g.addEdge(3, 4, -2.0, 0.0, true);
    const std::vector<double> before = edgeWeights(g);

    BellmanFordResult r = bellmanFord(g, 0);
    EXPECT_FALSE(r.hasNegativeCycleReachableFromSource);
    EXPECT_EQ(r.dist, std::vector<double>({0.0, 4.0, 7.0, 9.0, 7.0}));
    EXPECT_EQ(extractPath(r, 4), std::vector<int>({0, 2, 1, 3, 4}));
    EXPECT_EQ(edgeWeights(g), before); // weights untouched

    // Dijkstra would refuse this graph; Bellman-Ford must not.
    EXPECT_TRUE(dijkstra(g, 0).rejectedNegativeWeight);
}

TEST(EscapeNetworkBellmanFordTest, DetectsReachableNegativeCycle) {
    Graph g = makeGraph(4);
    g.addEdge(0, 1, 1.0, 0.0, true);
    g.addEdge(1, 2, 2.0, 0.0, true);
    g.addEdge(2, 1, -4.0, 0.0, true); // cycle 1->2->1 weight -2
    g.addEdge(2, 3, 1.0, 0.0, true);

    BellmanFordResult r = bellmanFord(g, 0);
    EXPECT_TRUE(r.hasNegativeCycleReachableFromSource);
    EXPECT_TRUE(extractPath(r, 3).empty());
}

TEST(EscapeNetworkBellmanFordTest, IgnoresUnreachableNegativeCycle) {
    Graph g = makeGraph(5);
    g.addEdge(0, 1, 2.0, 0.0, true);
    g.addEdge(1, 2, -1.0, 0.0, true);
    // Negative cycle 3->4->3 (weight -3), not reachable from 0.
    g.addEdge(3, 4, 1.0, 0.0, true);
    g.addEdge(4, 3, -4.0, 0.0, true);
    g.addEdge(3, 0, 1.0, 0.0, true); // cycle can reach source, not vice versa

    BellmanFordResult r = bellmanFord(g, 0);
    EXPECT_FALSE(r.hasNegativeCycleReachableFromSource);
    EXPECT_DOUBLE_EQ(r.dist[2], 1.0);
    EXPECT_EQ(r.dist[3], INF);
    EXPECT_EQ(r.dist[4], INF);
    EXPECT_EQ(extractPath(r, 2), std::vector<int>({0, 1, 2}));
}

TEST(EscapeNetworkBellmanFordTest, UnreachableVertex) {
    Graph g = makeGraph(4);
    g.addEdge(0, 1, -2.0, 0.0, true);
    g.addEdge(1, 2, 3.0, 0.0, true);
    // vertex 3 isolated

    BellmanFordResult r = bellmanFord(g, 0);
    EXPECT_FALSE(r.hasNegativeCycleReachableFromSource);
    EXPECT_EQ(r.dist[3], INF);
    EXPECT_EQ(r.prev[3], -1);
    EXPECT_TRUE(extractPath(r, 3).empty());
    EXPECT_DOUBLE_EQ(r.dist[2], 1.0);
}

TEST(EscapeNetworkLevel2_2Test, TollTrapPredefinedGraph) {
    Graph g;
    LevelMetadata meta;
    std::string err;
    ASSERT_TRUE(Level2_2::loadPredefinedGraph(g, meta, err)) << err;
    EXPECT_EQ(meta.levelId, 22);

    bool hasNegative = false;
    for (const Edge& e : g.getEdges()) {
        hasNegative = hasNegative || e.weight < 0.0;
    }
    EXPECT_TRUE(hasNegative);
    const std::vector<double> before = edgeWeights(g);

    TollRouteReport report = Level2_2::computeTollRoute(
        g, Level2_2::DEFAULT_SOURCE_VERTEX, Level2_2::DEFAULT_TARGET_VERTEX);
    EXPECT_FALSE(report.hasNegativeCycleReachableFromSource);
    EXPECT_TRUE(report.reachable);
    EXPECT_DOUBLE_EQ(report.shortestDistance, 7.0);
    EXPECT_EQ(report.path, std::vector<int>({0, 2, 1, 3, 4}));

    EXPECT_TRUE(Level2_2::verifyPlayerPath(g, 0, 4, {0, 2, 1, 3, 4}));
    EXPECT_FALSE(Level2_2::verifyPlayerPath(g, 0, 4, {0, 1, 3, 4})); // cost 9
    EXPECT_FALSE(Level2_2::verifyPlayerPath(g, 0, 4, {0, 4}));       // no edge

    // Heat penalty is isolated: computing it leaves the graph untouched.
    EXPECT_EQ(Level2_2::computeBellmanFordHeatPenalty(2, 1), 25);
    EXPECT_EQ(Level2_2::computeBellmanFordHeatPenalty(-3, -1), 0);
    EXPECT_EQ(edgeWeights(g), before);
}

// =============================================================================
// Prim's MST
// =============================================================================

namespace {

// Verifies that `edges` form an acyclic set and returns the number of
// connected components they induce over `vertexCount` vertices.
bool isAcyclic(const std::vector<Edge>& edges, int vertexCount, int& outComponents) {
    std::vector<int> parent(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        parent[i] = i;
    }
    auto find = [&](int v) {
        while (parent[v] != v) {
            v = parent[v];
        }
        return v;
    };
    outComponents = vertexCount;
    for (const Edge& e : edges) {
        const int a = find(e.from);
        const int b = find(e.to);
        if (a == b) {
            return false;
        }
        parent[a] = b;
        --outComponents;
    }
    return true;
}

double totalWeight(const std::vector<Edge>& edges) {
    double t = 0.0;
    for (const Edge& e : edges) {
        t += e.weight;
    }
    return t;
}

} // namespace

TEST(EscapeNetworkPrimTest, ConnectedGraphProducesSpanningTree) {
    Graph g = makeGraph(5);
    g.addEdge(0, 1, 2.0);
    g.addEdge(0, 3, 6.0);
    g.addEdge(1, 2, 3.0);
    g.addEdge(1, 3, 8.0);
    g.addEdge(1, 4, 5.0);
    g.addEdge(2, 4, 7.0);
    g.addEdge(3, 4, 9.0);

    PrimResult r = prim(g, 0);
    EXPECT_FALSE(r.graphDisconnected);
    ASSERT_EQ(r.mstEdges.size(), 4u);
    EXPECT_DOUBLE_EQ(totalWeight(r.mstEdges), 16.0);

    int components = 0;
    EXPECT_TRUE(isAcyclic(r.mstEdges, 5, components));
    EXPECT_EQ(components, 1);
}

TEST(EscapeNetworkPrimTest, DisconnectedGraphReturnsFlaggedForest) {
    Graph g = makeGraph(6);
    g.addEdge(0, 1, 1.0);
    g.addEdge(1, 2, 2.0);
    g.addEdge(0, 2, 5.0);
    g.addEdge(3, 4, 4.0);
    // vertex 5 isolated -> 3 components

    PrimResult r = prim(g, 0);
    EXPECT_TRUE(r.graphDisconnected);
    ASSERT_EQ(r.mstEdges.size(), 3u); // V - C = 6 - 3
    EXPECT_DOUBLE_EQ(totalWeight(r.mstEdges), 7.0);

    int components = 0;
    EXPECT_TRUE(isAcyclic(r.mstEdges, 6, components));
    EXPECT_EQ(components, 3);
}

TEST(EscapeNetworkPrimTest, SingleVertexAndEmptyGraph) {
    Graph empty;
    PrimResult r0 = prim(empty, 0);
    EXPECT_TRUE(r0.mstEdges.empty());
    EXPECT_FALSE(r0.graphDisconnected);

    Graph one = makeGraph(1);
    PrimResult r1 = prim(one, 0);
    EXPECT_TRUE(r1.mstEdges.empty());
    EXPECT_FALSE(r1.graphDisconnected);
}

TEST(EscapeNetworkLevel3_2Test, PowerTheDistrictPredefinedGraph) {
    Graph g;
    LevelMetadata meta;
    std::string err;
    ASSERT_TRUE(Level3_2::loadPredefinedGraph(g, meta, err)) << err;
    EXPECT_EQ(meta.levelId, 32);
    EXPECT_TRUE(g.isConnected());

    PowerGridReport report = Level3_2::computePowerGrid(g, Level3_2::DEFAULT_START_VERTEX);
    EXPECT_FALSE(report.graphDisconnected);
    ASSERT_EQ(static_cast<int>(report.mstEdges.size()), g.getVertexCount() - 1);
    EXPECT_DOUBLE_EQ(report.totalWeight, 12.0);

    int components = 0;
    EXPECT_TRUE(isAcyclic(report.mstEdges, g.getVertexCount(), components));
    EXPECT_EQ(components, 1);

    std::vector<int> ids;
    for (const Edge& e : report.mstEdges) {
        ids.push_back(e.id);
    }
    EXPECT_TRUE(Level3_2::verifyPlayerSelection(g, ids));
    EXPECT_FALSE(Level3_2::verifyPlayerSelection(g, {0, 1, 2, 3, 4})); // has cycle 0-1-2
    EXPECT_FALSE(Level3_2::verifyPlayerSelection(g, {0, 2, 3}));       // too few
}
