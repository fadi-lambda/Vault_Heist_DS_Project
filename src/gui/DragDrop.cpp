#include "gui/DragDrop.h"

namespace VaultHeist {

static inline float sqDist(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy;
}

DragDropController::DragDropController() {
    tokenTotal = 0;
    slotTotal = 0;
    dragToken = -1;
    grabDX = grabDY = 0.f;
    curX = curY = 0.f;
    originSlot = -1;
    hoverSlot = -1;
    hoverAccepts = false;
}

void DragDropController::clear() {
    for (int i = 0; i < DRAG_MAX_TOKENS; ++i) tokens[i] = DragToken();
    for (int i = 0; i < DRAG_MAX_SLOTS; ++i) slots[i] = DropSlot();
    tokenTotal = 0;
    slotTotal = 0;
    dragToken = -1;
    originSlot = -1;
    hoverSlot = -1;
    hoverAccepts = false;
}

int DragDropController::addToken(int value, float homeX, float homeY, float radius) {
    if (tokenTotal >= DRAG_MAX_TOKENS) return -1;
    DragToken& t = tokens[tokenTotal];
    t = DragToken();
    t.id = tokenTotal;
    t.value = value;
    t.homeX = t.x = homeX;
    t.homeY = t.y = homeY;
    t.radius = radius;
    t.used = true;
    return tokenTotal++;
}

int DragDropController::addSlot(float x, float y, float radius, int acceptValue) {
    if (slotTotal >= DRAG_MAX_SLOTS) return -1;
    DropSlot& s = slots[slotTotal];
    s = DropSlot();
    s.id = slotTotal;
    s.x = x;
    s.y = y;
    s.radius = radius;
    s.acceptValue = acceptValue;
    s.used = true;
    return slotTotal++;
}

void DragDropController::setTokenEnabled(int tokenId, bool enabled) {
    if (tokenId >= 0 && tokenId < tokenTotal) tokens[tokenId].enabled = enabled;
}

void DragDropController::setSlotEnabled(int slotId, bool enabled) {
    if (slotId >= 0 && slotId < slotTotal) slots[slotId].enabled = enabled;
}

void DragDropController::setTokenLabel(int tokenId, const std::string& label) {
    if (tokenId >= 0 && tokenId < tokenTotal) tokens[tokenId].label = label;
}

void DragDropController::setSlotLabel(int slotId, const std::string& label) {
    if (slotId >= 0 && slotId < slotTotal) slots[slotId].label = label;
}

const DragToken* DragDropController::token(int id) const {
    return (id >= 0 && id < tokenTotal) ? &tokens[id] : nullptr;
}

const DropSlot* DragDropController::slot(int id) const {
    return (id >= 0 && id < slotTotal) ? &slots[id] : nullptr;
}

int DragDropController::tokenInSlot(int slotId) const {
    return (slotId >= 0 && slotId < slotTotal) ? slots[slotId].tokenId : -1;
}

bool DragDropController::canAccept(int tokenId, int slotId) const {
    if (tokenId < 0 || tokenId >= tokenTotal || slotId < 0 || slotId >= slotTotal) return false;
    const DropSlot& s = slots[slotId];
    if (!s.enabled) return false;
    if (s.tokenId != -1 && s.tokenId != tokenId) return false;   // occupied by someone else
    if (s.acceptValue != DRAG_ANY_VALUE && s.acceptValue != tokens[tokenId].value) return false;
    return true;
}

int DragDropController::slotAt(float x, float y) const {
    int best = -1;
    float bestD = 0.f;
    for (int i = 0; i < slotTotal; ++i) {
        const DropSlot& s = slots[i];
        if (!s.enabled) continue;
        float d = sqDist(x, y, s.x, s.y);
        if (d <= s.radius * s.radius && (best < 0 || d < bestD)) {
            best = i;
            bestD = d;
        }
    }
    return best;
}

void DragDropController::updateHover() {
    hoverSlot = -1;
    hoverAccepts = false;
    if (dragToken < 0) return;
    hoverSlot = slotAt(curX, curY);
    if (hoverSlot >= 0) hoverAccepts = canAccept(dragToken, hoverSlot);
}

bool DragDropController::press(float mx, float my) {
    if (dragToken >= 0) return false;
    int pick = -1;
    float bestD = 0.f;
    // Topmost = highest id wins ties; closest centre wins otherwise
    for (int i = 0; i < tokenTotal; ++i) {
        const DragToken& t = tokens[i];
        if (!t.enabled) continue;
        float d = sqDist(mx, my, t.x, t.y);
        if (d <= t.radius * t.radius && (pick < 0 || d <= bestD)) {
            pick = i;
            bestD = d;
        }
    }
    if (pick < 0) return false;
    dragToken = pick;
    grabDX = tokens[pick].x - mx;
    grabDY = tokens[pick].y - my;
    curX = tokens[pick].x;
    curY = tokens[pick].y;
    originSlot = tokens[pick].slotId;
    updateHover();
    return true;
}

void DragDropController::move(float mx, float my) {
    if (dragToken < 0) return;
    curX = mx + grabDX;
    curY = my + grabDY;
    updateHover();
}

DropResult DragDropController::release(float mx, float my) {
    DropResult r;
    if (dragToken < 0) return r;
    move(mx, my);

    DragToken& t = tokens[dragToken];
    r.tokenId = t.id;
    r.tokenValue = t.value;

    int target = slotAt(curX, curY);
    if (target < 0 && slotAt(mx, my) >= 0) target = slotAt(mx, my);   // cursor itself over a slot

    if (target >= 0 && canAccept(t.id, target)) {
        // Vacate previous slot
        if (t.slotId >= 0) slots[t.slotId].tokenId = -1;
        slots[target].tokenId = t.id;
        t.slotId = target;
        t.x = slots[target].x;
        t.y = slots[target].y;
        r.outcome = DROP_SNAPPED;
        r.slotId = target;
    } else {
        // Return to origin (slot or home)
        if (t.slotId >= 0 && t.slotId == originSlot) {
            t.x = slots[t.slotId].x;
            t.y = slots[t.slotId].y;
        } else {
            t.x = t.homeX;
            t.y = t.homeY;
        }
        r.outcome = (target >= 0) ? DROP_REJECTED : DROP_CANCELLED;
        r.slotId = target;
    }

    dragToken = -1;
    originSlot = -1;
    hoverSlot = -1;
    hoverAccepts = false;
    return r;
}

void DragDropController::cancel() {
    if (dragToken < 0) return;
    DragToken& t = tokens[dragToken];
    if (t.slotId >= 0) { t.x = slots[t.slotId].x; t.y = slots[t.slotId].y; }
    else { t.x = t.homeX; t.y = t.homeY; }
    dragToken = -1;
    originSlot = -1;
    hoverSlot = -1;
    hoverAccepts = false;
}

bool DragDropController::placeTokenInSlot(int tokenId, int slotId) {
    if (!canAccept(tokenId, slotId)) return false;
    DragToken& t = tokens[tokenId];
    if (t.slotId >= 0) slots[t.slotId].tokenId = -1;
    slots[slotId].tokenId = tokenId;
    t.slotId = slotId;
    t.x = slots[slotId].x;
    t.y = slots[slotId].y;
    return true;
}

void DragDropController::returnTokenHome(int tokenId) {
    if (tokenId < 0 || tokenId >= tokenTotal) return;
    DragToken& t = tokens[tokenId];
    if (t.slotId >= 0) slots[t.slotId].tokenId = -1;
    t.slotId = -1;
    t.x = t.homeX;
    t.y = t.homeY;
}

} // namespace VaultHeist
