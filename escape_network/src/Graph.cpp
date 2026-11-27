#include "Graph.h"

namespace EscapeNetwork {

int Graph::addVertex(const std::string& label, float x, float y) {
    const int id = nextVertexId++;
    vertices.push_back(Vertex{id, label, x, y});
    adj.emplace_back();
    return id;
}

int Graph::addEdge(int from, int to, double weight, double capacity, bool directed) {
    if (!hasVertex(from) || !hasVertex(to)) {
        return -1;
    }

    const int id = nextEdgeId++;
    edges.push_back(Edge{id, from, to, weight, capacity, directed});

    adj[from].push_back(AdjEntry{to, id});
    if (!directed && from != to) {
        adj[to].push_back(AdjEntry{from, id});
    }

    return id;
}

const std::vector<AdjEntry>& Graph::getNeighbors(int vertexId) const {
    static const std::vector<AdjEntry> empty;
    if (!hasVertex(vertexId)) {
        return empty;
    }
    return adj[vertexId];
}

int Graph::getVertexCount() const {
    return static_cast<int>(vertices.size());
}

int Graph::getEdgeCount() const {
    return static_cast<int>(edges.size());
}

bool Graph::hasVertex(int vertexId) const {
    return vertexId >= 0 && vertexId < static_cast<int>(vertices.size());
}

const Vertex* Graph::getVertex(int vertexId) const {
    if (!hasVertex(vertexId)) {
        return nullptr;
    }
    return &vertices[vertexId];
}

const Edge* Graph::getEdge(int edgeId) const {
    if (edgeId < 0 || edgeId >= static_cast<int>(edges.size())) {
        return nullptr;
    }
    return &edges[edgeId];
}

const std::vector<Vertex>& Graph::getVertices() const {
    return vertices;
}

const std::vector<Edge>& Graph::getEdges() const {
    return edges;
}

} // namespace EscapeNetwork
