#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

namespace VaultHeist {

// Toast lifecycle states (no enum allowed)
static const int TOAST_SLIDE_IN = 0;
static const int TOAST_HOLD     = 1;
static const int TOAST_FADE_OUT = 2;

// Toast kinds
static const int TOAST_KIND_SUCCESS = 0;  // green
static const int TOAST_KIND_PENALTY = 1;  // red
static const int TOAST_KIND_WARNING = 2;  // yellow

static const int MAX_TOASTS = 4;

struct ToastItem {
    std::string message;
    mutable std::string fittedMsg;
    mutable std::optional<sf::Text> text;
    mutable const sf::Font* fontPtr;
    mutable bool fitted;
    int kind;
    int state;
    float elapsed;

    ToastItem() : fontPtr(nullptr), fitted(false), kind(TOAST_KIND_SUCCESS), state(TOAST_SLIDE_IN), elapsed(0.f) {}
};

/**
 * @brief Manual fixed-size queue of active toasts (no std::vector).
 * Each toast slides down from the top, holds ~1s, fades out, then is removed.
 */
class ToastQueue {
private:
    ToastItem items[MAX_TOASTS];
    int count;
    mutable sf::RectangleShape m_box;
    mutable sf::RectangleShape m_bar;

public:
    ToastQueue() : count(0) {}

    void clear() { count = 0; }
    int getCount() const { return count; }
    void push(const std::string& message, int kind);
    void update(float dt);
    void draw(sf::RenderTarget& target, const sf::Font& font, float windowW) const;
};

} // namespace VaultHeist
