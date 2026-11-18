#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include "gui/LayoutEngine.h"
#include "gui/Ui.h"
#include "events/Event.h"

namespace VaultHeist {

// Highlight style constants (no enum allowed)
static const int HL_NONE    = 0;
static const int HL_VISIT   = 1;
static const int HL_COMPARE = 2;
static const int HL_INSERT  = 3;
static const int HL_DELETE  = 4;
static const int HL_SUCCESS = 5;
static const int HL_SWAP    = 6;

struct RenderHighlightState {
    int primaryNodeId;
    int secondaryNodeId;
    int selectedNodeId;
    int hoveredNodeId;
    int prunedSubtreeId;
    int primaryStyle;
    int arrayHl1;
    int arrayHl2;
    bool showBadges;
    int markedNodeIds[16];
    int markedCount;

    RenderHighlightState()
        : primaryNodeId(-1), secondaryNodeId(-1), selectedNodeId(-1),
          hoveredNodeId(-1), prunedSubtreeId(-1), primaryStyle(HL_NONE),
          arrayHl1(-1), arrayHl2(-1), showBadges(true), markedCount(0) {
        for (int i = 0; i < 16; ++i) markedNodeIds[i] = -1;
    }

    void addMarked(int id) {
        if (id < 0 || markedCount >= 16) return;
        for (int i = 0; i < markedCount; ++i) {
            if (markedNodeIds[i] == id) return;
        }
        markedNodeIds[markedCount++] = id;
    }

    bool isMarked(int id, int& outOrder) const {
        for (int i = 0; i < markedCount; ++i) {
            if (markedNodeIds[i] == id) {
                outOrder = i + 1;
                return true;
            }
        }
        return false;
    }
};

class Renderer {
private:
    static const int MAX_LOG_LINE_CACHE = 256;

    mutable UiCache m_ui;
    mutable sf::VertexArray m_edgeVertices;
    mutable sf::VertexArray m_shapeVertices;
    mutable sf::VertexArray m_gridVertices;

    // Event log formatting cache
    mutable int m_cachedLogTotal;
    mutable int m_cachedLogBatches;
    mutable int m_cachedLogFirstType;
    mutable int m_cachedLogFirstKey;
    mutable int m_cachedLogLastType;
    mutable int m_cachedLogLastKey;
    mutable float m_cachedLogW;
    mutable std::string m_cachedLogLines[MAX_LOG_LINE_CACHE];
    mutable int m_cachedHdrTotal;
    mutable int m_cachedHdrCursor;
    mutable int m_cachedHdrBatches;
    mutable std::string m_cachedLogHeader;

    // Heap / speed / tooltip string caches
    mutable int m_cachedHeapActive;
    mutable int m_cachedHeapSorted;
    mutable std::string m_cachedHeapLabel;

    mutable int m_cachedSpeedTenths;
    mutable std::string m_cachedSpeedStr;

    mutable int m_cachedTipId;
    mutable int m_cachedTipKey;
    mutable int m_cachedTipType;
    mutable int m_cachedTipH;
    mutable int m_cachedTipBf;
    mutable int m_cachedTipSz;
    mutable int m_cachedTipD;
    mutable bool m_cachedTipViol;
    mutable std::string m_cachedTipLine1;
    mutable std::string m_cachedTipLine2;

public:
    Renderer()
        : m_edgeVertices(sf::PrimitiveType::Lines),
          m_shapeVertices(sf::PrimitiveType::Triangles),
          m_gridVertices(sf::PrimitiveType::Lines),
          m_cachedLogTotal(-1), m_cachedLogBatches(-1),
          m_cachedLogFirstType(-1), m_cachedLogFirstKey(-999999),
          m_cachedLogLastType(-1), m_cachedLogLastKey(-999999),
          m_cachedLogW(-1.f),
          m_cachedHdrTotal(-1), m_cachedHdrCursor(-2), m_cachedHdrBatches(-1),
          m_cachedHeapActive(-1), m_cachedHeapSorted(-1),
          m_cachedSpeedTenths(-1),
          m_cachedTipId(-2), m_cachedTipKey(0), m_cachedTipType(-1),
          m_cachedTipH(0), m_cachedTipBf(0), m_cachedTipSz(0),
          m_cachedTipD(0), m_cachedTipViol(false) {}

    void drawCanvasGrid(sf::RenderWindow& window, const sf::View& canvasView) const;

    void drawTreeLayout(sf::RenderWindow& window, const sf::Font& font,
                        const LayoutResult& layout, const RenderHighlightState& hl,
                        bool showBfBadge, bool showIdBadge) const;

    void drawHeapLayout(sf::RenderWindow& window, const sf::Font& font,
                        const LayoutResult& layout, const RenderHighlightState& hl,
                        float arrayLabelX, float arrayLabelY) const;

    void drawEventLogPanel(sf::RenderWindow& window, const sf::Font& font,
                           float x, float y, float w, float h,
                           const EventBatch& batch, int cursor, int totalBatchesLogged) const;

    void drawPlaybackControls(sf::RenderWindow& window, const sf::Font& font,
                              float panelX, float panelY, float panelW, float panelH,
                              bool isPlaying, float speedMultiplier,
                              int cursor, int totalEvents,
                              const UiInput& in) const;

    void drawNodeTooltip(sf::RenderWindow& window, const sf::Font& font,
                         const LayoutResult& layout, int hoveredNodeId,
                         float windowW, float windowH,
                         float screenNodeX = -99999.f, float screenNodeY = -99999.f) const;
};

} // namespace VaultHeist
