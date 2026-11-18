#pragma once
#include <SFML/Graphics.hpp>
#include "gui/DragDrop.h"
#include "gui/Ui.h"

namespace VaultHeist {

// Draws a DragDropController: slot rings (highlighted green/red while a token hovers them),
// resting tokens, and the dragged token lifted under the cursor with a shadow and a
// guide line back to its origin.
class DragDropView {
public:
    static void draw(sf::RenderTarget& target, const sf::Font& font,
                     const DragDropController& dd, UiCache* cache = nullptr);
};

} // namespace VaultHeist
