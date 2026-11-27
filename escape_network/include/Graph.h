#pragma once

#include <string>
#include <vector>

namespace EscapeNetwork {

struct Vertex {
    int id;
    std::string label;
    float x;
    float y;
};

struct Edge {
    int id;
    int from;
    int to;
    double weight;
    double capacity;
    bool directed;
};

struct AdjEntry {
    int to;
    int edgeId;
};

class Graph {
public:
    Graph() = default;

    int addVertex(const std::string& label, float x, float y);
    int addEdge(int from, int to, double weight, double capacity = 0.0, bool directed = false);

    const std::vector<AdjEntry>& getNeighbors(int vertexId) const;
    int getVertexCount() const;
    int getEdgeCount() const;

    bool hasVertex(int vertexId) const;
    const Vertex* getVertex(int vertexId) const;
    const Edge* getEdge(int edgeId) const;
    const std::vector<Vertex>& getVertices() const;
    const std::vector<Edge>& getEdges() const;

private:
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    std::vector<std::vector<AdjEntry>> adj;
    int nextVertexId = 0;
    int nextEdgeId = 0;
};

} // namespace EscapeNetwork
