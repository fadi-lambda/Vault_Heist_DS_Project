#pragma once
#include <string>

namespace VaultHeist {

// Generic drag-and-drop mechanism: pick up a "token" (a value / node) with the mouse
// and drop it on a "slot" (a tree position, a pivot choice, ...). Snaps to the slot on
// release. SFML-free on purpose so it can be unit tested; see DragDropView.h for drawing.
//
// Usage per frame/event:
//   press(mx,my) on MouseButtonPressed, move(mx,my) on every mouse move,
//   release(mx,my) on MouseButtonReleased -> returns a DropResult.

static const int DRAG_MAX_TOKENS = 32;
static const int DRAG_MAX_SLOTS  = 32;
static const int DRAG_ANY_VALUE  = -1000000;   // slot accepts every token value

// DropResult::outcome values (no enum)
static const int DROP_NONE      = 0;  // nothing was being dragged
static const int DROP_SNAPPED   = 1;  // token snapped into a slot
static const int DROP_REJECTED  = 2;  // released over a slot that refuses it (occupied / wrong value)
static const int DROP_CANCELLED = 3;  // released over empty space: token returned to where it came from

struct DragToken {
    int   id;
    int   value;
    std::string label;      // optional display label (defaults to value)
    float homeX, homeY;     // rest position when not in a slot
    float x, y;             // current rest position (follows slot centre when snapped)
    float radius;
    int   slotId;           // slot currently holding the token, or -1
    bool  enabled;
    bool  used;
    DragToken() : id(-1), value(0), homeX(0), homeY(0), x(0), y(0), radius(24.f),
                  slotId(-1), enabled(true), used(false) {}
};

struct DropSlot {
    int   id;
    float x, y;             // centre
    float radius;           // snap radius
    int   acceptValue;      // DRAG_ANY_VALUE or the only value this slot accepts
    std::string label;      // optional label shown when slot is empty
    int   tokenId;          // occupant token id, or -1
    bool  enabled;
    bool  used;
    DropSlot() : id(-1), x(0), y(0), radius(30.f), acceptValue(DRAG_ANY_VALUE),
                 tokenId(-1), enabled(true), used(false) {}
};

struct DropResult {
    int outcome;     // DROP_*
    int tokenId;
    int tokenValue;
    int slotId;      // slot snapped into / rejected by, else -1
    DropResult() : outcome(DROP_NONE), tokenId(-1), tokenValue(0), slotId(-1) {}
};

class DragDropController {
public:
    DragDropController();

    void clear();

    // Returns the new token / slot id (== index), or -1 if full.
    int addToken(int value, float homeX, float homeY, float radius = 24.f);
    int addSlot(float x, float y, float radius = 30.f, int acceptValue = DRAG_ANY_VALUE);

    void setTokenEnabled(int tokenId, bool enabled);
    void setSlotEnabled(int slotId, bool enabled);
    void setTokenLabel(int tokenId, const std::string& label);
    void setSlotLabel(int slotId, const std::string& label);

    // Mouse interaction. press() returns true when a token was picked up.
    bool press(float mx, float my);
    void move(float mx, float my);
    DropResult release(float mx, float my);
    // Abort the current drag, token returns to its previous place.
    void cancel();

    // Programmatic helpers
    bool placeTokenInSlot(int tokenId, int slotId);   // snap without dragging
    void returnTokenHome(int tokenId);

    // State for rendering / game logic
    bool  isDragging() const { return dragToken >= 0; }
    int   draggedTokenId() const { return dragToken; }
    float dragX() const { return curX; }              // token centre while dragging
    float dragY() const { return curY; }
    // Slot under the cursor while dragging (-1 if none) and whether it would accept the token
    int   hoveredSlotId() const { return hoverSlot; }
    bool  hoveredSlotAccepts() const { return hoverAccepts; }

    const DragToken* token(int id) const;
    const DropSlot*  slot(int id) const;
    int tokenCount() const { return tokenTotal; }
    int slotCount() const { return slotTotal; }
    int tokenInSlot(int slotId) const;                // occupant token id or -1

    // Whether the token could legally go into the slot right now
    bool canAccept(int tokenId, int slotId) const;

private:
    DragToken tokens[DRAG_MAX_TOKENS];
    DropSlot  slots[DRAG_MAX_SLOTS];
    int tokenTotal;
    int slotTotal;

    int   dragToken;
    float grabDX, grabDY;   // cursor offset from token centre at pickup
    float curX, curY;
    int   originSlot;       // slot the token came from, or -1 (home)
    int   hoverSlot;
    bool  hoverAccepts;

    int  slotAt(float x, float y) const;   // nearest enabled slot within snap radius
    void updateHover();
};

} // namespace VaultHeist
