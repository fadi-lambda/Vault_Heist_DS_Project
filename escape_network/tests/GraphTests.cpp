#include <gtest/gtest.h>

#include "Graph.h"
#include "Level1_1.h"
#include "LevelManager.h"
#include "ScoreManager.h"
#include "Validator.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace EscapeNetwork;

// =============================================================================
// Graph Connectivity & Components
// =============================================================================

TEST(EscapeNetworkGraphTest, EmptyGraphConnectivityAndCycle) {
    Graph g;
    EXPECT_TRUE(g.isConnected());
    EXPECT_TRUE(g.getConnectedComponents().empty());
    EXPECT_FALSE(g.hasCycle());
}

TEST(EscapeNetworkGraphTest, SingleVertexGraph) {
    Graph g;
    int v0 = g.addVertex("Vault", 100.0f, 100.0f);
    EXPECT_EQ(v0, 0);
    EXPECT_TRUE(g.isConnected());
    EXPECT_FALSE(g.hasCycle());

    auto comps = g.getConnectedComponents();
    ASSERT_EQ(comps.size(), 1u);
    EXPECT_EQ(comps[0], std::vector<int>({0}));
}

TEST(EscapeNetworkGraphTest, ConnectedUndirectedGraph) {
    Graph g;
    for (int i = 0; i < 4; ++i) {
        g.addVertex("V" + std::to_string(i), static_cast<float>(i * 100), 0.0f);
    }
    g.addEdge(0, 1, 1.0, 0.0, false);
    g.addEdge(1, 2, 1.0, 0.0, false);
    g.addEdge(2, 3, 1.0, 0.0, false);

    EXPECT_TRUE(g.isConnected());
    auto comps = g.getConnectedComponents();
    ASSERT_EQ(comps.size(), 1u);
    EXPECT_EQ(comps[0].size(), 4u);
}

TEST(EscapeNetworkGraphTest, DisconnectedUndirectedGraph) {
    Graph g;
    for (int i = 0; i < 5; ++i) {
        g.addVertex("V" + std::to_string(i), static_cast<float>(i * 100), 0.0f);
    }
    // Component 1: {0, 1}
    g.addEdge(0, 1, 1.0, 0.0, false);
    // Component 2: {2, 3}
    g.addEdge(2, 3, 1.0, 0.0, false);
    // Component 3: {4} isolated

    EXPECT_FALSE(g.isConnected());
    auto comps = g.getConnectedComponents();
    ASSERT_EQ(comps.size(), 3u);
    EXPECT_EQ(comps[0], std::vector<int>({0, 1}));
    EXPECT_EQ(comps[1], std::vector<int>({2, 3}));
    EXPECT_EQ(comps[2], std::vector<int>({4}));
}

// =============================================================================
// Cycle Detection (Undirected & Directed)
// =============================================================================

TEST(EscapeNetworkGraphTest, UndirectedAcyclicTree) {
    Graph g;
    for (int i = 0; i < 4; ++i) {
        g.addVertex("V" + std::to_string(i), 0.0f, 0.0f);
    }
    g.addEdge(0, 1, 1.0, 0.0, false);
    g.addEdge(0, 2, 1.0, 0.0, false);
    g.addEdge(1, 3, 1.0, 0.0, false);

    EXPECT_FALSE(g.hasCycle());
}

TEST(EscapeNetworkGraphTest, UndirectedCyclicGraph) {
    Graph g;
    for (int i = 0; i < 4; ++i) {
        g.addVertex("V" + std::to_string(i), 0.0f, 0.0f);
    }
    g.addEdge(0, 1, 1.0, 0.0, false);
    g.addEdge(1, 2, 1.0, 0.0, false);
    g.addEdge(2, 0, 1.0, 0.0, false); // forms triangle 0-1-2
    // vertex 3 is disconnected

    EXPECT_FALSE(g.isConnected());
    EXPECT_TRUE(g.hasCycle());
}

TEST(EscapeNetworkGraphTest, DirectedAcyclicGraph) {
    // Diamond DAG: 0 -> 1 -> 3 and 0 -> 2 -> 3 (acyclic!)
    Graph g;
    for (int i = 0; i < 4; ++i) {
        g.addVertex("V" + std::to_string(i), 0.0f, 0.0f);
    }
    g.addEdge(0, 1, 1.0, 0.0, true);
    g.addEdge(0, 2, 1.0, 0.0, true);
    g.addEdge(1, 3, 1.0, 0.0, true);
    g.addEdge(2, 3, 1.0, 0.0, true);

    EXPECT_TRUE(g.isConnected());
    EXPECT_FALSE(g.hasCycle());
}

TEST(EscapeNetworkGraphTest, DirectedCyclicGraph) {
    Graph g;
    for (int i = 0; i < 4; ++i) {
        g.addVertex("V" + std::to_string(i), 0.0f, 0.0f);
    }
    g.addEdge(0, 1, 1.0, 0.0, true);
    g.addEdge(1, 2, 1.0, 0.0, true);
    g.addEdge(2, 3, 1.0, 0.0, true);
    g.addEdge(3, 1, 1.0, 0.0, true); // back edge 3 -> 1

    EXPECT_TRUE(g.hasCycle());
}

TEST(EscapeNetworkGraphTest, DirectedSelfLoopIsCycle) {
    Graph g;
    g.addVertex("Self", 0.0f, 0.0f);
    g.addEdge(0, 0, 1.0, 0.0, true);
    EXPECT_TRUE(g.hasCycle());
}

// =============================================================================
// Level 1.1 ("Map the City") Pure Logic
// =============================================================================

TEST(EscapeNetworkLevel1_1Test, InspectsGraphRepresentation) {
    Graph g;
    g.addVertex("Vault", 100.0f, 100.0f);
    g.addVertex("Alley", 200.0f, 100.0f);
    g.addVertex("Exit",  300.0f, 100.0f);
    g.addEdge(0, 1, 4.0, 0.0, false);
    g.addEdge(1, 2, 5.0, 0.0, false);

    EXPECT_EQ(Level1_1::getVertexCount(g), 3);
    EXPECT_EQ(Level1_1::getEdgeCount(g), 2);

    DegreeInfo deg1 = Level1_1::getDegreeInfo(g, 1);
    EXPECT_EQ(deg1.vertexId, 1);
    EXPECT_EQ(deg1.degree, 2);
    EXPECT_EQ(deg1.neighborIds, std::vector<int>({0, 2}));

    DegreeInfo invalid = Level1_1::getDegreeInfo(g, 99);
    EXPECT_EQ(invalid.vertexId, 99);
    EXPECT_EQ(invalid.degree, 0);
    EXPECT_TRUE(invalid.neighborIds.empty());

    std::vector<AdjListRow> dump = Level1_1::getAdjacencyListDump(g);
    ASSERT_EQ(dump.size(), 3u);
    EXPECT_EQ(dump[0].label, "Vault");
    EXPECT_EQ(dump[0].neighborIds, std::vector<int>({1}));
    EXPECT_EQ(dump[1].neighborIds, std::vector<int>({0, 2}));
    EXPECT_EQ(dump[2].neighborIds, std::vector<int>({1}));

    GraphSummary summary = Level1_1::getSummary(g);
    EXPECT_EQ(summary.vertexCount, 3);
    EXPECT_EQ(summary.edgeCount, 2);
    EXPECT_TRUE(summary.connected);
    EXPECT_FALSE(summary.hasCycle);
}

// =============================================================================
// LevelManager + level_1_1.json
// =============================================================================

TEST(EscapeNetworkLevelManagerTest, LoadsLevel1_1Json) {
    LevelManager manager;
    Graph graph;
    LevelMetadata meta;
    std::string err;

    ASSERT_TRUE(manager.loadLevelById(11, graph, meta, err)) << err;
    EXPECT_EQ(meta.levelId, 11);
    EXPECT_EQ(meta.act, 1);
    EXPECT_EQ(meta.title, "1.1 Map the City");
    EXPECT_FALSE(meta.objective.empty());
    EXPECT_FALSE(meta.hints.empty());

    EXPECT_EQ(graph.getVertexCount(), 6);
    EXPECT_EQ(graph.getEdgeCount(), 8);
    EXPECT_TRUE(graph.isConnected());
    EXPECT_TRUE(graph.hasCycle());

    const Vertex* v0 = graph.getVertex(0);
    ASSERT_NE(v0, nullptr);
    EXPECT_EQ(v0->label, "Vault Gate");
    EXPECT_FLOAT_EQ(v0->x, 160.0f);
    EXPECT_FLOAT_EQ(v0->y, 260.0f);

    DegreeInfo d0 = Level1_1::getDegreeInfo(graph, 0);
    EXPECT_EQ(d0.degree, 2);
}

TEST(EscapeNetworkLevelManagerTest, FailsGracefullyOnMissingFile) {
    LevelManager manager;
    Graph graph;
    std::string err;

    EXPECT_FALSE(manager.loadLevelFromJson("nonexistent_level.json", graph, err));
    EXPECT_FALSE(err.empty());
}

// =============================================================================
// Skeletons (ScoreManager & Validator)
// =============================================================================

TEST(EscapeNetworkSkeletonsTest, ScoreManagerAndValidatorShellsCompileAndReset) {
    ScoreManager score;
    EXPECT_EQ(score.getMistakeCount(), 0);
    score.recordMistake();
    EXPECT_EQ(score.getMistakeCount(), 1);
    score.reset();
    EXPECT_EQ(score.getMistakeCount(), 0);
    EXPECT_EQ(score.calculateStars(10, 10), 3);
    EXPECT_EQ(ScoreManager::computeRouteHeatCapacity(11.0), 19); // ceil(1.5 * 11) + 2 = 17 + 2 = 19
    EXPECT_EQ(score.calculateRouteStars(11.0, 11.0, 11, 19, true), 3);
    score.recordMistake();
    EXPECT_EQ(score.calculateRouteStars(11.0, 13.0, 13, 19, true), 2);
    EXPECT_EQ(score.calculateNonRouteStars(true, 3), 2);
    EXPECT_EQ(score.calculateRouteStars(11.0, 11.0, 20, 19, true), 0);

    Validator validator;
    Graph g;
    ValidationResult stepRes = validator.validateStep(g, 0);
    EXPECT_FALSE(stepRes.valid);
    ValidationResult subRes = validator.validateSubmission(g, {0});
    EXPECT_FALSE(subRes.valid);
    validator.reset();
}

TEST(EscapeNetworkLevelManagerTest, AllTenPredefinedLevelsLoadCleanly) {
    LevelManager manager;
    const int levelIds[10] = { 11, 12, 21, 22, 31, 32, 41, 42, 51, 52 };
    for (int id : levelIds) {
        Graph g;
        LevelMetadata meta;
        std::string err;
        EXPECT_TRUE(manager.loadLevelById(id, g, meta, err)) << "Level " << id << " failed: " << err;
        EXPECT_EQ(meta.levelId, id);
        EXPECT_FALSE(meta.title.empty());
        EXPECT_GE(g.getVertexCount(), 5);
        EXPECT_GE(g.getEdgeCount(), 5);
    }
}
