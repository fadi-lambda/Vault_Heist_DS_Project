#include "Graph.h"
#include <iostream>

int main() {
    EscapeNetwork::Graph graph;
    int v0 = graph.addVertex("Vault", 100.0f, 100.0f);
    int v1 = graph.addVertex("Exit", 300.0f, 100.0f);
    graph.addEdge(v0, v1, 5.0, 10.0, false);

    std::cout << "VaultHeist_EscapeNetwork initialized ("
              << graph.getVertexCount() << " vertices, "
              << graph.getEdgeCount() << " edges)\n";
    return 0;
}
