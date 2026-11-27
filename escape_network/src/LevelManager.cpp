#include "LevelManager.h"
#include "Json.h"
#include <fstream>
#include <unordered_map>

namespace EscapeNetwork {

bool LevelManager::loadLevelFromJson(const std::string& path, Graph& outGraph, std::string& err) const {
    LevelMetadata unusedMeta;
    return loadLevelFromJson(path, outGraph, unusedMeta, err);
}

bool LevelManager::loadLevelFromJson(const std::string& path,
                                     Graph& outGraph,
                                     LevelMetadata& outMeta,
                                     std::string& err) const {
    JsonValue* root = parseJsonFile(path, err);
    if (!root) {
        return false;
    }
    if (!root->isObject()) {
        err = path + ": top-level JSON value must be an object";
        delete root;
        return false;
    }

    LevelMetadata meta;
    meta.levelId     = root->getInt("level_id", 0);
    meta.act         = root->getInt("act", 0);
    meta.title       = root->getString("title", "");
    meta.conceptText = root->getString("concept", "");
    meta.objective   = root->getString("objective", "");
    meta.controls    = root->getString("controls", "");
    meta.startStatus = root->getString("start_status", "");

    if (meta.levelId == 0 || meta.title.empty()) {
        err = path + ": missing level_id or title";
        delete root;
        return false;
    }

    const JsonValue* verticesVal = root->get("vertices");
    if (!verticesVal || !verticesVal->isArray()) {
        err = path + ": missing or invalid 'vertices' array";
        delete root;
        return false;
    }

    Graph graph;
    std::unordered_map<int, int> jsonIdToGraphId;

    for (int i = 0; i < verticesVal->size(); ++i) {
        const JsonValue* v = verticesVal->at(i);
        if (!v || !v->isObject()) {
            err = path + ": vertex entry at index " + std::to_string(i) + " must be an object";
            delete root;
            return false;
        }
        const int jsonId = v->getInt("id", i);
        const std::string label = v->getString("label", "V" + std::to_string(jsonId));
        const float x = static_cast<float>(v->getDouble("x", 0.0));
        const float y = static_cast<float>(v->getDouble("y", 0.0));

        if (jsonIdToGraphId.find(jsonId) != jsonIdToGraphId.end()) {
            err = path + ": duplicate vertex id " + std::to_string(jsonId);
            delete root;
            return false;
        }

        const int graphId = graph.addVertex(label, x, y);
        jsonIdToGraphId[jsonId] = graphId;
    }

    const JsonValue* edgesVal = root->get("edges");
    if (edgesVal) {
        if (!edgesVal->isArray()) {
            err = path + ": 'edges' must be an array";
            delete root;
            return false;
        }
        for (int i = 0; i < edgesVal->size(); ++i) {
            const JsonValue* e = edgesVal->at(i);
            if (!e || !e->isObject()) {
                err = path + ": edge entry at index " + std::to_string(i) + " must be an object";
                delete root;
                return false;
            }
            if (!e->get("from") || !e->get("to")) {
                err = path + ": edge entry at index " + std::to_string(i) + " missing 'from' or 'to'";
                delete root;
                return false;
            }
            const int fromJsonId = e->getInt("from", -1);
            const int toJsonId   = e->getInt("to", -1);
            auto fromIt = jsonIdToGraphId.find(fromJsonId);
            auto toIt   = jsonIdToGraphId.find(toJsonId);
            if (fromIt == jsonIdToGraphId.end() || toIt == jsonIdToGraphId.end()) {
                err = path + ": edge at index " + std::to_string(i) + " references unknown vertex";
                delete root;
                return false;
            }

            const double weight   = e->getDouble("weight", 1.0);
            const double capacity = e->getDouble("capacity", 0.0);
            const bool directed   = e->getBool("directed", false);

            graph.addEdge(fromIt->second, toIt->second, weight, capacity, directed);
        }
    }

    const JsonValue* hintsVal = root->get("hints");
    if (hintsVal && hintsVal->isArray()) {
        for (int i = 0; i < hintsVal->size(); ++i) {
            const JsonValue* h = hintsVal->at(i);
            if (h && h->isString()) {
                meta.hints.push_back(h->asString());
            }
        }
    }

    delete root;
    outGraph = std::move(graph);
    outMeta = std::move(meta);
    return true;
}

std::string LevelManager::levelJsonRelativePath(int levelId) {
    const int act = levelId / 10;
    const int idx = levelId % 10;
    return "escape_network/levels/level_" + std::to_string(act) + "_" + std::to_string(idx) + ".json";
}

bool LevelManager::loadLevelById(int levelId,
                                 Graph& outGraph,
                                 LevelMetadata& outMeta,
                                 std::string& err) const {
    static const char* const prefixes[6] = {
        "", "../", "../../", "../../../", "../../../../", "../../../../../"
    };
    const std::string rel = levelJsonRelativePath(levelId);
    for (int i = 0; i < 6; ++i) {
        const std::string path = std::string(prefixes[i]) + rel;
        std::ifstream probe(path);
        if (probe.good()) {
            probe.close();
            return loadLevelFromJson(path, outGraph, outMeta, err);
        }
    }
    err = "level file not found: " + rel + " (searched working dir and up to 5 parents)";
    return false;
}

} // namespace EscapeNetwork
