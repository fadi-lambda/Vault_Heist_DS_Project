#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace VaultHeist {

// Logical (design) resolution. Every screen is laid out against these.
static const unsigned int SCREEN_WIDTH  = 1600;
static const unsigned int SCREEN_HEIGHT = 900;

// Button styles (no enum allowed)
static const int BTN_PRIMARY   = 0;
static const int BTN_SECONDARY = 1;
static const int BTN_DANGER    = 2;
static const int BTN_GHOST     = 3;
static const int BTN_DISABLED  = 4;

// Text alignment modes for pooled sf::Text cache (no enum allowed)
static const int TEXT_ALIGN_TOP_LEFT  = 0;
static const int TEXT_ALIGN_CENTER_X  = 1;
static const int TEXT_ALIGN_CENTER_XY = 2;
static const int TEXT_ALIGN_BOTTOM_X  = 3;

// Text roles - one consistent typographic hierarchy for every screen
static const unsigned int TXT_TITLE = 46;
static const unsigned int TXT_H1    = 28;
static const unsigned int TXT_H2    = 20;
static const unsigned int TXT_BODY  = 15;
static const unsigned int TXT_HINT  = 13;

struct UiInput {
    float mx;
    float my;
    bool down;
    UiInput() : mx(0.f), my(0.f), down(false) {}
};

// Fixed-size persistent sf::Text slot & string-fit cache (no globals, no std::vector).
struct UiTextSlot {
    std::string key;
    unsigned int size;
    bool bold;
    int align;
    float origX;
    float origY;
    float lastX;
    float lastY;
    sf::Color lastColor;
    std::uint64_t lastFrame;
    std::optional<sf::Text> text;
    bool used;

    UiTextSlot()
        : size(0), bold(false), align(TEXT_ALIGN_TOP_LEFT), origX(0.f), origY(0.f),
          lastX(-999999.f), lastY(-999999.f), lastColor(0, 0, 0, 0),
          lastFrame(0), used(false) {}
};

struct UiCache {
    struct StrCacheEntry {
        std::string src;
        unsigned int size;
        float maxW;
        bool bold;
        bool isWrap;
        std::string out;
        bool valid;
        StrCacheEntry() : size(0), maxW(0.f), bold(false), isWrap(false), valid(false) {}
    };

    static const int STR_CACHE_CAP = 64;
    static const int TEXT_BUCKETS  = 64;
    static const int TEXT_WAYS     = 8;
    static const int TEXT_POOL_CAP = TEXT_BUCKETS * TEXT_WAYS;

    const sf::Font* fontPtr;
    sf::RectangleShape rect;
    sf::ConvexShape star;
    sf::VertexArray gridVertices;
    bool gridBuilt;
    float gridW;
    float gridH;
    StrCacheEntry strCache[STR_CACHE_CAP];
    int strNext;
    UiTextSlot* textPool;
    std::uint64_t frameTick;

    UiCache()
        : fontPtr(nullptr), star(10), gridVertices(sf::PrimitiveType::Lines),
          gridBuilt(false), gridW(0.f), gridH(0.f), strNext(0), frameTick(1) {
        textPool = new UiTextSlot[TEXT_POOL_CAP];
    }

    ~UiCache() {
        delete[] textPool;
        textPool = nullptr;
    }

    UiCache(const UiCache&) = delete;
    UiCache& operator=(const UiCache&) = delete;
};

namespace Ui {

// Palette (dark navy / gold "Vault Heist" theme) - functions, not global variables
inline sf::Color gold()      { return sf::Color(245, 185, 40); }
inline sf::Color cyan()      { return sf::Color(56, 189, 248); }
inline sf::Color bgNavy()    { return sf::Color(10, 16, 28); }
inline sf::Color panelFill() { return sf::Color(16, 24, 42); }
inline sf::Color textBody()  { return sf::Color(214, 226, 244); }
inline sf::Color textMuted() { return sf::Color(128, 148, 178); }
inline sf::Color green()     { return sf::Color(70, 220, 130); }
inline sf::Color red()       { return sf::Color(240, 90, 90); }
inline sf::Color amber()     { return sf::Color(250, 200, 70); }

bool inside(float mx, float my, float x, float y, float w, float h);

float textWidth(const sf::Font& font, std::string_view s, unsigned int size, bool bold = false);
// Truncates with "..." so the text never exceeds maxW pixels.
std::string fit(const sf::Font& font, const std::string& s, unsigned int size, float maxW, bool bold = false);
const std::string& fitCached(UiCache& cache, const sf::Font& font, const std::string& s, unsigned int size, float maxW, bool bold = false);
// Word-wraps (honours '\n') so every line fits in maxW pixels.
std::string wrap(const sf::Font& font, const std::string& s, unsigned int size, float maxW, bool bold = false);
const std::string& wrapCached(UiCache& cache, const sf::Font& font, const std::string& s, unsigned int size, float maxW, bool bold = false);
int lineCount(std::string_view s);

void drawPooledText(sf::RenderTarget& t, UiCache* cache, const sf::Font& font,
                    std::string_view s, unsigned int size, bool bold, int align,
                    sf::Color color, float x, float y);

void text(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
          sf::Color color, float x, float y, bool bold = false, UiCache* cache = nullptr);
// Single line, truncated to maxW.
void textFit(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
             sf::Color color, float x, float y, float maxW, bool bold = false, UiCache* cache = nullptr);
void textCenter(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
                sf::Color color, float cx, float y, bool bold = false, UiCache* cache = nullptr);
// Wrapped paragraph; returns the height drawn.
float textWrap(sf::RenderTarget& t, const sf::Font& font, const std::string& s, unsigned int size,
               sf::Color color, float x, float y, float maxW, bool bold = false, UiCache* cache = nullptr);

void panel(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color outline,
           unsigned char alpha = 235, UiCache* cache = nullptr);

// Hover = slight scale-up + glow; press = slight scale-down. Returns true if hovered.
bool button(sf::RenderTarget& t, const sf::Font& font, const std::string& label,
            float x, float y, float w, float h, int style, const UiInput& in,
            unsigned int size = 15, UiCache* cache = nullptr);

void stars(sf::RenderTarget& t, float cx, float cy, int count, int maxCount, float size, UiCache* cache = nullptr);
void grid(sf::RenderTarget& t, float w, float h, UiCache* cache = nullptr);
// Standard screen header: H1 title + muted subtitle + cyan underline.
void screenHeader(sf::RenderTarget& t, const sf::Font& font, const std::string& title,
                  const std::string& subtitle, UiCache* cache = nullptr);

} // namespace Ui
} // namespace VaultHeist
