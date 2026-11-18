#include "gui/ScoreLog.h"
#include <fstream>
#include <cstdlib>

namespace VaultHeist {

void ScoreLog::load(const std::string& filePath) {
    path = filePath;
    count = 0;
    std::ifstream in(path);
    if (!in.is_open()) return;

    std::string line;
    while (count < MAX_SCORE_ENTRIES && std::getline(in, line)) {
        size_t a = line.find('|');
        if (a == std::string::npos) continue;
        size_t b = line.find('|', a + 1);
        if (b == std::string::npos) continue;
        size_t c = line.find('|', b + 1);
        if (c == std::string::npos) continue;

        ScoreEntry e;
        e.score = std::atoi(line.substr(0, a).c_str());
        e.stars = std::atoi(line.substr(a + 1, b - a - 1).c_str());
        e.alarm = std::atoi(line.substr(b + 1, c - b - 1).c_str());
        e.levelTitle = line.substr(c + 1);
        while (!e.levelTitle.empty() && (e.levelTitle.back() == '\r' || e.levelTitle.back() == '\n'))
            e.levelTitle.pop_back();
        entries[count++] = e;
    }
}

bool ScoreLog::writeFile() const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;
    for (int i = 0; i < count; ++i) {
        out << entries[i].score << '|' << entries[i].stars << '|' << entries[i].alarm << '|'
            << entries[i].levelTitle << '\n';
    }
    return out.good();
}

bool ScoreLog::append(const ScoreEntry& e) {
    if (count >= MAX_SCORE_ENTRIES) {
        for (int i = 1; i < MAX_SCORE_ENTRIES; ++i) entries[i - 1] = entries[i];
        count = MAX_SCORE_ENTRIES - 1;
    }
    entries[count++] = e;
    return writeFile();
}

int ScoreLog::topIndex(int rank) const {
    if (rank < 0 || rank >= count) return -1;
    bool used[MAX_SCORE_ENTRIES];
    for (int i = 0; i < MAX_SCORE_ENTRIES; ++i) used[i] = false;

    int pick = -1;
    for (int r = 0; r <= rank; ++r) {
        pick = -1;
        for (int i = 0; i < count; ++i) {
            if (used[i]) continue;
            if (pick < 0 || entries[i].score > entries[pick].score) pick = i;
        }
        if (pick < 0) return -1;
        used[pick] = true;
    }
    return pick;
}

} // namespace VaultHeist
