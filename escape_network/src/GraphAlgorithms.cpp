#include "GraphAlgorithms.h"

#include <deque>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

namespace EscapeNetwork {

namespace {

std::string vertexName(const Graph& graph, int vertexId) {
    const Vertex* v = graph.getVertex(vertexId);
    if (!v) {
        return std::to_string(vertexId);
    }
    return v->label + " (" + std::to_string(vertexId) + ")";
}

std::string joinIds(const std::vector<int>& ids) {
    std::string out;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += std::to_string(ids[i]);
    }
    return out;
}

std::vector<int> bfsImpl(const Graph& graph, int startVertexId, std::vector<TraversalStep>* steps) {
    std::vector<int> order;
    if (steps) {
        steps->clear();
    }
    if (!graph.hasVertex(startVertexId)) {
        return order;
    }

    std::vector<bool> discovered(graph.getVertexCount(), false);
    std::deque<int> queue;
    discovered[startVertexId] = true;
    queue.push_back(startVertexId);

    if (steps) {
        TraversalStep init;
        init.frontierVertexIds.push_back(startVertexId);
        init.label = "BFS start: enqueue " + vertexName(graph, startVertexId);
        steps->push_back(init);
    }

    while (!queue.empty()) {
        const int u = queue.front();
        queue.pop_front();
        order.push_back(u);

        std::vector<int> enqueued;
        for (const AdjEntry& entry : graph.getNeighbors(u)) {
            if (!discovered[entry.to]) {
                discovered[entry.to] = true;
                queue.push_back(entry.to);
                enqueued.push_back(entry.to);
            }
        }

        if (steps) {
            TraversalStep step;
            step.currentVertexId = u;
            step.frontierVertexIds.assign(queue.begin(), queue.end());
            step.visitedVertexIds = order;
            step.label = "Dequeue " + vertexName(graph, u);
            step.label += enqueued.empty() ? "; no new neighbors" : "; enqueue " + joinIds(enqueued);
            steps->push_back(step);
        }
    }
    return order;
}

std::vector<int> dfsImpl(const Graph& graph, int startVertexId, std::vector<TraversalStep>* steps) {
    std::vector<int> order;
    if (steps) {
        steps->clear();
    }
    if (!graph.hasVertex(startVertexId)) {
        return order;
    }

    const int n = graph.getVertexCount();
    std::vector<bool> visited(n, false);
    std::vector<int> stack; // back() is top
    stack.push_back(startVertexId);

    // Frontier snapshot: unvisited stack entries, top first, without duplicates.
    auto snapshotFrontier = [&]() {
        std::vector<int> frontier;
        std::vector<bool> seen(n, false);
        for (int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            const int v = stack[i];
            if (!visited[v] && !seen[v]) {
                seen[v] = true;
                frontier.push_back(v);
            }
        }
        return frontier;
    };

    if (steps) {
        TraversalStep init;
        init.frontierVertexIds = snapshotFrontier();
        init.label = "DFS start: push " + vertexName(graph, startVertexId);
        steps->push_back(init);
    }

    while (!stack.empty()) {
        const int u = stack.back();
        stack.pop_back();
        if (visited[u]) {
            continue;
        }
        visited[u] = true;
        order.push_back(u);

        // Push neighbors in reverse so the first adjacency entry is explored
        // first (same order as the recursive formulation).
        const std::vector<AdjEntry>& nbrs = graph.getNeighbors(u);
        std::vector<int> pushed;
        for (int i = static_cast<int>(nbrs.size()) - 1; i >= 0; --i) {
            if (!visited[nbrs[i].to]) {
                stack.push_back(nbrs[i].to);
                pushed.insert(pushed.begin(), nbrs[i].to);
            }
        }

        if (steps) {
            TraversalStep step;
            step.currentVertexId = u;
            step.frontierVertexIds = snapshotFrontier();
            step.visitedVertexIds = order;
            step.label = "Visit " + vertexName(graph, u);
            step.label += pushed.empty() ? "; dead end, backtrack" : "; push " + joinIds(pushed);
            steps->push_back(step);
        }
    }
    return order;
}

} // namespace

std::vector<int> bfsOrder(const Graph& graph, int startVertexId) {
    return bfsImpl(graph, startVertexId, nullptr);
}

std::vector<int> dfsOrder(const Graph& graph, int startVertexId) {
    return dfsImpl(graph, startVertexId, nullptr);
}

std::vector<int> bfsOrder(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps) {
    return bfsImpl(graph, startVertexId, &outSteps);
}

std::vector<int> dfsOrder(const Graph& graph, int startVertexId, std::vector<TraversalStep>& outSteps) {
    return dfsImpl(graph, startVertexId, &outSteps);
}

DijkstraResult dijkstra(const Graph& graph, int sourceVertexId) {
    DijkstraResult result;

    // Reject negative weights up front -- never run partially.
    for (const Edge& e : graph.getEdges()) {
        if (e.weight < 0.0) {
            result.rejectedNegativeWeight = true;
            return result;
        }
    }

    const int n = graph.getVertexCount();
    const double INF = std::numeric_limits<double>::infinity();
    result.dist.assign(n, INF);
    result.prev.assign(n, -1);
    if (!graph.hasVertex(sourceVertexId)) {
        return result;
    }

    using HeapItem = std::pair<double, int>; // (distance, vertex)
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> heap;
    std::vector<bool> settled(n, false);

    result.dist[sourceVertexId] = 0.0;
    heap.push(HeapItem(0.0, sourceVertexId));

    while (!heap.empty()) {
        const HeapItem top = heap.top();
        heap.pop();
        const int u = top.second;
        if (settled[u]) {
            continue; // stale entry (lazy deletion)
        }
        settled[u] = true;

        for (const AdjEntry& entry : graph.getNeighbors(u)) {
            const Edge* edge = graph.getEdge(entry.edgeId);
            if (!edge || settled[entry.to]) {
                continue;
            }
            const double candidate = result.dist[u] + edge->weight;
            if (candidate < result.dist[entry.to]) {
                result.dist[entry.to] = candidate;
                result.prev[entry.to] = u;
                heap.push(HeapItem(candidate, entry.to));
            }
        }
    }
    return result;
}

std::vector<int> extractPath(const DijkstraResult& result, int targetVertexId) {
    std::vector<int> path;
    if (result.rejectedNegativeWeight || targetVertexId < 0 ||
        targetVertexId >= static_cast<int>(result.dist.size()) ||
        result.dist[targetVertexId] == std::numeric_limits<double>::infinity()) {
        return path;
    }
    for (int v = targetVertexId; v != -1; v = result.prev[v]) {
        path.insert(path.begin(), v);
    }
    return path;
}

} // namespace EscapeNetwork
