#include "Graph.h"
#include <queue>

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

namespace {

bool hasCycleDirectedDfs(int u,
                         const std::vector<std::vector<AdjEntry>>& adj,
                         std::vector<int>& color) {
    color[u] = 1; // visiting (on recursion stack)
    for (const AdjEntry& entry : adj[u]) {
        if (color[entry.to] == 1) {
            return true;
        }
        if (color[entry.to] == 0 && hasCycleDirectedDfs(entry.to, adj, color)) {
            return true;
        }
    }
    color[u] = 2; // visited
    return false;
}

bool hasCycleUndirectedDfs(int u,
                           int parentEdgeId,
                           const std::vector<std::vector<AdjEntry>>& adj,
                           std::vector<bool>& visited) {
    visited[u] = true;
    for (const AdjEntry& entry : adj[u]) {
        if (entry.edgeId == parentEdgeId) {
            continue;
        }
        if (visited[entry.to]) {
            return true;
        }
        if (hasCycleUndirectedDfs(entry.to, entry.edgeId, adj, visited)) {
            return true;
        }
    }
    return false;
}

} // namespace

bool Graph::isConnected() const {
    if (vertices.empty()) {
        return true;
    }

    std::vector<bool> visited(vertices.size(), false);
    std::queue<int> q;
    visited[0] = true;
    q.push(0);
    int visitedCount = 1;

    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (const AdjEntry& entry : adj[u]) {
            if (!visited[entry.to]) {
                visited[entry.to] = true;
                ++visitedCount;
                q.push(entry.to);
            }
        }
    }

    return visitedCount == static_cast<int>(vertices.size());
}

std::vector<std::vector<int>> Graph::getConnectedComponents() const {
    std::vector<std::vector<int>> components;
    std::vector<bool> visited(vertices.size(), false);

    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        if (visited[i]) {
            continue;
        }
        std::vector<int> component;
        std::queue<int> q;
        visited[i] = true;
        q.push(i);

        while (!q.empty()) {
            const int u = q.front();
            q.pop();
            component.push_back(u);
            for (const AdjEntry& entry : adj[u]) {
                if (!visited[entry.to]) {
                    visited[entry.to] = true;
                    q.push(entry.to);
                }
            }
        }
        components.push_back(component);
    }

    return components;
}

bool Graph::hasCycle() const {
    if (edges.empty()) {
        return false;
    }

    const bool isDirected = edges[0].directed;
    if (isDirected) {
        std::vector<int> color(vertices.size(), 0);
        for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
            if (color[i] == 0 && hasCycleDirectedDfs(i, adj, color)) {
                return true;
            }
        }
        return false;
    }

    std::vector<bool> visited(vertices.size(), false);
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        if (!visited[i] && hasCycleUndirectedDfs(i, -1, adj, visited)) {
            return true;
        }
    }
    return false;
}

} // namespace EscapeNetwork
