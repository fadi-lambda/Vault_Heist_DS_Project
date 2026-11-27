#include "Renderer.h"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace EscapeNetwork {

namespace {
std::string formatNum(double val) {
    std::ostringstream oss;
    if (std::abs(val - std::round(val)) < 1e-6) {
        oss << static_cast<long long>(std::round(val));
    } else {
        oss << std::fixed << std::setprecision(1) << val;
    }
    return oss.str();
}
} // namespace

sf::Color Renderer::vertexFillColor(int state) {
    switch (state) {
        case VS_CURRENT:  return sf::Color(115, 86, 20);
        case VS_FRONTIER: return sf::Color(28, 66, 120);
        case VS_VISITED:  return sf::Color(22, 84, 92);
        case VS_ACCEPTED: return sf::Color(24, 92, 54);
        case VS_REJECTED: return sf::Color(110, 32, 36);
        case VS_UNVISITED:
        default:          return sf::Color(44, 52, 66);
    }
}

sf::Color Renderer::vertexOutlineColor(int state) {
    switch (state) {
        case VS_CURRENT:  return sf::Color(250, 205, 60);
        case VS_FRONTIER: return sf::Color(80, 175, 250);
        case VS_VISITED:  return sf::Color(70, 195, 200);
        case VS_ACCEPTED: return sf::Color(75, 225, 130);
        case VS_REJECTED: return sf::Color(245, 85, 85);
        case VS_UNVISITED:
        default:          return sf::Color(135, 145, 165);
    }
}

sf::Color Renderer::edgeColor(int state) {
    switch (state) {
        case VS_CURRENT:  return sf::Color(250, 205, 60);
        case VS_FRONTIER: return sf::Color(80, 175, 250);
        case VS_VISITED:  return sf::Color(70, 180, 190);
        case VS_ACCEPTED: return sf::Color(75, 225, 130);
        case VS_REJECTED: return sf::Color(245, 85, 85);
        case VS_UNVISITED:
        default:          return sf::Color(110, 120, 140);
    }
}

std::string Renderer::stateTag(int state) {
    switch (state) {
        case VS_CURRENT:  return "*";
        case VS_FRONTIER: return "?";
        case VS_VISITED:  return "~";
        case VS_ACCEPTED: return "OK";
        case VS_REJECTED: return "X";
        case VS_UNVISITED:
        default:          return "";
    }
}

void Renderer::drawGraph(sf::RenderWindow& window,
                         const sf::Font& font,
                         const Graph& graph,
                         const GraphVisuals& visuals,
                         float canvasX,
                         float canvasY,
                         int hoveredVertexId) const {
    const int vCount = graph.getVertexCount();
    const int eCount = graph.getEdgeCount();
    const float nodeRadius = 24.f;

    // 1. Draw Edges
    for (int i = 0; i < eCount; ++i) {
        const Edge* ePtr = graph.getEdge(i);
        if (!ePtr) continue;
        const Edge& e = *ePtr;
        const Vertex* u = graph.getVertex(e.from);
        const Vertex* v = graph.getVertex(e.to);
        if (!u || !v) continue;

        int st = (visuals.edgeStates && i < visuals.edgeCount) ? visuals.edgeStates[i] : VS_UNVISITED;
        sf::Color col = edgeColor(st);

        float x1 = canvasX + u->x;
        float y1 = canvasY + u->y;
        float x2 = canvasX + v->x;
        float y2 = canvasY + v->y;

        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-3f) continue;

        float ux = dx / len;
        float uy = dy / len;

        float thick = (st == VS_ACCEPTED || st == VS_CURRENT) ? 4.f : 2.2f;
        sf::RectangleShape lineRect({len, thick});
        lineRect.setOrigin({0.f, thick * 0.5f});
        lineRect.setPosition({x1, y1});
        lineRect.setRotation(sf::radians(std::atan2(dy, dx)));
        lineRect.setFillColor(col);
        window.draw(lineRect);

        // Directed arrowhead at target perimeter
        if (e.directed) {
            float tipX = x2 - ux * (nodeRadius + 2.f);
            float tipY = y2 - uy * (nodeRadius + 2.f);
            float backX = tipX - ux * 12.f;
            float backY = tipY - uy * 12.f;
            float px = -uy * 6.f;
            float py = ux * 6.f;

            sf::ConvexShape arrow(3);
            arrow.setPoint(0, {tipX, tipY});
            arrow.setPoint(1, {backX + px, backY + py});
            arrow.setPoint(2, {backX - px, backY - py});
            arrow.setFillColor(col);
            window.draw(arrow);
        }

        // Edge label (capacity if > 0, else weight; include state tag as secondary cue)
        std::string lbl;
        if (e.capacity > 0.0) {
            double fl = (visuals.edgeFlow && i < visuals.edgeCount) ? visuals.edgeFlow[i] : 0.0;
            lbl = formatNum(fl) + "/" + formatNum(e.capacity);
        } else {
            lbl = formatNum(e.weight);
        }
        std::string tag = stateTag(st);
        if (!tag.empty()) lbl += " [" + tag + "]";

        float midX = (x1 + x2) * 0.5f - uy * 14.f;
        float midY = (y1 + y2) * 0.5f + ux * 14.f;

        sf::RectangleShape pill({static_cast<float>(lbl.size()) * 7.5f + 10.f, 18.f});
        pill.setOrigin({pill.getSize().x * 0.5f, pill.getSize().y * 0.5f});
        pill.setPosition({midX, midY});
        pill.setFillColor(sf::Color(16, 22, 36, 225));
        pill.setOutlineThickness(1.f);
        pill.setOutlineColor(col);
        window.draw(pill);

        sf::Text wtText(font, lbl, 11);
        wtText.setFillColor(sf::Color(235, 242, 255));
        auto wb = wtText.getLocalBounds();
        wtText.setOrigin({wb.position.x + wb.size.x * 0.5f, wb.position.y + wb.size.y * 0.5f});
        wtText.setPosition({midX, midY});
        window.draw(wtText);
    }

    // 2. Draw Vertices
    for (int i = 0; i < vCount; ++i) {
        const Vertex* vPtr = graph.getVertex(i);
        if (!vPtr) continue;
        const Vertex& v = *vPtr;
        int st = (visuals.vertexStates && i < visuals.vertexCount) ? visuals.vertexStates[i] : VS_UNVISITED;
        bool hovered = (v.id == hoveredVertexId);

        float vx = canvasX + v.x;
        float vy = canvasY + v.y;

        if (hovered || st == VS_CURRENT || st == VS_ACCEPTED) {
            sf::CircleShape halo(nodeRadius + 5.f);
            halo.setOrigin({nodeRadius + 5.f, nodeRadius + 5.f});
            halo.setPosition({vx, vy});
            halo.setFillColor(sf::Color::Transparent);
            halo.setOutlineThickness(2.f);
            halo.setOutlineColor(hovered ? sf::Color(255, 255, 255, 180) : vertexOutlineColor(st));
            window.draw(halo);
        }

        sf::CircleShape circle(nodeRadius);
        circle.setOrigin({nodeRadius, nodeRadius});
        circle.setPosition({vx, vy});
        circle.setFillColor(vertexFillColor(st));
        circle.setOutlineThickness((st == VS_UNVISITED) ? 2.f : 3.2f);
        circle.setOutlineColor(vertexOutlineColor(st));
        window.draw(circle);

        // Vertex ID inside circle
        sf::Text idTxt(font, std::to_string(v.id), 15);
        idTxt.setFillColor(sf::Color::White);
        auto ib = idTxt.getLocalBounds();
        idTxt.setOrigin({ib.position.x + ib.size.x * 0.5f, ib.position.y + ib.size.y * 0.5f});
        idTxt.setPosition({vx, vy});
        window.draw(idTxt);

        // Vertex label below circle
        sf::Text lblTxt(font, v.label, 12);
        lblTxt.setFillColor(sf::Color(215, 225, 242));
        auto lb = lblTxt.getLocalBounds();
        lblTxt.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y});
        lblTxt.setPosition({vx, vy + nodeRadius + 6.f});
        window.draw(lblTxt);

        // Non-colour secondary symbol badge on top-right
        std::string tag = stateTag(st);
        if (!tag.empty()) {
            sf::CircleShape badge(9.f);
            badge.setOrigin({9.f, 9.f});
            badge.setPosition({vx + nodeRadius - 2.f, vy - nodeRadius + 2.f});
            badge.setFillColor(vertexOutlineColor(st));
            window.draw(badge);

            sf::Text tagTxt(font, tag, 9);
            tagTxt.setFillColor(sf::Color(10, 15, 25));
            auto tb = tagTxt.getLocalBounds();
            tagTxt.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
            tagTxt.setPosition({vx + nodeRadius - 2.f, vy - nodeRadius + 2.f});
            window.draw(tagTxt);
        }
    }
}

int Renderer::hitTestVertex(const Graph& graph,
                            float mx,
                            float my,
                            float canvasX,
                            float canvasY,
                            float radius) const {
    const int vCount = graph.getVertexCount();
    for (int i = 0; i < vCount; ++i) {
        const Vertex* vPtr = graph.getVertex(i);
        if (!vPtr) continue;
        float dx = mx - (canvasX + vPtr->x);
        float dy = my - (canvasY + vPtr->y);
        if (dx * dx + dy * dy <= radius * radius) {
            return vPtr->id;
        }
    }
    return -1;
}

void Renderer::drawPanel(sf::RenderWindow& window,
                         float x, float y, float w, float h,
                         sf::Color fill, sf::Color outline, float thickness) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(fill);
    rect.setOutlineThickness(thickness);
    rect.setOutlineColor(outline);
    window.draw(rect);
}

void Renderer::drawButton(sf::RenderWindow& window,
                          const sf::Font& font,
                          const std::string& label,
                          float x, float y, float w, float h,
                          bool hovered,
                          sf::Color fill,
                          sf::Color outline,
                          unsigned int charSize) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(hovered ? sf::Color(fill.r + 18, fill.g + 22, fill.b + 30) : fill);
    rect.setOutlineThickness(hovered ? 2.6f : 1.8f);
    rect.setOutlineColor(hovered ? sf::Color(250, 205, 60) : outline);
    window.draw(rect);

    sf::Text txt(font, label, charSize);
    txt.setFillColor(sf::Color::White);
    auto b = txt.getLocalBounds();
    txt.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    txt.setPosition({x + w * 0.5f, y + h * 0.5f});
    window.draw(txt);
}

bool Renderer::isInside(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

} // namespace EscapeNetwork
