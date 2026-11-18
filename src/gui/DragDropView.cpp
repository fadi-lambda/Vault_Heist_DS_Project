#include "gui/DragDropView.h"
#include <string>

namespace VaultHeist {

namespace {

void drawRing(sf::RenderTarget& t, float x, float y, float r, sf::Color fill,
              sf::Color outline, float thickness) {
    sf::CircleShape c(r);
    c.setOrigin({r, r});
    c.setPosition({x, y});
    c.setFillColor(fill);
    c.setOutlineThickness(thickness);
    c.setOutlineColor(outline);
    t.draw(c);
}

void drawTokenBody(sf::RenderTarget& t, const sf::Font& font, const DragToken& tok,
                   float x, float y, float scale, bool lifted, UiCache* cache) {
    float r = tok.radius * scale;
    if (lifted) {
        drawRing(t, x + 5.f, y + 7.f, r, sf::Color(0, 0, 0, 90), sf::Color::Transparent, 0.f);
    }
    sf::Color fill = lifted ? sf::Color(250, 205, 80) : sf::Color(30, 48, 78);
    sf::Color outline = lifted ? Ui::gold() : Ui::cyan();
    drawRing(t, x, y, r, fill, outline, 2.5f);
    const std::string& text = tok.label.empty() ? std::to_string(tok.value) : tok.label;
    Ui::textCenter(t, font, text, 16,
                   lifted ? sf::Color(20, 20, 30) : sf::Color::White, x, y - 10.f, true, cache);
}

} // namespace

void DragDropView::draw(sf::RenderTarget& target, const sf::Font& font,
                        const DragDropController& dd, UiCache* cache) {
    const bool dragging = dd.isDragging();

    // Slots
    for (int i = 0; i < dd.slotCount(); ++i) {
        const DropSlot* s = dd.slot(i);
        if (!s || !s->enabled) continue;
        bool hovered = dragging && dd.hoveredSlotId() == i;
        sf::Color outline = sf::Color(90, 110, 145);
        sf::Color fill = sf::Color(16, 24, 42, 120);
        float thick = 2.f;
        if (dragging) outline = sf::Color(120, 150, 200);
        if (hovered) {
            bool ok = dd.hoveredSlotAccepts();
            outline = ok ? Ui::green() : Ui::red();
            fill = ok ? sf::Color(40, 120, 80, 110) : sf::Color(140, 50, 50, 110);
            thick = 3.5f;
        }
        drawRing(target, s->x, s->y, s->radius, fill, outline, thick);
        if (s->tokenId < 0 && !s->label.empty()) {
            Ui::textCenter(target, font, s->label, 12, sf::Color(150, 175, 210, 180),
                           s->x, s->y - 7.f, false, cache);
        }
    }

    // Resting tokens (snapped tokens sit at their slot centre)
    for (int i = 0; i < dd.tokenCount(); ++i) {
        const DragToken* t = dd.token(i);
        if (!t || (dragging && dd.draggedTokenId() == i)) continue;
        drawTokenBody(target, font, *t, t->x, t->y, 1.f, false, cache);
        if (!t->enabled) {
            drawRing(target, t->x, t->y, t->radius, sf::Color(0, 0, 0, 110), sf::Color::Transparent, 0.f);
        }
    }

    // Dragged token: guide line to origin, then lifted token
    if (dragging) {
        const DragToken* t = dd.token(dd.draggedTokenId());
        if (t) {
            sf::Vertex line[] = {
                sf::Vertex({t->x, t->y}, sf::Color(245, 185, 40, 120)),
                sf::Vertex({dd.dragX(), dd.dragY()}, sf::Color(245, 185, 40, 200))
            };
            target.draw(line, 2, sf::PrimitiveType::Lines);
            drawTokenBody(target, font, *t, dd.dragX(), dd.dragY(), 1.15f, true, cache);
        }
    }
}

} // namespace VaultHeist
