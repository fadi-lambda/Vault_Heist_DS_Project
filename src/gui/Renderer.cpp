#include "gui/Renderer.h"
#include "gui/AnimationController.h"
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <string_view>

namespace VaultHeist {

static inline std::uint8_t clampByte(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<std::uint8_t>(v);
}

static const int CIRCLE_SEGS = 24;

static void buildUnitCircle(float* cosT, float* sinT) {
    for (int i = 0; i <= CIRCLE_SEGS; ++i) {
        float ang = (2.f * 3.14159265f * static_cast<float>(i)) / static_cast<float>(CIRCLE_SEGS);
        cosT[i] = std::cos(ang);
        sinT[i] = std::sin(ang);
    }
}

static void appendDisc(sf::VertexArray& va, float cx, float cy, float r, sf::Color col,
                       const float* cosT, const float* sinT) {
    for (int i = 0; i < CIRCLE_SEGS; ++i) {
        va.append(sf::Vertex({cx, cy}, col));
        va.append(sf::Vertex({cx + cosT[i] * r, cy + sinT[i] * r}, col));
        va.append(sf::Vertex({cx + cosT[i + 1] * r, cy + sinT[i + 1] * r}, col));
    }
}

static void appendRing(sf::VertexArray& va, float cx, float cy, float rIn, float rOut, sf::Color col,
                       const float* cosT, const float* sinT) {
    for (int i = 0; i < CIRCLE_SEGS; ++i) {
        float c0 = cosT[i], s0 = sinT[i];
        float c1 = cosT[i + 1], s1 = sinT[i + 1];
        sf::Vector2f p00(cx + c0 * rIn,  cy + s0 * rIn);
        sf::Vector2f p01(cx + c0 * rOut, cy + s0 * rOut);
        sf::Vector2f p11(cx + c1 * rOut, cy + s1 * rOut);
        sf::Vector2f p10(cx + c1 * rIn,  cy + s1 * rIn);
        va.append(sf::Vertex(p00, col));
        va.append(sf::Vertex(p01, col));
        va.append(sf::Vertex(p11, col));
        va.append(sf::Vertex(p00, col));
        va.append(sf::Vertex(p11, col));
        va.append(sf::Vertex(p10, col));
    }
}

static void appendQuad(sf::VertexArray& va, float x, float y, float w, float h, sf::Color col) {
    va.append(sf::Vertex({x, y}, col));
    va.append(sf::Vertex({x + w, y}, col));
    va.append(sf::Vertex({x + w, y + h}, col));
    va.append(sf::Vertex({x, y}, col));
    va.append(sf::Vertex({x + w, y + h}, col));
    va.append(sf::Vertex({x, y + h}, col));
}

static void appendOutlinedRect(sf::VertexArray& va, float x, float y, float w, float h,
                               float t, sf::Color fill, sf::Color outline) {
    if (fill.a > 0) appendQuad(va, x, y, w, h, fill);
    if (t > 0.f && outline.a > 0) {
        appendQuad(va, x - t, y - t, w + 2.f * t, t, outline);
        appendQuad(va, x - t, y + h, w + 2.f * t, t, outline);
        appendQuad(va, x - t, y, t, h, outline);
        appendQuad(va, x + w, y, t, h, outline);
    }
}

void Renderer::drawCanvasGrid(sf::RenderWindow& window, const sf::View& canvasView) const {
    sf::Vector2f center = canvasView.getCenter();
    sf::Vector2f size   = canvasView.getSize();
    float left   = center.x - size.x * 0.5f;
    float right  = center.x + size.x * 0.5f;
    float top    = center.y - size.y * 0.5f;
    float bottom = center.y + size.y * 0.5f;

    float step = 32.f;
    while (size.x / step > 120.f || size.y / step > 120.f) {
        step *= 2.f;
    }

    m_gridVertices.clear();
    m_gridVertices.setPrimitiveType(sf::PrimitiveType::Lines);

    sf::Color minorCol(10, 36, 18, 145);
    sf::Color majorCol(18, 66, 32, 195);

    float startX = std::floor(left / step) * step;
    for (float x = startX; x <= right + 0.5f; x += step) {
        long long idx = std::llround(x / step);
        sf::Color c = ((idx % 5) == 0) ? majorCol : minorCol;
        m_gridVertices.append(sf::Vertex({x, top}, c));
        m_gridVertices.append(sf::Vertex({x, bottom}, c));
    }

    float startY = std::floor(top / step) * step;
    for (float y = startY; y <= bottom + 0.5f; y += step) {
        long long idx = std::llround(y / step);
        sf::Color c = ((idx % 5) == 0) ? majorCol : minorCol;
        m_gridVertices.append(sf::Vertex({left, y}, c));
        m_gridVertices.append(sf::Vertex({right, y}, c));
    }

    if (m_gridVertices.getVertexCount() > 0) window.draw(m_gridVertices);
}

void Renderer::drawTreeLayout(sf::RenderWindow& window, const sf::Font& font,
                              const LayoutResult& layout, const RenderHighlightState& hl,
                              bool showBfBadge, bool showIdBadge) const {
    if (layout.nodeCount == 0) {
        Ui::textCenter(window, font, "[ Empty Structure ]", 16, Ui::textMuted(), 600.f, 340.f, false, &m_ui);
        return;
    }

    // 1. Batch all edges into one VertexArray draw call
    m_edgeVertices.clear();
    m_edgeVertices.setPrimitiveType(sf::PrimitiveType::Lines);
    for (int i = 0; i < layout.edgeCount; ++i) {
        const EdgeLayout& e = layout.edges[i];
        sf::Color edgeColor = e.isViolation ? sf::Color(220, 70, 70)
                                            : sf::Color(0, 210, 65);
        m_edgeVertices.append(sf::Vertex({e.x1, e.y1}, edgeColor));
        m_edgeVertices.append(sf::Vertex({e.x2, e.y2}, edgeColor));
    }
    if (m_edgeVertices.getVertexCount() > 0) window.draw(m_edgeVertices);

    // 2. Batch all node circles, outlines, selection rings, and badge circles into ONE VertexArray draw call
    float cosT[CIRCLE_SEGS + 1], sinT[CIRCLE_SEGS + 1];
    buildUnitCircle(cosT, sinT);

    m_shapeVertices.clear();
    m_shapeVertices.setPrimitiveType(sf::PrimitiveType::Triangles);

    struct CachedNodeVis {
        float r;
        std::uint8_t alpha;
        bool isPrimary;
        bool isMarked;
        int markOrder;
    };
    CachedNodeVis vis[MAX_LAYOUT_NODES];

    for (int i = 0; i < layout.nodeCount; ++i) {
        const NodeLayout& n = layout.nodes[i];
        std::uint8_t alpha = clampByte(static_cast<int>(n.alpha * 255.f));
        float r = n.radius * n.scale;

        bool isPrimary  = (n.id == hl.primaryNodeId);
        bool isSecond   = (n.id == hl.secondaryNodeId);
        bool isSelected = (n.id == hl.selectedNodeId);
        bool isHovered  = (n.id == hl.hoveredNodeId);
        bool isPruned   = (n.id == hl.prunedSubtreeId);
        int markOrder = 0;
        bool isMarked   = hl.isMarked(n.id, markOrder);

        sf::Color fill(6, 22, 10, alpha);
        sf::Color outline(0, 255, 65, alpha);
        float thickness = 2.4f;

        if (n.isViolation) {
            fill = sf::Color(80, 25, 25, alpha);
            outline = sf::Color(240, 80, 80, alpha);
        } else if (layout.structureType == STRUCT_MAX_HEAP && n.isRoot) {
            fill = sf::Color(16, 68, 28, alpha);
            outline = sf::Color(0, 255, 65, alpha);
        }

        if (isPruned) {
            fill = sf::Color(12, 42, 32, alpha);
            outline = sf::Color(0, 210, 120, alpha);
            thickness = 3.2f;
        }
        if (isSecond) {
            fill = sf::Color(12, 54, 24, alpha);
            outline = sf::Color(80, 255, 120, alpha);
            thickness = 3.2f;
        }
        if (isMarked && !isPrimary) {
            fill = sf::Color(16, 64, 30, alpha);
            outline = sf::Color(0, 255, 65, alpha);
            thickness = 3.0f;
        }
        if (isPrimary) {
            thickness = 3.8f;
            if (hl.primaryStyle == HL_INSERT || hl.primaryStyle == HL_SUCCESS) {
                fill = sf::Color(16, 82, 34, alpha);
                outline = sf::Color(0, 255, 65, alpha);
            } else if (hl.primaryStyle == HL_DELETE) {
                fill = sf::Color(115, 32, 32, alpha);
                outline = sf::Color(255, 95, 95, alpha);
            } else if (hl.primaryStyle == HL_COMPARE) {
                fill = sf::Color(18, 72, 30, alpha);
                outline = sf::Color(120, 255, 140, alpha);
            } else if (hl.primaryStyle == HL_SWAP) {
                fill = sf::Color(24, 78, 42, alpha);
                outline = sf::Color(0, 255, 110, alpha);
            } else {
                fill = sf::Color(16, 76, 30, alpha);
                outline = sf::Color(0, 255, 65, alpha);
            }
        }
        if (isHovered) {
            r *= 1.12f;
            if (!isPrimary) {
                outline = sf::Color(120, 255, 145, alpha);
                thickness = 3.2f;
            }
        }

        vis[i].r = r;
        vis[i].alpha = alpha;
        vis[i].isPrimary = isPrimary;
        vis[i].isMarked = isMarked;
        vis[i].markOrder = markOrder;

        if (isSelected || isMarked) {
            sf::Color selCol = isSelected ? sf::Color(0, 255, 65, alpha) : sf::Color(70, 220, 130, alpha);
            appendRing(m_shapeVertices, n.x, n.y, r + 5.f, r + 7.6f, selCol, cosT, sinT);
        }

        // Layered soft glow ring behind/around node circle
        std::uint8_t glowA = clampByte((static_cast<int>(alpha) * ((isSelected || isPrimary || isHovered) ? 95 : 55)) / 255);
        sf::Color glowCol(outline.r, outline.g, outline.b, glowA);
        appendRing(m_shapeVertices, n.x, n.y, r + thickness, r + thickness + 4.5f, glowCol, cosT, sinT);

        appendDisc(m_shapeVertices, n.x, n.y, r, fill, cosT, sinT);
        appendRing(m_shapeVertices, n.x, n.y, r, r + thickness, outline, cosT, sinT);

        if (isMarked) {
            float mx = n.x - r * 0.85f, my = n.y - r * 0.85f;
            appendDisc(m_shapeVertices, mx, my, 9.f, sf::Color(22, 120, 68, alpha), cosT, sinT);
            appendRing(m_shapeVertices, mx, my, 9.f, 10.4f, sf::Color(255, 255, 255, alpha), cosT, sinT);
        }

        if (hl.showBadges && showBfBadge) {
            float bx = n.x + r * 0.85f, by = n.y - r * 0.85f;
            sf::Color bCol = (n.bf >= -1 && n.bf <= 1) ? sf::Color(40, 180, 80, alpha)
                                                       : sf::Color(220, 50, 50, alpha);
            appendDisc(m_shapeVertices, bx, by, 9.f, bCol, cosT, sinT);
            appendRing(m_shapeVertices, bx, by, 9.f, 10.4f, sf::Color(255, 255, 255, alpha), cosT, sinT);
        }
    }
    if (m_shapeVertices.getVertexCount() > 0) window.draw(m_shapeVertices);

    // 3. Draw node text labels using persistent TextPool (zero setString rebuilds on steady frames)
    char buf[48];
    for (int i = 0; i < layout.nodeCount; ++i) {
        const NodeLayout& n = layout.nodes[i];
        float r = vis[i].r;
        std::uint8_t alpha = vis[i].alpha;

        int len = std::snprintf(buf, sizeof(buf), "%d", n.key);
        bool darkText = false;
        sf::Color keyCol = sf::Color(255, 255, 255, alpha);
        if (len > 0) {
            Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                               14, false, TEXT_ALIGN_CENTER_XY, keyCol, n.x, n.y);
        }

        if (vis[i].isMarked) {
            len = std::snprintf(buf, sizeof(buf), "%d", vis[i].markOrder);
            if (len > 0) {
                Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                                   10, false, TEXT_ALIGN_CENTER_XY, sf::Color(255, 255, 255, alpha),
                                   n.x - r * 0.85f, n.y - r * 0.85f);
            }
        }

        if (hl.showBadges) {
            if (showBfBadge) {
                len = std::snprintf(buf, sizeof(buf), "%d", n.bf);
                if (len > 0) {
                    Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                                       10, false, TEXT_ALIGN_CENTER_XY, sf::Color(255, 255, 255, alpha),
                                       n.x + r * 0.85f, n.y - r * 0.85f);
                }
            } else if (layout.structureType != STRUCT_MAX_HEAP) {
                len = std::snprintf(buf, sizeof(buf), "h%d", n.height);
                if (len > 0) {
                    Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                                       9, false, TEXT_ALIGN_TOP_LEFT, sf::Color(145, 205, 245, alpha),
                                       std::round(n.x + r * 0.72f), std::round(n.y - r - 2.f));
                }
            }

            len = 0;
            if (layout.structureType == STRUCT_MAX_HEAP) {
                len = std::snprintf(buf, sizeof(buf), "[%d]", n.id);
            } else if (showBfBadge) {
                len = std::snprintf(buf, sizeof(buf), "sz:%d", n.size);
            } else if (showIdBadge) {
                len = std::snprintf(buf, sizeof(buf), "#%d", n.id);
            }
            if (len > 0) {
                Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                                   10, false, TEXT_ALIGN_BOTTOM_X, sf::Color(150, 180, 220, alpha),
                                   n.x, n.y + r + 3.f);
            }
        }
    }
}

void Renderer::drawHeapLayout(sf::RenderWindow& window, const sf::Font& font,
                              const LayoutResult& layout, const RenderHighlightState& hl,
                              float arrayLabelX, float arrayLabelY) const {
    if (layout.nodeCount > 0) {
        drawTreeLayout(window, font, layout, hl, false, false);
    }

    int activeCount = 0;
    int sortedCount = 0;
    for (int i = 0; i < layout.cellCount; ++i) {
        if (layout.cells[i].isSorted) sortedCount++;
        else activeCount++;
    }

    if (m_cachedHeapActive != activeCount || m_cachedHeapSorted != sortedCount) {
        m_cachedHeapActive = activeCount;
        m_cachedHeapSorted = sortedCount;
        m_cachedHeapLabel = "SYNCHRONISED ARRAY VIEW  [Active: " + std::to_string(activeCount) +
            (sortedCount > 0 ? (" | Sorted: " + std::to_string(sortedCount)) : "") + "]";
    }

    Ui::drawPooledText(window, &m_ui, font, m_cachedHeapLabel, 12, false, TEXT_ALIGN_TOP_LEFT,
                       sf::Color(200, 220, 255), arrayLabelX, arrayLabelY);

    // Batch all array cell borders + fills into ONE VertexArray draw call
    m_shapeVertices.clear();
    m_shapeVertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    for (int i = 0; i < layout.cellCount; ++i) {
        const ArrayCellLayout& c = layout.cells[i];
        sf::Color fill, edge;
        if (c.isSorted) {
            fill = sf::Color(12, 46, 22);
            edge = sf::Color(0, 255, 65);
        } else {
            bool isHl = (c.index == hl.arrayHl1 || c.index == hl.arrayHl2);
            if (isHl) {
                fill = sf::Color(18, 72, 32);
                edge = sf::Color(120, 255, 140);
            } else if (c.isRoot) {
                fill = sf::Color(14, 54, 24);
                edge = sf::Color(0, 255, 65);
            } else {
                fill = sf::Color(6, 22, 10);
                edge = sf::Color(0, 210, 65);
            }
        }
        appendOutlinedRect(m_shapeVertices, c.x - 2.f, c.y - 2.f, c.w + 4.f, c.h + 4.f, 2.5f,
                           sf::Color(0, 0, 0, 0), sf::Color(edge.r, edge.g, edge.b, 55));
        appendOutlinedRect(m_shapeVertices, c.x, c.y, c.w, c.h, 2.f, fill, edge);
    }
    if (m_shapeVertices.getVertexCount() > 0) window.draw(m_shapeVertices);

    // Draw cell values & indices using pooled text
    char buf[32];
    for (int i = 0; i < layout.cellCount; ++i) {
        const ArrayCellLayout& c = layout.cells[i];
        int len = std::snprintf(buf, sizeof(buf), "%d", c.value);
        if (len > 0) {
            Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                               14, false, TEXT_ALIGN_CENTER_XY,
                               c.isSorted ? sf::Color(180, 255, 200) : sf::Color::White,
                               c.x + c.w * 0.5f, c.y + c.h * 0.5f);
        }

        if (!c.isSorted) {
            len = std::snprintf(buf, sizeof(buf), "%d", c.index);
            if (len > 0) {
                Ui::drawPooledText(window, &m_ui, font, std::string_view(buf, static_cast<size_t>(len)),
                                   9, false, TEXT_ALIGN_TOP_LEFT, sf::Color(130, 150, 180),
                                   c.x + 3.f, c.y + 1.f);
            }
        }
    }
}

void Renderer::drawEventLogPanel(sf::RenderWindow& window, const sf::Font& font,
                                 float x, float y, float w, float h,
                                 const EventBatch& batch, int cursor,
                                 int totalBatchesLogged) const {
    int total = batch.getCount();
    const int maxRows = static_cast<int>((h - 144.f) / 24.f);
    int visibleRows = (maxRows < 1) ? 1 : maxRows;
    int startIdx = 0;
    if (total > visibleRows && cursor > visibleRows / 2) {
        startIdx = cursor - visibleRows / 2;
        if (startIdx + visibleRows > total) startIdx = total - visibleRows;
        if (startIdx < 0) startIdx = 0;
    }

    m_shapeVertices.clear();
    m_shapeVertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    appendOutlinedRect(m_shapeVertices, x - 2.f, y - 2.f, w + 4.f, h + 4.f, 3.f,
                       sf::Color(0, 0, 0, 0), sf::Color(0, 255, 65, 55));
    appendOutlinedRect(m_shapeVertices, x, y, w, h, 1.5f, sf::Color(5, 13, 8, 235), sf::Color(0, 255, 65, 210));
    appendQuad(m_shapeVertices, x + 10.f, y + 28.f, w - 20.f, 1.f, sf::Color(0, 255, 65, 90));

    if (total > 0 && cursor >= startIdx && cursor < startIdx + visibleRows && cursor < total) {
        float rowY = y + 34.f + static_cast<float>(cursor - startIdx) * 24.f;
        appendOutlinedRect(m_shapeVertices, x + 8.f, rowY, w - 16.f, 22.f, 1.f,
                           sf::Color(12, 48, 22), sf::Color(0, 255, 65));
    }
    window.draw(m_shapeVertices);

    if (m_cachedHdrTotal != total || m_cachedHdrCursor != cursor || m_cachedHdrBatches != totalBatchesLogged) {
        m_cachedHdrTotal = total;
        m_cachedHdrCursor = cursor;
        m_cachedHdrBatches = totalBatchesLogged;
        m_cachedLogHeader = "EVENT LOG  [" +
            std::to_string(total > 0 && cursor >= 0 ? (cursor + 1) : 0) +
            "/" + std::to_string(total) + "]  Ops:" + std::to_string(totalBatchesLogged);
    }

    Ui::drawPooledText(window, &m_ui, font, m_cachedLogHeader, 12, true, TEXT_ALIGN_TOP_LEFT,
                       sf::Color(0, 255, 65), x + 10.f, y + 8.f);

    if (total == 0) {
        Ui::textWrap(window, font, "No operation events yet. Trigger an action or click a node on the canvas.", 12,
                     sf::Color(110, 185, 125), x + 12.f, y + 40.f, w - 24.f, false, &m_ui);
        return;
    }

    int firstType = (total > 0) ? batch.getEventAt(0).type : -1;
    int firstKey  = (total > 0) ? (batch.getEventAt(0).nodeId ^ (batch.getEventAt(0).newValue << 8)) : -999999;
    int lastType  = (total > 0) ? batch.getEventAt(total - 1).type : -1;
    int lastKey   = (total > 0) ? (batch.getEventAt(total - 1).nodeId ^ (batch.getEventAt(total - 1).newValue << 8)) : -999999;
    bool batchChanged = (m_cachedLogTotal != total || m_cachedLogBatches != totalBatchesLogged ||
                         m_cachedLogFirstType != firstType || m_cachedLogFirstKey != firstKey ||
                         m_cachedLogLastType != lastType || m_cachedLogLastKey != lastKey ||
                         std::abs(m_cachedLogW - w) > 0.5f);
    if (batchChanged) {
        m_cachedLogTotal = total;
        m_cachedLogBatches = totalBatchesLogged;
        m_cachedLogFirstType = firstType;
        m_cachedLogFirstKey = firstKey;
        m_cachedLogLastType = lastType;
        m_cachedLogLastKey = lastKey;
        m_cachedLogW = w;
        int lim = (total < MAX_LOG_LINE_CACHE) ? total : MAX_LOG_LINE_CACHE;
        for (int i = 0; i < lim; ++i) {
            m_cachedLogLines[i] = Ui::fit(font, AnimationController::formatEvent(batch.getEventAt(i)), 12, w - 28.f);
        }
    }

    for (int r = 0; r < visibleRows && (startIdx + r) < total; ++r) {
        int idx = startIdx + r;
        float rowY = y + 34.f + static_cast<float>(r) * 24.f;
        sf::Color rowCol = (idx == cursor) ? sf::Color(0, 255, 65)
                         : (idx < cursor)  ? sf::Color(195, 240, 205)
                                           : sf::Color(90, 145, 100);
        if (idx < MAX_LOG_LINE_CACHE) {
            Ui::drawPooledText(window, &m_ui, font, m_cachedLogLines[idx], 12, false, TEXT_ALIGN_TOP_LEFT,
                               rowCol, x + 12.f, rowY + 3.f);
        } else {
            const std::string& line = Ui::fitCached(m_ui, font, AnimationController::formatEvent(batch.getEventAt(idx)), 12, w - 28.f);
            Ui::drawPooledText(window, &m_ui, font, line, 12, false, TEXT_ALIGN_TOP_LEFT,
                               rowCol, x + 12.f, rowY + 3.f);
        }
    }
}

void Renderer::drawPlaybackControls(sf::RenderWindow& window, const sf::Font& font,
                                    float panelX, float panelY, float panelW, float panelH,
                                    bool isPlaying, float speedMultiplier,
                                    int cursor, int totalEvents,
                                    const UiInput& in) const {
    float row0Y = panelY + panelH - 96.f;
    float row1Y = panelY + panelH - 64.f;
    float row2Y = panelY + panelH - 30.f;

    Ui::button(window, font, "Replay", panelX + 8.f, row0Y, 72.f, 24.f, BTN_SECONDARY, in, 12, &m_ui);
    float scrubX = panelX + 90.f;
    float scrubW = panelW - 104.f;
    if (scrubW < 40.f) scrubW = 40.f;
    float scrubY = row0Y + 10.f;

    float scrubNorm = 0.f;
    if (totalEvents > 1 && cursor >= 0) {
        scrubNorm = static_cast<float>(cursor) / static_cast<float>(totalEvents - 1);
        if (scrubNorm < 0.f) scrubNorm = 0.f;
        if (scrubNorm > 1.f) scrubNorm = 1.f;
    } else if (totalEvents == 1 && cursor >= 0) {
        scrubNorm = 1.0f;
    }

    float btnW = (panelW - 32.f) / 3.f;
    if (btnW > 116.f) btnW = 116.f;
    Ui::button(window, font, isPlaying ? "Pause [Spc]" : "Play [Spc]", panelX + 8.f, row1Y, btnW, 25.f,
               isPlaying ? BTN_PRIMARY : BTN_SECONDARY, in, 12, &m_ui);
    Ui::button(window, font, "Step [>]", panelX + 16.f + btnW, row1Y, btnW, 25.f, BTN_SECONDARY, in, 12, &m_ui);
    Ui::button(window, font, "Skip [>>]", panelX + 24.f + 2.f * btnW, row1Y, btnW, 25.f, BTN_GHOST, in, 12, &m_ui);

    int tenths = static_cast<int>(speedMultiplier * 10.f + 0.5f);
    if (m_cachedSpeedTenths != tenths) {
        m_cachedSpeedTenths = tenths;
        m_cachedSpeedStr = "Speed " + std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + "x";
    }
    Ui::drawPooledText(window, &m_ui, font, m_cachedSpeedStr, 12, false, TEXT_ALIGN_TOP_LEFT,
                       sf::Color(140, 230, 155), panelX + 10.f, row2Y + 2.f);

    float trackX = panelX + 94.f;
    float trackW = panelW - 108.f;
    if (trackW > 240.f) trackW = 240.f;
    float trackY = row2Y + 10.f;

    float norm = (speedMultiplier - 0.25f) / (3.0f - 0.25f);
    if (norm < 0.f) norm = 0.f;
    if (norm > 1.f) norm = 1.f;

    float cosT[CIRCLE_SEGS + 1], sinT[CIRCLE_SEGS + 1];
    buildUnitCircle(cosT, sinT);

    m_shapeVertices.clear();
    m_shapeVertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    appendQuad(m_shapeVertices, scrubX, scrubY, scrubW, 5.f, sf::Color(40, 55, 80));
    if (scrubNorm > 0.f) appendQuad(m_shapeVertices, scrubX, scrubY, scrubW * scrubNorm, 5.f, sf::Color(168, 85, 247));
    appendDisc(m_shapeVertices, scrubX + scrubW * scrubNorm, scrubY + 2.5f, 6.f, sf::Color(245, 185, 40), cosT, sinT);
    appendRing(m_shapeVertices, scrubX + scrubW * scrubNorm, scrubY + 2.5f, 6.f, 7.4f, sf::Color::White, cosT, sinT);

    appendQuad(m_shapeVertices, trackX, trackY, trackW, 5.f, sf::Color(40, 55, 80));
    if (norm > 0.f) appendQuad(m_shapeVertices, trackX, trackY, trackW * norm, 5.f, sf::Color(56, 189, 248));
    appendDisc(m_shapeVertices, trackX + trackW * norm, trackY + 2.5f, 7.f, sf::Color(245, 185, 40), cosT, sinT);
    appendRing(m_shapeVertices, trackX + trackW * norm, trackY + 2.5f, 7.f, 8.5f, sf::Color::White, cosT, sinT);

    window.draw(m_shapeVertices);
}

void Renderer::drawNodeTooltip(sf::RenderWindow& window, const sf::Font& font,
                               const LayoutResult& layout, int hoveredNodeId,
                               float windowW, float windowH,
                               float screenNodeX, float screenNodeY) const {
    if (hoveredNodeId < 0) return;
    int idx = layout.findNodeIndexById(hoveredNodeId);
    if (idx < 0) return;
    const NodeLayout& n = layout.nodes[idx];

    if (m_cachedTipId != n.id || m_cachedTipKey != n.key || m_cachedTipType != layout.structureType ||
        m_cachedTipH != n.height || m_cachedTipBf != n.bf || m_cachedTipSz != n.size ||
        m_cachedTipD != n.depth || m_cachedTipViol != n.isViolation) {
        m_cachedTipId = n.id;
        m_cachedTipKey = n.key;
        m_cachedTipType = layout.structureType;
        m_cachedTipH = n.height;
        m_cachedTipBf = n.bf;
        m_cachedTipSz = n.size;
        m_cachedTipD = n.depth;
        m_cachedTipViol = n.isViolation;
        m_cachedTipLine1 = (layout.structureType == STRUCT_MAX_HEAP)
            ? ("Val: " + std::to_string(n.key) + "   Idx: [" + std::to_string(n.id) + "]")
            : ("Key: " + std::to_string(n.key) + "   (#" + std::to_string(n.id) + ")");
        m_cachedTipLine2 = (layout.structureType == STRUCT_MAX_HEAP)
            ? ("Depth: " + std::to_string(n.depth) + (n.isViolation ? "  [VIOLATION]" : "  [VALID]"))
            : ("h=" + std::to_string(n.height) + "  BF=" + std::to_string(n.bf) +
               "  sz=" + std::to_string(n.size) + "  d=" + std::to_string(n.depth));
    }

    float anchorX = (screenNodeX > -90000.f) ? screenNodeX : n.x;
    float anchorY = (screenNodeY > -90000.f) ? screenNodeY : n.y;
    float tw = 196.f;
    float th = 46.f;
    float tx = anchorX + n.radius + 10.f;
    float ty = anchorY - th - 6.f;
    if (tx + tw > windowW - 10.f) tx = anchorX - tw - n.radius - 10.f;
    if (tx < 10.f) tx = 10.f;
    if (ty < 10.f) ty = anchorY + n.radius + 8.f;
    if (ty + th > windowH - 10.f) ty = windowH - th - 10.f;

    m_shapeVertices.clear();
    m_shapeVertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    appendOutlinedRect(m_shapeVertices, tx - 2.f, ty - 2.f, tw + 4.f, th + 4.f, 2.5f,
                       sf::Color(0, 0, 0, 0), sf::Color(0, 255, 65, 55));
    appendOutlinedRect(m_shapeVertices, tx, ty, tw, th, 1.5f, sf::Color(5, 13, 8, 230), sf::Color(0, 255, 65));
    window.draw(m_shapeVertices);

    Ui::drawPooledText(window, &m_ui, font, m_cachedTipLine1, 11, false, TEXT_ALIGN_TOP_LEFT,
                       sf::Color(0, 255, 65), tx + 8.f, ty + 5.f);
    Ui::drawPooledText(window, &m_ui, font, m_cachedTipLine2, 10, false, TEXT_ALIGN_TOP_LEFT,
                       sf::Color::White, tx + 8.f, ty + 23.f);
}

} // namespace VaultHeist
