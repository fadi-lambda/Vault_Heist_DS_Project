#pragma once

#include "Graph.h"
#include <string>
#include <vector>

namespace EscapeNetwork {

struct LevelMetadata {
    int levelId = 0;
    int act = 0;
    std::string title;
    std::string conceptText;
    std::string objective;
    std::string controls;
    std::string startStatus;
    std::vector<std::string> hints;
};

class LevelManager {
public:
    LevelManager() = default;

    bool loadLevelFromJson(const std::string& path, Graph& outGraph, std::string& err) const;
    bool loadLevelFromJson(const std::string& path,
                           Graph& outGraph,
                           LevelMetadata& outMeta,
                           std::string& err) const;
    bool loadLevelById(int levelId,
                       Graph& outGraph,
                       LevelMetadata& outMeta,
                       std::string& err) const;

    static std::string levelJsonRelativePath(int levelId);
};

} // namespace EscapeNetwork
