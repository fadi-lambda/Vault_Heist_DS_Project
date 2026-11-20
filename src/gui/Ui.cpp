#include "gui/Ui.h"
#include <cmath>
#include <cstdint>

namespace VaultHeist {
namespace Ui {

static sf::RectangleShape& acquireRect(UiCache* cache, sf::RectangleShape& fallback) {
    return cache ? cache->rect : fallback;
}

static void resetCacheForFont(UiCache& cache, const sf::Font& font) {
    if (cache.fontPtr == &font) return;
    cache.fontPtr = &font;
    for (int i = 0; i < UiCache::STR_CACHE_CAP; ++i) cache.strCache[i].valid = false;
    for (int i = 0; i < UiCache::TEXT_POOL_CAP; ++i) {
        cache.textPool[i].used = false;
        cache.textPool[i].text.reset();
    }
}

bool inside(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

float textWidth(const sf::Font& font, std::string_view s, unsigned int size, bool bold) {
    float curW = 0.f;
    float maxW = 0.f;
    char32_t prev = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char32_t ch = static_cast<char32_t>(static_cast<unsigned char>(s[i]));
        if (ch == U'\r') continue;
        if (ch == U'\n') {
            if (curW > maxW) maxW = curW;
            curW = 0.f;
            prev = 0;
            continue;
        }
        if (prev != 0) curW += font.getKerning(prev, ch, size, bold);
        curW += font.getGlyph(ch, size, bold).advance;
        prev = ch;
    }
    return (curW > maxW) ? curW : maxW;
}

std::string fit(const sf::Font& font, const std::string& s, unsigned int size, float maxW, bool bold) {
    if (textWidth(font, s, size, bold) <= maxW) return s;
    std::string r = s;
    float dotsW = textWidth(font, "...", size, bold);
    float limit = maxW - dotsW;
    if (limit <= 0.f) return "...";
    while (r.size() > 1 && textWidth(font, r, size, bold) > limit) r.pop_back();
    return r + "...";
}

const std::string& fitCached(UiCache& cache, const sf::Font& font, const std::string& s,
                             unsigned int size, float maxW, bool bold) {
    resetCacheForFont(cache, font);
    for (int i = 0; i < UiCache::STR_CACHE_CAP; ++i) {
        const auto& e = cache.strCache[i];
        if (e.valid && !e.isWrap && e.size == size && e.bold == bold &&
            std::abs(e.maxW - maxW) < 0.25f && e.src == s) {
            return e.out;
        }
    }
    auto& slot = cache.strCache[cache.strNext];
    cache.strNext = (cache.strNext + 1) % UiCache::STR_CACHE_CAP;
    slot.src = s;
    slot.size = size;
    slot.maxW = maxW;
    slot.bold = bold;
    slot.isWrap = false;
    slot.out = fit(font, s, size, maxW, bold);
    slot.valid = true;
    return slot.out;
}

std::string wrap(const sf::Font& font, const std::string& s, unsigned int size, float maxW, bool bold) {
    std::string out;
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t nl = s.find('\n', pos);
        std::string para = s.substr(pos, (nl == std::string::npos) ? std::string::npos : nl - pos);

        std::string line;
        size_t i = 0;
        while (i <= para.size()) {
            size_t sp = para.find(' ', i);
            std::string word = para.substr(i, (sp == std::string::npos) ? std::string::npos : sp - i);
            if (line.empty()) {
                line = word;
            } else if (textWidth(font, line + " " + word, size, bold) <= maxW) {
                line += " " + word;
            } else {
                out += line + "\n";
                line = word;
            }
            if (sp == std::string::npos) break;
            i = sp + 1;
        }
        out += line;
        if (nl == std::string::npos) break;
        out += "\n";
        pos = nl + 1;
    }
    return out;
}

const std::string& wrapCached(UiCache& cache, const sf::Font& font, const std::string& s,
                              unsigned int size, float maxW, bool bold) {
    resetCacheForFont(cache, font);
    for (int i = 0; i < UiCache::STR_CACHE_CAP; ++i) {
        const auto& e = cache.strCache[i];
        if (e.valid && e.isWrap && e.size == size && e.bold == bold &&
            std::abs(e.maxW - maxW) < 0.25f && e.src == s) {
            return e.out;
        }
    }
    auto& slot = cache.strCache[cache.strNext];
    cache.strNext = (cache.strNext + 1) % UiCache::STR_CACHE_CAP;
    slot.src = s;
    slot.size = size;
    slot.maxW = maxW;
    slot.bold = bold;
    slot.isWrap = true;
    slot.out = wrap(font, s, size, maxW, bold);
    slot.valid = true;
    return slot.out;
}

int lineCount(std::string_view s) {
    int n = 1;
    for (size_t i = 0; i < s.size(); ++i) if (s[i] == '\n') n++;
    return n;
}

static void applyAlignAndDraw(sf::RenderTarget& t, sf::Text& tx, int align,
                              float& outOrigX, float& outOrigY,
                              float& outPx, float& outPy,
                              sf::Color color, float x, float y, bool computeOrigin) {
    if (computeOrigin) {
        if (align == TEXT_ALIGN_TOP_LEFT) {
            outOrigX = 0.f;
            outOrigY = 0.f;
        } else {
            auto b = tx.getLocalBounds();
            if (align == TEXT_ALIGN_CENTER_X) {
                outOrigX = std::round(b.position.x + b.size.x * 0.5f);
                outOrigY = 0.f;
            } else if (align == TEXT_ALIGN_CENTER_XY) {
                outOrigX = std::round(b.position.x + b.size.x * 0.5f);
                outOrigY = std::round(b.position.y + b.size.y * 0.5f);
            } else { // TEXT_ALIGN_BOTTOM_X
                outOrigX = std::round(b.position.x + b.size.x * 0.5f);
                outOrigY = b.position.y;
            }
        }
        tx.setOrigin({outOrigX, outOrigY});
    }

    float px = (align == TEXT_ALIGN_CENTER_X || align == TEXT_ALIGN_CENTER_XY || align == TEXT_ALIGN_BOTTOM_X)
               ? std::round(x) : x;
    float py = (align == TEXT_ALIGN_CENTER_X || align == TEXT_ALIGN_CENTER_XY || align == TEXT_ALIGN_BOTTOM_X)
               ? std::round(y) : y;
    tx.setFillColor(color);
    tx.setPosition({px, py});
    outPx = px;
    outPy = py;
    t.draw(tx);
}

void drawPooledText(sf::RenderTarget& t, UiCache* cache, const sf::Font& font,
                    std::string_view s, unsigned int size, bool bold, int align,
                    sf::Color color, float x, float y) {
    if (!cache) {
        sf::Text tx(font, std::string(s), size);
        if (bold) tx.setStyle(sf::Text::Bold);
        float ox = 0.f, oy = 0.f, px = 0.f, py = 0.f;
        applyAlignAndDraw(t, tx, align, ox, oy, px, py, color, x, y, true);
        return;
    }

    resetCacheForFont(*cache, font);

    std::uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < s.size(); ++i) {
        h ^= static_cast<unsigned char>(s[i]);
        h *= 1099511628211ULL;
    }
    h ^= static_cast<std::uint64_t>(size) * 2654435761ULL;
    h ^= (bold ? 0x9e3779b97f4a7c15ULL : 0ULL);
    h ^= static_cast<std::uint64_t>(align) * 0x517cc1b727220a95ULL;
    h ^= (h >> 33);

    int bucket = static_cast<int>(h & static_cast<std::uint64_t>(UiCache::TEXT_BUCKETS - 1));
    int base = bucket * UiCache::TEXT_WAYS;

    int targetSlot = -1;
    std::uint64_t oldestTick = ~0ULL;
    int oldestSlot = base;

    for (int w = 0; w < UiCache::TEXT_WAYS; ++w) {
        UiTextSlot& slot = cache->textPool[base + w];
        if (slot.used) {
            if (slot.size == size && slot.bold == bold && slot.align == align && slot.key == s) {
                sf::Text& tx = *slot.text;
                slot.lastFrame = cache->frameTick;
                float px = (align == TEXT_ALIGN_TOP_LEFT) ? x : std::round(x);
                float py = (align == TEXT_ALIGN_TOP_LEFT) ? y : std::round(y);
                if (slot.lastColor != color) {
                    tx.setFillColor(color);
                    slot.lastColor = color;
                }
                if (slot.lastX != px || slot.lastY != py) {
                    tx.setPosition({px, py});
                    slot.lastX = px;
                    slot.lastY = py;
                }
                t.draw(tx);
                return;
            }
            if (slot.lastFrame < oldestTick) {
                oldestTick = slot.lastFrame;
                oldestSlot = base + w;
            }
        } else if (targetSlot < 0) {
            targetSlot = base + w;
        }
    }

    if (targetSlot < 0) targetSlot = oldestSlot;
    cache->frameTick++;

    UiTextSlot& slot = cache->textPool[targetSlot];
    slot.key.assign(s.data(), s.size());
    slot.size = size;
    slot.bold = bold;
    slot.align = align;
    slot.used = true;
    slot.lastFrame = cache->frameTick;

    if (!slot.text.has_value()) {
        slot.text.emplace(font, slot.key, size);
    } else {
        slot.text->setString(slot.key);
        slot.text->setCharacterSize(size);
    }
    slot.text->setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
    slot.lastColor = color;
    applyAlignAndDraw(t, *slot.text, align, slot.origX, slot.origY, slot.lastX, slot.lastY, color, x, y, true);
}

void text(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
          sf::Color color, float x, float y, bool bold, UiCache* cache) {
    drawPooledText(t, cache, font, s, size, bold, TEXT_ALIGN_TOP_LEFT, color, x, y);
}

void textFit(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
             sf::Color color, float x, float y, float maxW, bool bold, UiCache* cache) {
    if (cache) {
        const std::string& fitted = fitCached(*cache, font, s, size, maxW, bold);
        drawPooledText(t, cache, font, fitted, size, bold, TEXT_ALIGN_TOP_LEFT, color, x, y);
    } else {
        text(t, font, fit(font, s, size, maxW, bold), size, color, x, y, bold, nullptr);
    }
}

void textCenter(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
                sf::Color color, float cx, float y, bool bold, UiCache* cache) {
    drawPooledText(t, cache, font, s, size, bold, TEXT_ALIGN_CENTER_X, color, cx, y);
}

float textWrap(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
               sf::Color color, float x, float y, float maxW, bool bold, UiCache* cache) {
    if (cache) {
        const std::string& w = wrapCached(*cache, font, s, size, maxW, bold);
        drawPooledText(t, cache, font, w, size, bold, TEXT_ALIGN_TOP_LEFT, color, x, y);
        return static_cast<float>(lineCount(w)) * (static_cast<float>(size) * 1.25f);
    }
    std::string w = wrap(font, s, size, maxW, bold);
    text(t, font, w, size, color, x, y, bold, nullptr);
    return static_cast<float>(lineCount(w)) * (static_cast<float>(size) * 1.25f);
}

void panel(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color outline,
           unsigned char alpha, UiCache* cache) {
    sf::RectangleShape fb;
    sf::RectangleShape& r = acquireRect(cache, fb);
    bool isDanger = (outline.r > 200 && outline.g < 110 && outline.b < 110);
    sf::Color glowEdge = isDanger ? sf::Color(240, 80, 80, 55) : sf::Color(0, 255, 65, 55);
    sf::Color mainEdge = isDanger ? outline : sf::Color(0, 255, 65, 210);

    // Layered soft glow behind panel
    r.setSize({w + 4.f, h + 4.f});
    r.setOrigin({0.f, 0.f});
    r.setPosition({x - 2.f, y - 2.f});
    r.setFillColor(sf::Color::Transparent);
    r.setOutlineThickness(3.f);
    r.setOutlineColor(glowEdge);
    t.draw(r);

    // Main panel
    r.setSize({w, h});
    r.setOrigin({0.f, 0.f});
    r.setPosition({x, y});
    r.setFillColor(sf::Color(5, 13, 8, alpha));
    r.setOutlineThickness(1.5f);
    r.setOutlineColor(mainEdge);
    t.draw(r);
}

bool button(sf::RenderTarget& t, const sf::Font& font, const std::string& label,
            float x, float y, float w, float h, int style, const UiInput& in,
            unsigned int size, UiCache* cache) {
    bool disabled = (style == BTN_DISABLED);
    bool hover = !disabled && inside(in.mx, in.my, x, y, w, h);
    bool pressed = hover && in.down;

    sf::Color fill, edge, txt = sf::Color::White;
    switch (style) {
        case BTN_PRIMARY:   fill = sf::Color(10, 44, 18); edge = sf::Color(0, 255, 65);   break;
        case BTN_SECONDARY: fill = sf::Color(6, 22, 11);  edge = sf::Color(0, 230, 60);   break;
        case BTN_DANGER:    fill = sf::Color(55, 18, 22); edge = sf::Color(235, 70, 70);  break;
        case BTN_GHOST:     fill = sf::Color(5, 14, 8);   edge = sf::Color(50, 140, 70);  break;
        default:            fill = sf::Color(6, 12, 8);   edge = sf::Color(35, 72, 42);
                            txt = sf::Color(74, 122, 74);                                 break;
    }

    float s = pressed ? 0.97f : (hover ? 1.03f : 1.0f);
    float cw = w * s, ch = h * s;
    float cx = x + w * 0.5f, cy = y + h * 0.5f;

    sf::RectangleShape fb;
    sf::RectangleShape& r = acquireRect(cache, fb);

    if (!disabled && (hover || style == BTN_PRIMARY)) {
        r.setSize({cw + 4.f, ch + 4.f});
        r.setOrigin({(cw + 4.f) * 0.5f, (ch + 4.f) * 0.5f});
        r.setPosition({cx, cy});
        r.setFillColor(sf::Color::Transparent);
        r.setOutlineThickness(hover ? 4.f : 2.5f);
        r.setOutlineColor(sf::Color(edge.r, edge.g, edge.b, hover ? 80 : 50));
        t.draw(r);
    }

    r.setSize({cw, ch});
    r.setOrigin({cw * 0.5f, ch * 0.5f});
    r.setPosition({cx, cy});
    int add = hover ? 18 : 0;
    r.setFillColor(sf::Color(static_cast<std::uint8_t>(fill.r + add), static_cast<std::uint8_t>(fill.g + add),
                             static_cast<std::uint8_t>(fill.b + add)));
    r.setOutlineThickness(hover ? 2.f : 1.5f);
    r.setOutlineColor(hover ? sf::Color(0, 255, 65) : edge);
    t.draw(r);

    if (cache) {
        const std::string& lbl = fitCached(*cache, font, label, size, w - 16.f, false);
        drawPooledText(t, cache, font, lbl, size, false, TEXT_ALIGN_CENTER_XY, txt, cx, cy);
    } else {
        drawPooledText(t, nullptr, font, fit(font, label, size, w - 16.f, false), size, false, TEXT_ALIGN_CENTER_XY, txt, cx, cy);
    }
    return hover;
}

void stars(sf::RenderTarget& t, float cx, float cy, int count, int maxCount, float size, UiCache* cache) {
    float gap = size * 2.4f;
    float startX = cx - gap * static_cast<float>(maxCount - 1) * 0.5f;
    sf::ConvexShape fbStar(10);
    sf::ConvexShape& star = cache ? cache->star : fbStar;
    for (int s = 0; s < maxCount; ++s) {
        float px = startX + gap * static_cast<float>(s);
        for (int i = 0; i < 10; ++i) {
            float ang = (-90.f + 36.f * static_cast<float>(i)) * 3.14159265f / 180.f;
            float r = (i % 2 == 0) ? size : size * 0.45f;
            star.setPoint(static_cast<std::size_t>(i), {px + std::cos(ang) * r, cy + std::sin(ang) * r});
        }
        bool on = s < count;
        star.setFillColor(on ? gold() : sf::Color(30, 40, 60));
        star.setOutlineThickness(1.5f);
        star.setOutlineColor(on ? sf::Color(255, 230, 140) : sf::Color(60, 75, 100));
        t.draw(star);
    }
}

void grid(sf::RenderTarget& t, float w, float h, UiCache* cache) {
    sf::VertexArray fbGrid(sf::PrimitiveType::Lines);
    sf::VertexArray& va = cache ? cache->gridVertices : fbGrid;
    if (!cache || !cache->gridBuilt || std::abs(cache->gridW - w) > 0.5f || std::abs(cache->gridH - h) > 0.5f) {
        va.clear();
        va.setPrimitiveType(sf::PrimitiveType::Lines);
        sf::Color c(20, 32, 54, 120);
        for (float x = 0.f; x <= w; x += 40.f) {
            va.append(sf::Vertex({x, 0.f}, c));
            va.append(sf::Vertex({x, h}, c));
        }
        for (float y = 0.f; y <= h; y += 40.f) {
            va.append(sf::Vertex({0.f, y}, c));
            va.append(sf::Vertex({w, y}, c));
        }
        if (cache) {
            cache->gridBuilt = true;
            cache->gridW = w;
            cache->gridH = h;
        }
    }
    t.draw(va);
}

void screenHeader(sf::RenderTarget& t, const sf::Font& font, const std::string& title,
                  const std::string& subtitle, UiCache* cache) {
    text(t, font, title, TXT_H1, gold(), 40.f, 24.f, true, cache);
    if (!subtitle.empty()) textFit(t, font, subtitle, TXT_HINT, textMuted(), 40.f, 62.f, 1000.f, false, cache);
    sf::RectangleShape fb;
    sf::RectangleShape& u = acquireRect(cache, fb);
    u.setSize({120.f, 3.f});
    u.setOrigin({0.f, 0.f});
    u.setPosition({40.f, 88.f});
    u.setOutlineThickness(0.f);
    u.setFillColor(cyan());
    t.draw(u);
}

} // namespace Ui
} // namespace VaultHeist
