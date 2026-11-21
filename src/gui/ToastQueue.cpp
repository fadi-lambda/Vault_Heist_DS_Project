#include "gui/ToastQueue.h"
#include "gui/Ui.h"
#include <cmath>
#include <cstdint>

namespace VaultHeist {

static const float TOAST_SLIDE_DUR = 0.22f;
static const float TOAST_HOLD_DUR  = 1.0f;
static const float TOAST_FADE_DUR  = 0.40f;

void ToastQueue::push(const std::string& message, int kind) {
    if (count >= MAX_TOASTS) {
        for (int i = 1; i < MAX_TOASTS; ++i) items[i - 1] = items[i];
        count = MAX_TOASTS - 1;
    }
    ToastItem t;
    t.message = message;
    t.kind = kind;
    items[count++] = t;
}

void ToastQueue::update(float dt) {
    int write = 0;
    for (int i = 0; i < count; ++i) {
        ToastItem t = items[i];
        t.elapsed += dt;
        bool expired = false;

        if (t.state == TOAST_SLIDE_IN) {
            if (t.elapsed >= TOAST_SLIDE_DUR) { t.state = TOAST_HOLD; t.elapsed = 0.f; }
        } else if (t.state == TOAST_HOLD) {
            if (t.elapsed >= TOAST_HOLD_DUR) { t.state = TOAST_FADE_OUT; t.elapsed = 0.f; }
        } else {
            if (t.elapsed >= TOAST_FADE_DUR) expired = true;
        }
        if (!expired) items[write++] = t;
    }
    count = write;
}

void ToastQueue::draw(sf::RenderTarget& target, const sf::Font& font, float windowW) const {
    if (count <= 0) return;

    for (int i = 0; i < count; ++i) {
        const ToastItem& t = items[i];
        float restY = 14.f + static_cast<float>(i) * 50.f;
        float y = restY;
        float a = 1.f;

        if (t.state == TOAST_SLIDE_IN) {
            float p = t.elapsed / TOAST_SLIDE_DUR;
            if (p > 1.f) p = 1.f;
            y = -50.f + (restY + 50.f) * p;
            a = p;
        } else if (t.state == TOAST_FADE_OUT) {
            a = 1.f - t.elapsed / TOAST_FADE_DUR;
            if (a < 0.f) a = 0.f;
        }

        sf::Color fill, edge;
        if (t.kind == TOAST_KIND_SUCCESS)      { fill = sf::Color(16, 64, 40);  edge = Ui::green(); }
        else if (t.kind == TOAST_KIND_PENALTY) { fill = sf::Color(80, 22, 28);  edge = Ui::red(); }
        else if (t.kind == TOAST_KIND_INFO)    { fill = sf::Color(10, 40, 24);  edge = sf::Color(0, 200, 90); }
        else                                   { fill = sf::Color(78, 60, 14);  edge = Ui::amber(); }

        std::uint8_t fa = static_cast<std::uint8_t>(a * 248.f);
        std::uint8_t ta = static_cast<std::uint8_t>(a * 255.f);

        float bw = 600.f, bh = 42.f;
        float bx = (windowW - bw) * 0.5f;

        m_box.setSize({bw, bh});
        m_box.setPosition({bx, y});
        m_box.setFillColor(sf::Color(fill.r, fill.g, fill.b, fa));
        m_box.setOutlineThickness(2.f);
        m_box.setOutlineColor(sf::Color(edge.r, edge.g, edge.b, ta));
        target.draw(m_box);

        m_bar.setSize({6.f, bh});
        m_bar.setPosition({bx, y});
        m_bar.setFillColor(sf::Color(edge.r, edge.g, edge.b, ta));
        target.draw(m_bar);

        if (!t.fitted || !t.text.has_value() || t.fontPtr != &font) {
            t.fittedMsg = Ui::fit(font, t.message, 16, bw - 40.f, true);
            t.fitted = true;
            t.fontPtr = &font;
            t.text.emplace(font, t.fittedMsg, 16);
            t.text->setStyle(sf::Text::Bold);
            auto b = t.text->getLocalBounds();
            t.text->setOrigin({std::round(b.position.x + b.size.x / 2.f), std::round(b.position.y + b.size.y / 2.f)});
        }
        sf::Text& txt = *t.text;
        txt.setFillColor(sf::Color(255, 255, 255, ta));
        txt.setPosition({std::round(bx + bw * 0.5f + 3.f), std::round(y + bh * 0.5f)});
        target.draw(txt);
    }
}

} // namespace VaultHeist
