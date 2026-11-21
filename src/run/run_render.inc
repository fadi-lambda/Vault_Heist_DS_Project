
    // =========================================================================
    // SHARED FRAME RENDERER (used by both main loop and --verify-screenshots)
    // =========================================================================
    auto renderFrame = [&](const UiInput& in, const std::string& capturePath = "",
                           double* outCpuMs = nullptr, double* outDispMs = nullptr) {
        sf::Clock frameCpuClk;
        int curLid = (currentLevelIdx >= 0 && currentLevelIdx < TOTAL_LEVELS) ? levels[currentLevelIdx].id : -1;
        bool needLayout = layoutDirty || (cachedLayoutScreen != currentScreen) ||
                          (currentScreen == SCREEN_LEVEL_PLAY && cachedLayoutLid != curLid);

        auto mapHoverToCam = [&](const CanvasCamera& cam, float sx, float sy, sf::Vector2f& outW) -> bool {
            if (!cam.containsPixel(sx, sy)) return false;
            outW = window.mapPixelToCoords({static_cast<int>(sx), static_cast<int>(sy)}, cam.view);
            return true;
        };
        auto drawCamTooltip = [&](const CanvasCamera& cam, const LayoutResult& layout, int hoveredNodeId) {
            if (hoveredNodeId < 0) return;
            int hIdx = layout.findNodeIndexById(hoveredNodeId);
            if (hIdx < 0) return;
            sf::Vector2i sp = window.mapCoordsToPixel({layout.nodes[hIdx].x, layout.nodes[hIdx].y}, cam.view);
            if (cam.containsPixel(static_cast<float>(sp.x), static_cast<float>(sp.y))) {
                guiRenderer.drawNodeTooltip(window, font, layout, hoveredNodeId,
                                            static_cast<float>(W), static_cast<float>(H),
                                            static_cast<float>(sp.x), static_cast<float>(sp.y));
            }
        };

        window.clear(Ui::bgNavy());
        if (showBgGrid && currentScreen != SCREEN_MAIN_MENU &&
            currentScreen != SCREEN_SPLASH && currentScreen != SCREEN_CAMPAIGN_MAP &&
            currentScreen != SCREEN_SETTINGS && currentScreen != SCREEN_LEADERBOARD &&
            currentScreen != SCREEN_ENDLESS_LEADERBOARD && currentScreen != SCREEN_RESULT) {
            Ui::grid(window, static_cast<float>(W), static_cast<float>(H), &uiCache);
        }

        // 0. SPLASH SCREEN
        if (currentScreen == SCREEN_SPLASH) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color splashBright(0, 255, 65);
            const sf::Color splashDim(74, 122, 74);
            float cx = W * 0.5f;

            sf::Text t1(menuFont, "VAULT ", 56);
            sf::Text t2(menuFont, "HEIST", 56);

            // 2. .width ki jagah .size.x use karein
            float w1 = t1.getLocalBounds().size.x;
            float w2 = t2.getLocalBounds().size.x;

            float startX = cx - (w1 + w2) / 2.f;

            Ui::text(window, menuFont, "VAULT ", 56, splashBright, startX, 326.f, true, &uiCache);
            Ui::text(window, menuFont, "HEIST", 56, sf::Color::White, startX + w1, 326.f, true, &uiCache);

            Ui::textCenter(window, menuFont, "THE LIVING VAULT", 18, sf::Color(255, 210, 40), cx, 396.f, false, &uiCache);

            float barW = 400.f, barH = 10.f;
            float barX = cx - barW * 0.5f, barY = 442.f;
            sf::RectangleShape pTrack({barW, barH});
            pTrack.setPosition({barX, barY});
            pTrack.setFillColor(sf::Color(12, 26, 15));
            pTrack.setOutlineColor(sf::Color(0, 255, 65, 100));
            pTrack.setOutlineThickness(1.f);
            window.draw(pTrack);

            float prog = minFloat(1.0f, splashTimer / 3.2f);
            if (prog > 0.0f) {
                sf::RectangleShape pFill({barW * prog, barH});
                pFill.setPosition({barX, barY});
                pFill.setFillColor(splashBright);
                window.draw(pFill);
            }

            const char* splashMsgs[4] = {
                "Cracking security protocols...",
                "Decrypting vault blueprints...",
                "Bypassing biometric locks...",
                "Initializing heist terminal..."
            };
            int msgIdx = static_cast<int>(splashTimer / 0.8f) % 4;
            if (msgIdx < 0) msgIdx = 0;
            Ui::textCenter(window, menuFont, splashMsgs[msgIdx], 15, sf::Color::White, cx, 474.f, false, &uiCache);
            Ui::textCenter(window, menuFont, "[Press any key or click to skip]", 14, sf::Color::White, cx, 780.f, false, &uiCache);
        }

        // 1. MAIN MENU
        if (currentScreen == SCREEN_MAIN_MENU) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }

            const sf::Color menuBright(0, 255, 65);
            const sf::Color menuDim(74, 122, 74);

            // Thin green border rectangle around entire screen
            sf::RectangleShape screenBorder({static_cast<float>(W) - 32.f, static_cast<float>(H) - 32.f});
            screenBorder.setPosition({16.f, 16.f});
            screenBorder.setFillColor(sf::Color::Transparent);
            screenBorder.setOutlineColor(sf::Color(0, 255, 65, 110));
            screenBorder.setOutlineThickness(1.f);
            window.draw(screenBorder);

            // Top-left title: "VAULT" (#00FF41) + "HEIST" (#FFFFFF)
            float titleX = 100.f;
            float titleY = 96.f;
            float vaultAdv = Ui::textWidth(menuFont, "VAULT ", 64, true);
            Ui::text(window, menuFont, "VAULT", 64, menuBright, titleX, titleY, true, &uiCache);
            Ui::text(window, menuFont, "HEIST", 64, sf::Color::White, titleX + vaultAdv, titleY, true, &uiCache);

            // Subtle accent line under title
            sf::RectangleShape titleLine({540.f, 1.5f});
            titleLine.setPosition({titleX, titleY + 86.f});
            titleLine.setFillColor(sf::Color(0, 255, 65, 160));
            window.draw(titleLine);

            // Subtitle below title
            Ui::text(window, menuFont, "BREAK  IN   .   TAKE  CONTROL   .   GET  OUT", 15, sf::Color(255, 210, 40), titleX, titleY + 100.f, false, &uiCache);

            // Top-right corner: "SECURE SYSTEM ▪"
            float ssW = Ui::textWidth(menuFont, "SECURE SYSTEM", 14, false);
            Ui::text(window, menuFont, "SECURE SYSTEM", 14, sf::Color::White,
                     static_cast<float>(W) - 62.f - ssW, 34.f, false, &uiCache);
            sf::RectangleShape topSq({10.f, 10.f});
            topSq.setPosition({static_cast<float>(W) - 46.f, 37.f});
            topSq.setFillColor(menuBright);
            window.draw(topSq);

            // Left-aligned menu list (x ~ 120px, starting y ~ 300px, spacing 70px)
            const char* menuItems[6] = {
                "1.  CAMPAIGN",
                "2.  ENDLESS",
                "3.  SANDBOX",
                "4.  LEADERBOARD",
                "5.  SETTINGS",
                "6.  QUIT"
            };
            int curMenuHov = -1;
            for (int i = 0; i < 6; ++i) {
                float itemY = 300.f + static_cast<float>(i) * 70.f;
                if (Ui::inside(in.mx, in.my, 90.f, itemY - 10.f, 430.f, 54.f)) curMenuHov = i;
                bool sel = (i == menuSelectedIdx);
                if (sel) {
                    Ui::text(window, menuFont, ">", 24, menuBright, 96.f, itemY, true, &uiCache);
                    Ui::text(window, menuFont, menuItems[i], 24, menuBright, 128.f, itemY, true, &uiCache);
                    sf::RectangleShape itemUnderline({360.f, 2.f});
                    itemUnderline.setPosition({128.f, itemY + 36.f});
                    itemUnderline.setFillColor(menuBright);
                    window.draw(itemUnderline);
                } else {
                    Ui::text(window, menuFont, menuItems[i], 24, sf::Color::White, 128.f, itemY, false, &uiCache);
                }
            }
            if (curMenuHov != mainMenuHoveredIdx) {
                if (curMenuHov != -1) playSfx(menuClickSnd);
                mainMenuHoveredIdx = curMenuHov;
            }

            // Bottom full-width thin green line (height 1px, #00FF41)
            sf::RectangleShape bottomLine({static_cast<float>(W) - 32.f, 1.f});
            bottomLine.setPosition({16.f, static_cast<float>(H) - 64.f});
            bottomLine.setFillColor(menuBright);
            window.draw(bottomLine);

            // Bottom-left and bottom-right small green squares
            sf::RectangleShape blSq({10.f, 10.f});
            blSq.setPosition({32.f, static_cast<float>(H) - 44.f});
            blSq.setFillColor(menuBright);
            window.draw(blSq);

            sf::RectangleShape brSq({10.f, 10.f});
            brSq.setPosition({static_cast<float>(W) - 42.f, static_cast<float>(H) - 44.f});
            brSq.setFillColor(menuBright);
            window.draw(brSq);

            // Bottom-left version & prompt text
            Ui::text(window, menuFont, "v1.0.0", 14, sf::Color::White, 58.f, static_cast<float>(H) - 48.f, false, &uiCache);
        }

        // 2. CAMPAIGN MAP
        if (currentScreen == SCREEN_CAMPAIGN_MAP) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }

            const sf::Color mapBright(0, 255, 65);
            const sf::Color mapDim(74, 122, 74);
            const sf::Color mapSubtle(110, 185, 110);

            UiInput mapIn = in;
            if (demoPromptOpen) {
                mapIn.mx = -1.f;
                mapIn.my = -1.f;
                mapIn.down = false;
            }

            static const char* actNames[4] = {
                "ACT 1 : BLUEPRINT WING",
                "ACT 2 : SORTED WING",
                "ACT 3 : ARCHITECT'S WING",
                "ACT 4 : HEAP FORTRESS"
            };
            static const char* actShort[4] = {
                "ACT 1", "ACT 2", "ACT 3", "ACT 4"
            };
            static const char* lvlNames[4][5] = {
                { "The Blueprint", "Decode the Blueprint", "The Alarm Spreads", "Two Routes Meet", "Mirror the Vault" },
                { "Search Heist", "Stash the Loot", "Steal and Collapse", "Range Raid", "The Lazy Architect" },
                { "Read the Badges", "Single Rotations", "Double Rotations", "Auto Mode", "Order-Statistic Heist" },
                { "Activate the Vault", "Repair Security Locks", "Extract the Greatest", "Race the Guards", "Final Vault Boss" }
            };
            static const char* lvlDescs[4][5] = {
                {
                    "Visit all rooms in traversal order to map the vault layout.",
                    "Reconstruct the tree from Pre-order and In-order sequences.",
                    "Calculate room heights and tree diameter before alarms trigger.",
                    "Find the common ancestor of two rooms and trace the path.",
                    "Invert the vault blueprint to expose the hidden exit."
                },
                {
                    "Navigate the BST to locate the target key before time runs out.",
                    "Insert new keys into the BST without breaking sort order.",
                    "Delete targets using correct BST deletion cases.",
                    "Report all rooms within the target range, prune the rest.",
                    "Insert sorted keys into a degenerating BST chain. Boss level."
                },
                {
                    "Identify all unbalanced nodes with violation badges.",
                    "Apply LL or RR rotations to restore AVL balance.",
                    "Execute LR and RL double rotations on zig-zag imbalances.",
                    "Predict the rotation type before each insertion.",
                    "Steal the k-th largest treasure in O(log n). Boss level."
                },
                {
                    "Build a Max Heap bottom-up from an unsorted array.",
                    "Sift down the sabotaged root to restore heap order.",
                    "Extract treasures one by one from the heap root.",
                    "Extract all treasures before the alarm capacity fills.",
                    "Build heap then heap-sort all treasures. Final boss."
                }
            };

            // Outer screen border
            sf::RectangleShape mapBorder({static_cast<float>(W) - 32.f, static_cast<float>(H) - 32.f});
            mapBorder.setPosition({16.f, 16.f});
            mapBorder.setFillColor(sf::Color::Transparent);
            mapBorder.setOutlineColor(sf::Color(0, 255, 65, 90));
            mapBorder.setOutlineThickness(1.f);
            window.draw(mapBorder);

            // Top bar: "VAULT HEIST" left, active act center, "BACK TO MENU [ESC]" right
            sf::Text t1(menuFont, "VAULT ", 28);
            t1.setStyle(sf::Text::Bold);
            float w1 = t1.getLocalBounds().size.x;

            // 2. Dono ko alag-alag colour mein draw karein
            Ui::text(window, menuFont, "VAULT ", 28, mapBright, 44.f, 24.f, true, &uiCache);
            Ui::text(window, menuFont, "HEIST", 28, sf::Color::White, 44.f + w1, 24.f, true, &uiCache);

            const char* curActTitle = actNames[selectedActIdx];
            float actTitleW = Ui::textWidth(menuFont, curActTitle, 20, true);
            float actTitleX = static_cast<float>(W) * 0.5f - actTitleW * 0.5f;
            {
                std::string actStr(curActTitle);
                size_t sep = actStr.find(" : ");
                if (sep != std::string::npos) {
                    std::string actHead = actStr.substr(0, sep);
                    std::string actTail = actStr.substr(sep);
                    float actHeadW = Ui::textWidth(menuFont, actHead, 20, true);
                    Ui::text(window, menuFont, actHead, 20, mapBright, actTitleX, 28.f, true, &uiCache);
                    Ui::text(window, menuFont, actTail, 20, sf::Color::White, actTitleX + actHeadW, 28.f, true, &uiCache);
                } else {
                    Ui::text(window, menuFont, curActTitle, 20, mapBright, actTitleX, 28.f, true, &uiCache);
                }
            }

            float topLineLW = actTitleX - 380.f;
            if (topLineLW < 20.f) topLineLW = 20.f;
            sf::RectangleShape topLineL({topLineLW, 1.f});
            topLineL.setPosition({350.f, 39.f});
            topLineL.setFillColor(sf::Color(0, 255, 65, 130));
            window.draw(topLineL);

            float topLineRW = 1320.f - (actTitleX + actTitleW + 30.f);
            if (topLineRW < 20.f) topLineRW = 20.f;
            sf::RectangleShape topLineR({topLineRW, 1.f});
            topLineR.setPosition({actTitleX + actTitleW + 30.f, 39.f});
            topLineR.setFillColor(sf::Color(0, 255, 65, 130));
            window.draw(topLineR);

            bool backHov = Ui::inside(mapIn.mx, mapIn.my, 1350.f, 20.f, 220.f, 38.f);
            Ui::text(window, menuFont, "BACK TO MENU [ESC]", 15, backHov ? sf::Color::White : mapDim,
                     1370.f, 30.f, backHov, &uiCache);

            // Left panel (~280px wide): 4 act buttons connected by dots/lines
            sf::RectangleShape leftDiv({1.f, 670.f});
            leftDiv.setPosition({300.f, 110.f});
            leftDiv.setFillColor(sf::Color(0, 255, 65, 90));
            window.draw(leftDiv);

            int curCampHov = -1;
            for (int a = 0; a < 4; ++a) {
                float bx = 44.f;
                float by = 190.f + static_cast<float>(a) * 124.f;
                float bw = 220.f, bh = 66.f;
                bool active = (a == selectedActIdx);
                bool hov = Ui::inside(mapIn.mx, mapIn.my, bx, by, bw, bh);
                if (hov) curCampHov = 100 + a;

                sf::RectangleShape actBox({bw, bh});
                actBox.setPosition({bx, by});
                actBox.setFillColor(active ? sf::Color(8, 28, 12, 225) : sf::Color(6, 14, 8, 200));
                actBox.setOutlineColor((active || hov) ? mapBright : mapDim);
                actBox.setOutlineThickness(active ? 2.f : 1.f);
                window.draw(actBox);

                std::string actLbl = active ? (std::string("> ") + actShort[a]) : std::string(actShort[a]);
                Ui::textCenter(window, menuFont, actLbl, 20, (active || hov) ? mapBright : sf::Color::White,
                               bx + bw * 0.5f, by + 20.f, active, &uiCache);

                if (a < 3) {
                    float cx = bx + bw * 0.5f;
                    sf::RectangleShape conn({2.f, 58.f});
                    conn.setPosition({cx - 1.f, by + bh});
                    conn.setFillColor(sf::Color(0, 255, 65, 130));
                    window.draw(conn);

                    sf::CircleShape dot(5.f);
                    dot.setOrigin({5.f, 5.f});
                    dot.setPosition({cx, by + bh + 29.f});
                    dot.setFillColor(mapBright);
                    window.draw(dot);
                }
            }

            // Center area: 5 level nodes for active act as circles connected by lines
            const float nodeX[5] = { 400.f, 520.f, 640.f, 760.f, 880.f };
            const float nodeY[5] = { 360.f, 460.f, 295.f, 450.f, 305.f };
            const float nodeR = 36.f;

            for (int i = 0; i < 4; ++i) {
                int liNext = selectedActIdx * 5 + (i + 1);
                sf::Color lineCol = levels[liNext].unlocked ? mapBright : mapDim;
                sf::Vertex seg[] = {
                    sf::Vertex({nodeX[i], nodeY[i]}, lineCol),
                    sf::Vertex({nodeX[i + 1], nodeY[i + 1]}, lineCol)
                };
                window.draw(seg, 2, sf::PrimitiveType::Lines);
            }

            for (int i = 0; i < 5; ++i) {
                int li = selectedActIdx * 5 + i;
                bool unl = levels[li].unlocked;
                bool isSel = (i == selectedLevelInAct);
                float dx = mapIn.mx - nodeX[i];
                float dy = mapIn.my - nodeY[i];
                bool hov = (dx * dx + dy * dy) <= (nodeR * nodeR);
                if (hov) curCampHov = 200 + i;

                sf::CircleShape nodeCirc(nodeR);
                nodeCirc.setOrigin({nodeR, nodeR});
                nodeCirc.setPosition({nodeX[i], nodeY[i]});
                nodeCirc.setFillColor(unl ? sf::Color(8, 28, 12, 235) : sf::Color(6, 14, 8, 225));
                nodeCirc.setOutlineColor((unl || isSel || hov) ? mapBright : mapDim);
                nodeCirc.setOutlineThickness(isSel ? 3.f : (unl ? 2.f : 1.5f));
                window.draw(nodeCirc);

                if (unl) {
                    char numBuf[4];
                    numBuf[0] = static_cast<char>('1' + i);
                    numBuf[1] = '\0';
                    Ui::textCenter(window, menuFont, numBuf, 28, sf::Color::White,
                                   nodeX[i], nodeY[i] - 18.f, true, &uiCache);
                    if (levels[li].stars > 0) {
                        Ui::stars(window, nodeX[i], nodeY[i] + nodeR + 14.f, levels[li].stars, 3, 6.f, &uiCache);
                    }
                } else {
                    sf::Color lockCol = sf::Color::White;
                    // Padlock shackle
                    sf::RectangleShape shackle({13.f, 10.5f});
                    shackle.setOrigin({6.5f, 10.5f});
                    shackle.setPosition({nodeX[i], nodeY[i] - 2.f});
                    shackle.setFillColor(sf::Color::Transparent);
                    shackle.setOutlineColor(lockCol);
                    shackle.setOutlineThickness(2.0f);
                    window.draw(shackle);
                    // Padlock body
                    sf::RectangleShape lockBody({19.f, 14.5f});
                    lockBody.setOrigin({9.5f, 0.f});
                    lockBody.setPosition({nodeX[i], nodeY[i] - 2.f});
                    lockBody.setFillColor(lockCol);
                    window.draw(lockBody);
                    // Keyhole
                    sf::RectangleShape keyhole({3.f, 5.5f});
                    keyhole.setOrigin({1.5f, 0.f});
                    keyhole.setPosition({nodeX[i], nodeY[i] + 3.f});
                    keyhole.setFillColor(sf::Color(6, 14, 8));
                    window.draw(keyhole);
                }
            }
            if (curCampHov != campaignHoveredId) {
                if (curCampHov != -1) playSfx(campaignClickSnd);
                campaignHoveredId = curCampHov;
            }

            // Bottom info panel (~180px tall)
            float infoX = 340.f, infoY = 600.f, infoW = 840.f, infoH = 180.f;
            sf::RectangleShape infoPanel({infoW, infoH});
            infoPanel.setPosition({infoX, infoY});
            infoPanel.setFillColor(sf::Color(6, 14, 10, 225));
            infoPanel.setOutlineColor(sf::Color(0, 255, 65, 140));
            infoPanel.setOutlineThickness(1.f);
            window.draw(infoPanel);

            // Top-left green bar accent on info panel
            sf::RectangleShape infoAccent({52.f, 4.f});
            infoAccent.setPosition({infoX, infoY});
            infoAccent.setFillColor(mapBright);
            window.draw(infoAccent);

            std::string missionTitle = "Mission " + std::to_string(selectedActIdx + 1) + "." +
                                       std::to_string(selectedLevelInAct + 1) + " : " +
                                       lvlNames[selectedActIdx][selectedLevelInAct];
            {
                size_t mSep = missionTitle.find(" : ");
                if (mSep != std::string::npos) {
                    std::string mHead = missionTitle.substr(0, mSep);
                    std::string mTail = missionTitle.substr(mSep);
                    float mHeadW = Ui::textWidth(menuFont, mHead, 22, true);
                    Ui::text(window, menuFont, mHead, 22, sf::Color(255, 210, 40), infoX + 28.f, infoY + 26.f, true, &uiCache);
                    Ui::text(window, menuFont, mTail, 22, sf::Color::White, infoX + 28.f + mHeadW, infoY + 26.f, true, &uiCache);
                } else {
                    Ui::text(window, menuFont, missionTitle, 22, sf::Color::White, infoX + 28.f, infoY + 26.f, true, &uiCache);
                }
            }
            Ui::textWrap(window, menuFont, lvlDescs[selectedActIdx][selectedLevelInAct], 15, sf::Color(220, 220, 220),
                         infoX + 28.f, infoY + 72.f, infoW - 56.f, false, &uiCache);

            int selLi = selectedActIdx * 5 + selectedLevelInAct;
            if (levels[selLi].unlocked) {
                Ui::text(window, menuFont, "[Click node or press Enter to launch]", 13, mapBright,
                         infoX + 28.f, infoY + infoH - 34.f, false, &uiCache);
            } else {
                Ui::text(window, menuFont, "[LOCKED - Clear previous level to unlock]", 13, mapDim,
                         infoX + 28.f, infoY + infoH - 34.f, false, &uiCache);
            }

            // Bottom status bar
            sf::RectangleShape bottomLine({static_cast<float>(W) - 32.f, 1.f});
            bottomLine.setPosition({16.f, static_cast<float>(H) - 64.f});
            bottomLine.setFillColor(mapBright);
            window.draw(bottomLine);

            sf::RectangleShape blSq({10.f, 10.f});
            blSq.setPosition({32.f, static_cast<float>(H) - 44.f});
            blSq.setFillColor(mapBright);
            window.draw(blSq);

            Ui::text(window, menuFont, "SELECT A ACT | NAVIGATE WITH ARROWS", 14, sf::Color::White,
                     58.f, static_cast<float>(H) - 48.f, false, &uiCache);

            // Demo unlock button centered in bottom status bar
            float demoBtnX = static_cast<float>(W) * 0.5f - 135.f;
            float demoBtnY = static_cast<float>(H) - 54.f;
            float demoBtnW = 270.f, demoBtnH = 34.f;
            bool demoHov = Ui::inside(mapIn.mx, mapIn.my, demoBtnX, demoBtnY, demoBtnW, demoBtnH);
            sf::RectangleShape demoBox({demoBtnW, demoBtnH});
            demoBox.setPosition({demoBtnX, demoBtnY});
            demoBox.setFillColor(demoHov ? sf::Color(12, 36, 18, 230) : sf::Color(6, 16, 10, 200));
            demoBox.setOutlineColor(demoHov ? mapBright : sf::Color(0, 255, 65, 110));
            demoBox.setOutlineThickness(1.f);
            window.draw(demoBox);
            Ui::textCenter(window, menuFont,
                           demoAllUnlocked ? "Demo - Lock All Levels" : "Demo - Unlock All Levels",
                           14, demoHov ? sf::Color::White : sf::Color::White,
                           static_cast<float>(W) * 0.5f, demoBtnY + 8.f, false, &uiCache);

            // Right progress counter
            int actCleared = 0;
            for (int i = 0; i < 5; ++i) {
                if (levels[selectedActIdx * 5 + i].stars > 0) ++actCleared;
            }
            std::string progStr = "PROGRESS: " + std::to_string(actCleared) + " / 5";
            float progW = Ui::textWidth(menuFont, progStr, 14, false);
            Ui::text(window, menuFont, progStr, 14, sf::Color::White,
                     static_cast<float>(W) - 36.f - progW, static_cast<float>(H) - 48.f, false, &uiCache);

            if (demoPromptOpen) {
                sf::RectangleShape dim({static_cast<float>(W), static_cast<float>(H)});
                dim.setFillColor(sf::Color(4, 10, 6, 195));
                window.draw(dim);

                float mw = 460.f, mh = 210.f;
                float mx0 = (static_cast<float>(W) - mw) * 0.5f;
                float my0 = (static_cast<float>(H) - mh) * 0.5f;

                sf::RectangleShape modalBox({mw, mh});
                modalBox.setPosition({mx0, my0});
                modalBox.setFillColor(sf::Color(5, 14, 8, 244));
                modalBox.setOutlineColor(mapBright);
                modalBox.setOutlineThickness(2.f);
                window.draw(modalBox);

                // Corner accents
                const float cLen = 14.f, cThk = 3.f;
                sf::RectangleShape c1({cLen, cThk}); c1.setPosition({mx0 - 1.f, my0 - 1.f}); c1.setFillColor(mapBright); window.draw(c1);
                sf::RectangleShape c2({cThk, cLen}); c2.setPosition({mx0 - 1.f, my0 - 1.f}); c2.setFillColor(mapBright); window.draw(c2);
                sf::RectangleShape c3({cLen, cThk}); c3.setPosition({mx0 + mw - cLen + 1.f, my0 - 1.f}); c3.setFillColor(mapBright); window.draw(c3);
                sf::RectangleShape c4({cThk, cLen}); c4.setPosition({mx0 + mw - cThk + 1.f, my0 - 1.f}); c4.setFillColor(mapBright); window.draw(c4);
                sf::RectangleShape c5({cLen, cThk}); c5.setPosition({mx0 - 1.f, my0 + mh - cThk + 1.f}); c5.setFillColor(mapBright); window.draw(c5);
                sf::RectangleShape c6({cThk, cLen}); c6.setPosition({mx0 - 1.f, my0 + mh - cLen + 1.f}); c6.setFillColor(mapBright); window.draw(c6);
                sf::RectangleShape c7({cLen, cThk}); c7.setPosition({mx0 + mw - cLen + 1.f, my0 + mh - cThk + 1.f}); c7.setFillColor(mapBright); window.draw(c7);
                sf::RectangleShape c8({cThk, cLen}); c8.setPosition({mx0 + mw - cThk + 1.f, my0 + mh - cLen + 1.f}); c8.setFillColor(mapBright); window.draw(c8);

                Ui::textCenter(window, menuFont,
                               demoAllUnlocked ? "LOCK ALL LEVELS (DEMO)" : "UNLOCK ALL LEVELS (DEMO)",
                               20, mapBright, mx0 + mw * 0.5f, my0 + 18.f, true, &uiCache);
                Ui::textCenter(window, menuFont, "Enter demo password.", 14, sf::Color::White,
                               mx0 + mw * 0.5f, my0 + 52.f, false, &uiCache);

                float ix = mx0 + 36.f, iy = my0 + 86.f, iw = mw - 72.f, ih = 42.f;
                sf::RectangleShape inputBox({iw, ih});
                inputBox.setPosition({ix, iy});
                inputBox.setFillColor(sf::Color(3, 10, 5));
                inputBox.setOutlineColor(mapBright);
                inputBox.setOutlineThickness(1.5f);
                window.draw(inputBox);

                char maskBuf[34];
                for (int k = 0; k < demoPasswordLen; ++k) maskBuf[k] = '*';
                bool showCursor = (static_cast<int>(uiAnimTimer * 2.5f) % 2 == 0);
                if (demoPasswordLen < 32 && showCursor) {
                    maskBuf[demoPasswordLen] = '_';
                    maskBuf[demoPasswordLen + 1] = '\0';
                } else {
                    maskBuf[demoPasswordLen] = '\0';
                }
                Ui::text(window, menuFont, maskBuf, 18, mapBright, ix + 14.f, iy + 9.f, false, &uiCache);

                bool subHov = Ui::inside(in.mx, in.my, mx0 + 36.f, my0 + 148.f, 180.f, 38.f);
                sf::RectangleShape subBox({180.f, 38.f});
                subBox.setPosition({mx0 + 36.f, my0 + 148.f});
                subBox.setFillColor(subHov ? sf::Color(0, 255, 65) : sf::Color(10, 72, 26));
                subBox.setOutlineColor(mapBright);
                subBox.setOutlineThickness(2.f);
                window.draw(subBox);
                Ui::textCenter(window, menuFont, "Submit [Enter]", 14,
                               subHov ? sf::Color(4, 14, 6) : sf::Color::White,
                               mx0 + 36.f + 90.f, my0 + 158.f, true, &uiCache);

                bool canHov = Ui::inside(in.mx, in.my, mx0 + mw - 216.f, my0 + 148.f, 180.f, 38.f);
                sf::RectangleShape canBox({180.f, 38.f});
                canBox.setPosition({mx0 + mw - 216.f, my0 + 148.f});
                canBox.setFillColor(canHov ? sf::Color(14, 38, 20) : sf::Color(5, 16, 8));
                canBox.setOutlineColor(canHov ? mapBright : mapDim);
                canBox.setOutlineThickness(1.5f);
                window.draw(canBox);
                Ui::textCenter(window, menuFont, "Cancel [Esc]", 14,
                               canHov ? sf::Color::White : mapSubtle,
                               mx0 + mw - 216.f + 90.f, my0 + 158.f, false, &uiCache);
            }
        }

        // 3. LEVEL PLAY SCREEN
        if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            if (missionBgSprite.has_value()) {
                window.draw(*missionBgSprite);
            }
            int lid = levels[currentLevelIdx].id;
            int alarmCap = levels[currentLevelIdx].alarmCapacity;

            if (cachedHudLid != lid) {
                cachedHudLid = lid;
                cachedHudObj = "Objective: " + levels[currentLevelIdx].objective;
                cachedHudCtrlLine = levels[currentLevelIdx].controls +
                                    "  |  [H] Hint  |  [Z] Undo  |  [Space] Play/Pause  |  [Right] Step  |  [Esc] Map";
            }
            if (cachedHudAlarm != alarmSteps || cachedHudCap != alarmCap || cachedHudScore != score) {
                cachedHudAlarm = alarmSteps;
                cachedHudCap = alarmCap;
                cachedHudScore = score;
                cachedHudAlarmStr = "Alarm: " + std::to_string(alarmSteps) + " / " + std::to_string(alarmCap);
                cachedHudScoreStr = "Score: " + std::to_string(score);
            }

            // Top HUD bar (x=30..1570)
            Ui::textFit(window, menuFont, levels[currentLevelIdx].title, 24, sf::Color(255, 210, 40), 30.f, 10.f, 675.f, true, &menuUiCache);
            Ui::textWrap(window, font, cachedHudObj, 12, sf::Color(215, 240, 220), 30.f, 42.f, 675.f, false, &uiCache);

            Ui::button(window, font, hintSys.isAvailable() ? "Hint [H]" : "Hint Used", 722.f, 18.f, 115.f, 34.f,
                       hintSys.isAvailable() ? BTN_PRIMARY : BTN_DISABLED, in, 13, &uiCache);
            Ui::button(window, font, "Undo [Z]", 850.f, 18.f, 115.f, 34.f,
                       undoStack.isEmpty() ? BTN_DISABLED : BTN_GHOST, in, 13, &uiCache);
            Ui::button(window, font, "Map [Esc]", 978.f, 18.f, 115.f, 34.f, BTN_GHOST, in, 13, &uiCache);

            float meterX = 1120.f, meterW = 450.f;
            sf::RectangleShape& meterRect = uiCache.rect;
            meterRect.setSize({meterW, 20.f});
            meterRect.setOrigin({0.f, 0.f});
            meterRect.setPosition({meterX, 16.f});
            meterRect.setFillColor(sf::Color(6, 18, 10));
            meterRect.setOutlineThickness(1.5f);
            meterRect.setOutlineColor(sf::Color(0, 255, 65, 160));
            window.draw(meterRect);

            float ratio = minFloat(1.0f, static_cast<float>(alarmSteps) / static_cast<float>(alarmCap));
            meterRect.setSize({meterW * ratio, 20.f});
            meterRect.setPosition({meterX, 16.f});
            meterRect.setOutlineThickness(0.f);
            meterRect.setFillColor(ratio > 0.8f ? Ui::red() : sf::Color(255, 210, 40));
            window.draw(meterRect);

            Ui::text(window, menuFont, cachedHudAlarmStr, 14, sf::Color(240, 90, 90), meterX, 44.f, true, &menuUiCache);
            float alarmStrW = Ui::textWidth(menuFont, cachedHudAlarmStr + "    ", 14, true);
            Ui::textFit(window, menuFont, cachedHudScoreStr, 14, sf::Color::White, meterX + alarmStrW, 44.f, meterW - alarmStrW, true, &menuUiCache);

            // Structure Canvas Frame (opaque interior so only canvas world grid shows)
            Ui::panel(window, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, sf::Color(55, 85, 130), 248, &uiCache);

            // Populate Highlight State
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.errorFlashNodeId = wrongFlashNodeId;
            hl.showBadges = showNodeBadges;
            for (int m = 0; m < markedCount; ++m) hl.addMarked(markedIds[m]);

            sf::Vector2f canvasHoverWorld(0.f, 0.f);
            bool inCanvasHover = !showingBriefing && mapHoverToCam(levelCam, in.mx, in.my, canvasHoverWorld);

            const LayoutResult* activeDrawLayout = nullptr;
            LayoutResult act2DimLayout;
            bool showBf = false, showId = false;

            if (lid >= LEVEL_1_1 && lid <= LEVEL_1_5) {
                if (lid == LEVEL_1_2 && !act1Reconstructed) {
                    layoutDirty = false;
                } else if (!animCtrl.hasEvents() && needLayout) {
                    layoutBtForScreen(false);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : btLayout;
                activeDrawLayout = &drawL;
                if (inCanvasHover && !(lid == LEVEL_1_2 && !act1Reconstructed)) {
                    int hovIdx = drawL.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
                }
                if (animCtrl.hasEvents()) {
                    animCtrl.fillHighlightState(hl);
                } else {
                    int hlIdx1 = drawL.findNodeIndexByKey(act1Hl1);
                    int hlIdx2 = drawL.findNodeIndexByKey(act1Hl2);
                    if (hlIdx1 >= 0) { hl.primaryNodeId = drawL.nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                    if (hlIdx2 >= 0) hl.secondaryNodeId = drawL.nodes[hlIdx2].id;
                }
                if (lid == LEVEL_1_3) hl.showBadges = false;
                showId = true;
            }
            else if (lid >= LEVEL_2_1 && lid <= LEVEL_2_5) {
                if (!animCtrl.hasEvents() && needLayout) {
                    layoutBstForScreen(false);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : bstLayout;
                activeDrawLayout = &drawL;
                if (lid == LEVEL_2_4 && act2PrunedCount > 0) {
                    act2DimLayout = drawL;
                    for (int di = 0; di < act2DimLayout.nodeCount; ++di) {
                        if (containsInt(act2PrunedKeys, act2PrunedCount, act2DimLayout.nodes[di].key)) {
                            act2DimLayout.nodes[di].alpha = 0.24f;
                        }
                    }
                    activeDrawLayout = &act2DimLayout;
                }
                if (inCanvasHover) {
                    int hovIdx = activeDrawLayout->hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = activeDrawLayout->nodes[hovIdx].id;
                }
                if (animCtrl.hasEvents() && (!animCtrl.isAtEnd() || animCtrl.isPlaying())) {
                    animCtrl.fillHighlightState(hl);
                } else {
                    int hlIdx1 = activeDrawLayout->findNodeIndexByKey(act2Hl1);
                    int hlIdx2 = activeDrawLayout->findNodeIndexByKey(act2Hl2);
                    if (hlIdx1 >= 0) { hl.primaryNodeId = activeDrawLayout->nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                    if (hlIdx2 >= 0) hl.secondaryNodeId = activeDrawLayout->nodes[hlIdx2].id;
                }
                hl.targetNodeId = -1;
                const LevelData& curLvlD = levelData[currentLevelIdx];
                if (lid == LEVEL_2_1 && act2TargetIdx >= 0 && act2TargetIdx < curLvlD.targetKeyCount) {
                    int tgtIdx = activeDrawLayout->findNodeIndexByKey(curLvlD.targetKeys[act2TargetIdx]);
                    if (tgtIdx >= 0) hl.targetNodeId = activeDrawLayout->nodes[tgtIdx].id;
                } else if (lid == LEVEL_2_5 && act2BossStage == 1) {
                    int tgtKey = (curLvlD.kQueryCount > 0) ? curLvlD.kQueries[0]
                               : (curLvlD.targetKeyCount > 0 ? curLvlD.targetKeys[curLvlD.targetKeyCount - 1] : -1);
                    if (tgtKey >= 0) {
                        int tgtIdx = activeDrawLayout->findNodeIndexByKey(tgtKey);
                        if (tgtIdx >= 0) hl.targetNodeId = activeDrawLayout->nodes[tgtIdx].id;
                    }
                }
                showId = true;
            }
            else if (lid >= LEVEL_3_1 && lid <= LEVEL_3_5) {
                if (!animCtrl.hasEvents() && needLayout) {
                    layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : avlLayout;
                activeDrawLayout = &drawL;
                if (inCanvasHover) {
                    int hovIdx = drawL.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
                }
                if (animCtrl.hasEvents()) animCtrl.fillHighlightState(hl);
                showBf = true;
                if (lid == LEVEL_3_5) hl.showBadges = true;
            }
            else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                bool useRealHeap = (lid == LEVEL_4_3 || lid == LEVEL_4_4 || (lid == LEVEL_4_5 && act4Built));
                if (!animCtrl.hasEvents() && needLayout) {
                    layoutHeapForScreen(useRealHeap, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : heapLayout;
                activeDrawLayout = &drawL;
                if (inCanvasHover) {
                    int hovIdx = drawL.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
                    else {
                        int cIdx = drawL.hitTestCell(canvasHoverWorld.x, canvasHoverWorld.y);
                        if (cIdx >= 0) hl.hoveredNodeId = cIdx;
                    }
                }
                if (animCtrl.hasEvents()) {
                    animCtrl.fillHighlightState(hl);
                } else {
                    hl.primaryNodeId = hlIndex1;
                    hl.secondaryNodeId = hlIndex2;
                    hl.arrayHl1 = hlIndex1;
                    hl.arrayHl2 = hlIndex2;
                    if (hlIndex1 >= 0) hl.primaryStyle = HL_VISIT;
                }
            }

            if (activeDrawLayout) {
                window.setView(levelCam.view);
                if (showBgGrid) guiRenderer.drawCanvasGrid(window, levelCam.view);
                if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                    guiRenderer.drawHeapLayout(window, font, *activeDrawLayout, hl, CANVAS_X + 14.f, CANVAS_Y + CANVAS_H - 80.f);
                } else if (lid == LEVEL_1_2 && !act1Reconstructed) {
                    for (int ei = 0; ei < activeDrawLayout->edgeCount; ++ei) {
                        const EdgeLayout& e = activeDrawLayout->edges[ei];
                        sf::Vertex line[] = {
                            sf::Vertex({e.x1, e.y1}, sf::Color(85, 130, 190, 185)),
                            sf::Vertex({e.x2, e.y2}, sf::Color(85, 130, 190, 185))
                        };
                        window.draw(line, 2, sf::PrimitiveType::Lines);
                    }
                    DragDropView::draw(window, font, dragDrop, &uiCache);
                } else {
                    guiRenderer.drawTreeLayout(window, font, *activeDrawLayout, hl, showBf, showId);
                    if (lid == LEVEL_1_3 && act1PostIdx > 0) {
                        int postCnt = 0;
                        int* postSeq = act1Tree.postorderRecursive(postCnt);
                        if (postSeq) {
                            for (int pi = 0; pi < act1PostIdx && pi < postCnt; ++pi) {
                                int nIdx = activeDrawLayout->findNodeIndexByKey(postSeq[pi]);
                                if (nIdx >= 0) {
                                    const NodeLayout& nd = activeDrawLayout->nodes[nIdx];
                                    float bx = nd.x + nd.radius - 6.f;
                                    float by = nd.y - nd.radius - 10.f;
                                    sf::RectangleShape& pill = uiCache.rect;
                                    pill.setSize({32.f, 18.f});
                                    pill.setOrigin({0.f, 0.f});
                                    pill.setPosition({bx, by});
                                    pill.setFillColor(sf::Color(20, 75, 50, 235));
                                    pill.setOutlineThickness(1.2f);
                                    pill.setOutlineColor(Ui::green());
                                    window.draw(pill);
                                    Ui::textCenter(window, font, "h=" + std::to_string(act1ConfirmedHeight[pi]),
                                                   11, Ui::green(), bx + 16.f, by + 2.f, true, &uiCache);
                                }
                            }
                            delete[] postSeq;
                        }
                    }
                }
                if ((lid != LEVEL_1_2 || act1Reconstructed) && (dragDrop.tokenCount() > 0 || dragDrop.slotCount() > 0)) {
                    DragDropView::draw(window, font, dragDrop, &uiCache);
                }
                window.setView(window.getDefaultView());

                Ui::text(window, font, levelCam.zoomText(), 11, Ui::textMuted(),
                         CANVAS_X + 12.f, CANVAS_Y + 6.f, false, &uiCache);
                if (lid != LEVEL_1_3 && !(lid == LEVEL_1_2 && !act1Reconstructed)) {
                    drawCamTooltip(levelCam, *activeDrawLayout, hl.hoveredNodeId);
                }
            }

            // Event Log + Playback Controls Panel (right column)
            guiRenderer.drawEventLogPanel(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            // Bottom Guidance & Minimal Structural Controls Panel
            Ui::panel(window, 30.f, 660.f, 1540.f, 210.f, sf::Color(60, 85, 125), 235, &uiCache);

            int act2PromptA = -9999, act2PromptB = -9999;
            int act3PromptA = -9999, act3PromptB = -9999;
            if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
                act2PromptA = act2CurKey;
                act2PromptB = act2TargetIdx * 10 + act2BossStage;
            } else if (lid == LEVEL_2_3) {
                act2PromptA = act2DelStage;
                act2PromptB = act2Bst.getSize();
            } else if (lid == LEVEL_2_4) {
                act2PromptA = act2RqIdx;
                act2PromptB = act2PrunedCount;
            } else if (lid == LEVEL_3_1) {
                act3PromptA = clicked31Mask;
            } else if (lid == LEVEL_3_2) {
                act3PromptA = manualSubPuzzle;
                act3PromptB = act3PivotPicked ? act3PivotKey : -1;
            } else if (lid == LEVEL_3_3) {
                act3PromptA = manualSubPuzzle;
                act3PromptB = manualDoubleStep;
            } else if (lid == LEVEL_3_5) {
                act3PromptA = bossQueryIdx;
                act3PromptB = avl.getSize();
            } else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                act3PromptA = scoring.steps();
                act3PromptB = sortedCount * 10 + (act4Built ? 1 : 0);
            }

            if (cachedPromptLid != lid || cachedPromptSeq != act1SeqIdx || cachedPromptAvl != avlQueuePos ||
                cachedPromptMode != act1TravMode || cachedPromptPost != act1PostIdx || cachedPromptLca != act1LcaStage ||
                cachedPromptAct2A != act2PromptA || cachedPromptAct2B != act2PromptB ||
                cachedPromptAct3A != act3PromptA || cachedPromptAct3B != act3PromptB) {
                cachedPromptLid = lid;
                cachedPromptSeq = act1SeqIdx;
                cachedPromptAvl = avlQueuePos;
                cachedPromptMode = act1TravMode;
                cachedPromptPost = act1PostIdx;
                cachedPromptLca = act1LcaStage;
                cachedPromptAct2A = act2PromptA;
                cachedPromptAct2B = act2PromptB;
                cachedPromptAct3A = act3PromptA;
                cachedPromptAct3B = act3PromptB;
                cachedPromptHead = "NODE INTERACTION GUIDE";
                const LevelData& curD = levelData[currentLevelIdx];
                if (lid == LEVEL_3_4 && avlQueuePos < curD.insertCount) {
                    cachedPromptHead = "INCOMING KEY [" + std::to_string(curD.inserts[avlQueuePos].key) +
                                       "] - Predict rotation (LL / RR / LR / RL / None):";
                } else if (lid == LEVEL_3_1) {
                    cachedPromptHead = "READ THE BADGES - Click every room with |BF| > 1 (height(L) - height(R)):";
                } else if (lid == LEVEL_3_2) {
                    if (!act3PivotPicked) {
                        cachedPromptHead = "SINGLE ROTATION [" + std::to_string(manualSubPuzzle) + "/" +
                                           std::to_string(curD.puzzleCount) + "] - Step 1/2: Click unbalanced pivot (|BF| > 1):";
                    } else {
                        cachedPromptHead = "PIVOT [" + std::to_string(act3PivotKey) +
                                           "] SELECTED - Step 2/2: Pick [LL: Rotate Right] or [RR: Rotate Left]:";
                    }
                } else if (lid == LEVEL_3_3) {
                    if (manualDoubleStep == 0) {
                        cachedPromptHead = "DOUBLE ROTATION [" + std::to_string(manualSubPuzzle) + "/" +
                                           std::to_string(curD.puzzleCount) + "] - Step 1/3: Pick [Case: LR] or [Case: RL]:";
                    } else if (manualDoubleStep == 1) {
                        cachedPromptHead = "DOUBLE ROTATION - Step 2/3: Click heavy CHILD pivot node first:";
                    } else {
                        cachedPromptHead = "DOUBLE ROTATION - Step 3/3: Click ROOT pivot node second:";
                    }
                } else if (lid == LEVEL_3_5 && bossQueryIdx < curD.kQueryCount) {
                    cachedPromptHead = "ORDER-STATISTIC BOSS [" + std::to_string(bossQueryIdx + 1) + "/" +
                                       std::to_string(curD.kQueryCount) + "] - Click #" +
                                       std::to_string(curD.kQueries[bossQueryIdx]) + " MOST VALUABLE (LARGEST) via sz";
                    if (act3LastComps > 0) {
                        cachedPromptHead += " | Last: " + std::to_string(act3LastComps) + " comps <= H=" +
                                            std::to_string(act3LastHeight) + " (sz " +
                                            std::string(act3SizeOk ? "OK" : "ERR") + ")";
                    }
                    cachedPromptHead += ":";
                } else if (lid == LEVEL_1_1) {
                    cachedPromptHead = std::string(traversalModeName(act1TravMode)) + " [" +
                                       std::to_string(act1SeqIdx) + "/" + std::to_string(act1Tree.getSize()) +
                                       "] - Click rooms in exact order:";
                } else if (lid == LEVEL_1_2) {
                    std::string preS = "PRE: ", inS = "IN: ";
                    for (int i = 0; i < curD.preorderCount; ++i) preS += std::to_string(curD.preorder[i]) + (i + 1 < curD.preorderCount ? " " : "");
                    for (int i = 0; i < curD.inorderCount; ++i)  inS += std::to_string(curD.inorder[i])  + (i + 1 < curD.inorderCount ? " " : "");
                    cachedPromptHead = preS + "   |   " + inS;
                } else if (lid == LEVEL_1_3) {
                    if (act1PostIdx < act1Tree.getSize()) {
                        cachedPromptHead = "POSTORDER HEIGHTS [" + std::to_string(act1PostIdx) + "/" +
                                           std::to_string(act1Tree.getSize()) + "] - Enter height of room " +
                                           std::to_string(act1Hl1) + " (leaf = 1):";
                    } else {
                        cachedPromptHead = "FINAL CHECK - Enter the tree's Diameter (longest path in nodes):";
                    }
                } else if (lid == LEVEL_1_4) {
                    if (act1LcaStage == 0) cachedPromptHead = "STEP 1/3 - Click Room A (any start room on the tree):";
                    else if (act1LcaStage == 1) cachedPromptHead = "STEP 1/3 - Room A=" + std::to_string(act1LcaA) + " picked; click Room B:";
                    else if (act1LcaStage == 2) cachedPromptHead = "STEP 2/3 - Click the Lowest Common Ancestor (LCA) of " + std::to_string(act1LcaA) + " & " + std::to_string(act1LcaB) + ":";
                    else cachedPromptHead = "STEP 3/3 - Trace the route between " + std::to_string(act1LcaA) + " & " + std::to_string(act1LcaB) + " in order:";
                } else if (lid == LEVEL_1_5) {
                    cachedPromptHead = "MIRROR THE VAULT - Click internal rooms bottom-up to swap children, or press Auto:";
                } else if (lid == LEVEL_2_1) {
                    int tgt = (act2TargetIdx < curD.targetKeyCount) ? curD.targetKeys[act2TargetIdx] : -1;
                    cachedPromptHead = "SEARCH HEIST - Target [" + std::to_string(tgt) + "]  |  Room [" +
                                       std::to_string(act2CurKey) + "] - Compare & pick Left / Found / Right:";
                } else if (lid == LEVEL_2_2) {
                    int insK = (act2TargetIdx < curD.targetKeyCount) ? curD.targetKeys[act2TargetIdx] : -1;
                    cachedPromptHead = "STASH THE LOOT [" + std::to_string(minInt(act2TargetIdx + 1, curD.targetKeyCount)) +
                                       "/" + std::to_string(curD.targetKeyCount) + "] - Key [" + std::to_string(insK) +
                                       "]  |  Room [" + std::to_string(act2CurKey) + "]:";
                } else if (lid == LEVEL_2_3) {
                    int delK = (act2DelStage < curD.deleteSequenceCount) ? curD.deleteSequence[act2DelStage] : -1;
                    cachedPromptHead = "STEAL & COLLAPSE [" + std::to_string(minInt(act2DelStage + 1, curD.deleteSequenceCount)) +
                                       "/" + std::to_string(curD.deleteSequenceCount) + "] - Room [" +
                                       (delK >= 0 ? std::to_string(delK) : std::string("Done")) + "] - Pick Case:";
                } else if (lid == LEVEL_2_4) {
                    int curR = (act2RqIdx < act2RqCount) ? act2RqSteps[act2RqIdx].nodeKey : -1;
                    cachedPromptHead = "RANGE RAID [" + std::to_string(curD.rangeLo) + ".." + std::to_string(curD.rangeHi) +
                                       "] (" + std::to_string(act2RqIdx) + "/" + std::to_string(act2RqCount) +
                                       ") - Room [" + (curR >= 0 ? std::to_string(curR) : std::string("Done")) +
                                       "]  |  Pruned: " + std::to_string(act2PrunedCount);
                } else if (lid == LEVEL_2_5) {
                    if (act2BossStage == 0) {
                        int insK = (act2TargetIdx < curD.targetKeyCount) ? curD.targetKeys[act2TargetIdx] : -1;
                        cachedPromptHead = "BOSS: LAZY ARCHITECT - Insert [" + std::to_string(minInt(act2TargetIdx + 1, curD.targetKeyCount)) +
                                           "/" + std::to_string(curD.targetKeyCount) + "] Key [" + std::to_string(insK) +
                                           "]  |  Room [" + std::to_string(act2CurKey) + "]  |  Height=" + std::to_string(act2Bst.getHeight());
                    } else {
                        int sk = (curD.kQueryCount > 0) ? curD.kQueries[0] : -1;
                        cachedPromptHead = "BOSS STAGE 2 - Search Key [" + std::to_string(sk) +
                                           "] down Height-" + std::to_string(act2Bst.getHeight()) +
                                           " Chain  |  Room [" + std::to_string(act2CurKey) + "]:";
                    }
                } else if (lid == LEVEL_4_1) {
                    cachedPromptHead = "ACTIVATE THE VAULT - Click bottom-up violation [" +
                                       std::to_string(maxInt(0, buildStepIdx)) + "] (or its larger child) to sift down:";
                } else if (lid == LEVEL_4_2) {
                    cachedPromptHead = "REPAIR SECURITY LOCKS - Click sabotaged root [0] (or larger child [1]):";
                } else if (lid == LEVEL_4_3) {
                    cachedPromptHead = "EXTRACT GREATEST [" + std::to_string(sortedCount) + "/" +
                                       std::to_string(curD.initialArrayCount) +
                                       "] - Click root [0] to fill ascending sorted tail:";
                } else if (lid == LEVEL_4_4) {
                    int optS = computeOptimalSteps(curD);
                    cachedPromptHead = "RACE AGAINST THE GUARDS - Steps: " + std::to_string(scoring.steps()) + "/" +
                                       std::to_string(optS) + " (" + std::to_string(optS) +
                                       " extractions, O(log n) each = Theta(n log n)):";
                } else if (lid == LEVEL_4_5) {
                    if (!act4Built) {
                        cachedPromptHead = "FINAL VAULT BOSS - Phase 1/2: Click [Build Max Heap [B]] to heapify:";
                    } else {
                        cachedPromptHead = "FINAL VAULT BOSS - Phase 2/2 [" + std::to_string(sortedCount) + "/" +
                                           std::to_string(curD.initialArrayCount) +
                                           "] - Click root [0] to fill ascending tail:";
                    }
                }
            }
            float headMaxW = (lid == LEVEL_3_4) ? 960.f : 1080.f;
            float bodyMaxW = (lid == LEVEL_3_4) ? 960.f : 1050.f;
            Ui::textFit(window, menuFont, cachedPromptHead, TXT_H2, sf::Color(255, 210, 40), 54.f, 678.f, headMaxW, true, &menuUiCache);
            Ui::textWrap(window, font, status, TXT_BODY, sf::Color::White, 54.f, 718.f, bodyMaxW, false, &uiCache);
            if (hintSys.wasUsed()) {
                Ui::textWrap(window, font, "HINT (-1 star cap): " + hintSys.shownText(),
                             TXT_HINT, sf::Color(0, 255, 65), 54.f, 774.f, bodyMaxW, false, &uiCache);
            }

            if (lid == LEVEL_1_1 && act1SeqIdx == 0 && !pendingWin) {
                Ui::button(window, font, "Cycle Mode [C]",
                           1220.f, 705.f, 310.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if (lid == LEVEL_1_3) {
                bool askingDiam = (act1PostIdx >= act1Tree.getSize());
                std::string valLabel = (askingDiam ? "Diam: " : "Height: ") + std::to_string(act1StepperVal);
                Ui::button(window, font, "-", 1130.f, 705.f, 60.f, 52.f, BTN_SECONDARY, in, 18, &uiCache);
                Ui::panel(window, 1200.f, 705.f, 150.f, 52.f, sf::Color(0, 255, 65), 240, &uiCache);
                Ui::textCenter(window, font, valLabel, 17, sf::Color(0, 255, 65), 1275.f, 719.f, true, &uiCache);
                Ui::button(window, font, "+", 1360.f, 705.f, 60.f, 52.f, BTN_SECONDARY, in, 18, &uiCache);
                Ui::button(window, font, "Submit", 1430.f, 705.f, 120.f, 52.f, BTN_PRIMARY, in, 15, &uiCache);
            } else if (lid == LEVEL_1_5 && !act1Mirrored && !pendingWin) {
                Ui::button(window, font, "Auto Mirror [A]",
                           1220.f, 705.f, 310.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if ((lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) && !pendingWin) {
                bool isSearchMode = (lid == LEVEL_2_1) || (lid == LEVEL_2_5 && act2BossStage == 1);
                const char* midLabel = isSearchMode ? "= Found" : "= Reject Dup";
                Ui::button(window, font, "< Go Left",  1122.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, midLabel,     1268.f, 705.f, 136.f, 52.f, BTN_PRIMARY,   in, 14, &uiCache);
                Ui::button(window, font, "Go Right >", 1414.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
            } else if (lid == LEVEL_2_3 && !pendingWin) {
                Ui::button(window, font, "Leaf",         1122.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "One-Child",    1268.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "Two-Children", 1414.f, 705.f, 136.f, 52.f, BTN_PRIMARY,   in, 13, &uiCache);
            } else if (lid == LEVEL_2_4 && !pendingWin) {
                Ui::button(window, font, "Report Room", 1122.f, 705.f, 136.f, 52.f, BTN_PRIMARY,   in, 14, &uiCache);
                Ui::button(window, font, "Prune Left",  1268.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "Prune Right", 1414.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
            } else if (lid == LEVEL_3_2 && !pendingWin) {
                Ui::button(window, font, "LL: Rotate Right (1)", 1122.f, 705.f, 206.f, 52.f,
                           act3PivotPicked ? BTN_PRIMARY : BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "RR: Rotate Left (2)",  1344.f, 705.f, 206.f, 52.f,
                           act3PivotPicked ? BTN_PRIMARY : BTN_SECONDARY, in, 14, &uiCache);
            } else if (lid == LEVEL_3_3 && manualDoubleStep == 0 && !pendingWin) {
                Ui::button(window, font, "Case: LR (1)", 1122.f, 705.f, 206.f, 52.f, BTN_PRIMARY,   in, 15, &uiCache);
                Ui::button(window, font, "Case: RL (2)", 1344.f, 705.f, 206.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if (lid == LEVEL_3_4 && !pendingWin) {
                Ui::button(window, font, "LL (1)",   1038.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "RR (2)",   1142.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "LR (3)",   1246.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "RL (4)",   1350.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "None (N)", 1454.f, 705.f, 96.f, 52.f, BTN_PRIMARY,   in, 14, &uiCache);
            } else if (lid == LEVEL_4_5 && !act4Built) {
                Ui::button(window, font, "Build Max Heap [B]",
                           1220.f, 705.f, 310.f, 52.f, BTN_PRIMARY, in, 15, &uiCache);
            }

            Ui::textFit(window, font, cachedHudCtrlLine, TXT_HINT, Ui::textMuted(), 54.f, 832.f, 1480.f, false, &uiCache);

            // Mission Briefing Modal (Part B)
            if (showingBriefing) {
                if (vaultBgSprite.has_value()) {
                    window.draw(*vaultBgSprite);
                }
                sf::RectangleShape& bd = uiCache.rect;
                bd.setSize({static_cast<float>(W), static_cast<float>(H)});
                bd.setOrigin({0.f, 0.f});
                bd.setPosition({0.f, 0.f});
                bd.setOutlineThickness(0.f);
                bd.setFillColor(sf::Color(4, 12, 7, 175));
                window.draw(bd);

                if (cachedBriefLid != lid || cachedBriefCap != alarmCap) {
                    cachedBriefLid = lid;
                    cachedBriefCap = alarmCap;
                    cachedBriefAct = "MISSION BRIEFING // ACT " + std::to_string(levels[currentLevelIdx].act);
                    cachedBriefObj = levels[currentLevelIdx].objective;
                    cachedBriefInt = levels[currentLevelIdx].controls;
                }

                const sf::Color brBright(0, 255, 65);
                float bw = 820.f, bh = 420.f;
                float bx = static_cast<float>(W) * 0.5f - bw * 0.5f;
                float by = static_cast<float>(H) * 0.5f - bh * 0.5f;

                // Layered glow + dark terminal panel
                Ui::panel(window, bx, by, bw, bh, brBright, 236, &uiCache);

                // Cut/notched corner accents
                const float cLen = 18.f, cThk = 3.5f;
                sf::RectangleShape bc1({cLen, cThk}); bc1.setPosition({bx - 1.f, by - 1.f}); bc1.setFillColor(brBright); window.draw(bc1);
                sf::RectangleShape bc2({cThk, cLen}); bc2.setPosition({bx - 1.f, by - 1.f}); bc2.setFillColor(brBright); window.draw(bc2);
                sf::RectangleShape bc3({cLen, cThk}); bc3.setPosition({bx + bw - cLen + 1.f, by - 1.f}); bc3.setFillColor(brBright); window.draw(bc3);
                sf::RectangleShape bc4({cThk, cLen}); bc4.setPosition({bx + bw - cThk + 1.f, by - 1.f}); bc4.setFillColor(brBright); window.draw(bc4);
                sf::RectangleShape bc5({cLen, cThk}); bc5.setPosition({bx - 1.f, by + bh - cThk + 1.f}); bc5.setFillColor(brBright); window.draw(bc5);
                sf::RectangleShape bc6({cThk, cLen}); bc6.setPosition({bx - 1.f, by + bh - cLen + 1.f}); bc6.setFillColor(brBright); window.draw(bc6);
                sf::RectangleShape bc7({cLen, cThk}); bc7.setPosition({bx + bw - cLen + 1.f, by + bh - cThk + 1.f}); bc7.setFillColor(brBright); window.draw(bc7);
                sf::RectangleShape bc8({cThk, cLen}); bc8.setPosition({bx + bw - cThk + 1.f, by + bh - cLen + 1.f}); bc8.setFillColor(brBright); window.draw(bc8);

                // Header + extending thin green line
                Ui::text(window, menuFont, cachedBriefAct, 14, brBright, bx + 40.f, by + 26.f, true, &uiCache);
                float hdrW = Ui::textWidth(menuFont, cachedBriefAct, 14, true);
                float lineX = bx + 54.f + hdrW;
                if (lineX < bx + bw - 40.f) {
                    sf::RectangleShape hdrLine({(bx + bw - 40.f) - lineX, 1.f});
                    hdrLine.setPosition({lineX, by + 35.f});
                    hdrLine.setFillColor(sf::Color(0, 255, 65, 160));
                    window.draw(hdrLine);
                }

                // Level title
                Ui::textFit(window, menuFont, levels[currentLevelIdx].title, 32, brBright,
                            bx + 40.f, by + 58.f, bw - 80.f, true, &uiCache);

                // Row 1: Target icon + OBJECTIVE: + objective text
                float r1Y = by + 132.f;
                float iconX = bx + 50.f;
                sf::CircleShape tgtOuter(10.f);
                tgtOuter.setOrigin({10.f, 10.f});
                tgtOuter.setPosition({iconX, r1Y + 10.f});
                tgtOuter.setFillColor(sf::Color::Transparent);
                tgtOuter.setOutlineColor(brBright);
                tgtOuter.setOutlineThickness(2.f);
                window.draw(tgtOuter);
                sf::CircleShape tgtDot(3.5f);
                tgtDot.setOrigin({3.5f, 3.5f});
                tgtDot.setPosition({iconX, r1Y + 10.f});
                tgtDot.setFillColor(brBright);
                window.draw(tgtDot);

                Ui::text(window, menuFont, "OBJECTIVE:", 15, sf::Color(255, 210, 40), bx + 76.f, r1Y, true, &uiCache);
                float objH = Ui::textWrap(window, font, cachedBriefObj, 15, sf::Color::White,
                                          bx + 216.f, r1Y, bw - 256.f, false, &uiCache);

                // Divider line
                float divY = r1Y + ((objH > 26.f) ? objH + 14.f : 38.f);
                sf::RectangleShape divLine({bw - 80.f, 1.f});
                divLine.setPosition({bx + 40.f, divY});
                divLine.setFillColor(sf::Color(0, 255, 65, 90));
                window.draw(divLine);

                // Row 2: Info icon + INTERACTION: + controls text
                float r2Y = divY + 20.f;
                sf::CircleShape infoCirc(10.f);
                infoCirc.setOrigin({10.f, 10.f});
                infoCirc.setPosition({iconX, r2Y + 10.f});
                infoCirc.setFillColor(brBright);
                window.draw(infoCirc);
                Ui::textCenter(window, menuFont, "i", 14, sf::Color(4, 14, 6), iconX, r2Y + 1.f, true, &uiCache);
                Ui::text(window, menuFont, "INTERACTION:", 15, sf::Color(255, 210, 40), bx + 76.f, r2Y, true, &uiCache);
                float intH = Ui::textWrap(window, font, cachedBriefInt, 15, sf::Color::White,
                                          bx + 216.f, r2Y, bw - 256.f, false, &uiCache);

                // Row 3: Warning icon + ALARM CAPACITY: + capacity strikes
                float r3Y = r2Y + ((intH > 26.f) ? intH + 16.f : 42.f);
                sf::ConvexShape warnTri(3);
                warnTri.setPoint(0, {iconX, r3Y});
                warnTri.setPoint(1, {iconX - 10.f, r3Y + 19.f});
                warnTri.setPoint(2, {iconX + 10.f, r3Y + 19.f});
                warnTri.setFillColor(brBright);
                window.draw(warnTri);
                Ui::textCenter(window, menuFont, "!", 13, sf::Color(4, 14, 6), iconX, r3Y + 3.f, true, &uiCache);
                Ui::text(window, menuFont, "ALARM CAPACITY:", 15, sf::Color(255, 210, 40), bx + 76.f, r3Y, true, &uiCache);
                float alarmLblW = Ui::textWidth(menuFont, "ALARM CAPACITY:", 15, true);
                Ui::text(window, font, std::to_string(alarmCap) + " strikes", 15, sf::Color::White,
                         bx + 90.f + alarmLblW, r3Y, false, &uiCache);

                Ui::button(window, menuFont, "START MISSION [Enter]", static_cast<float>(W) * 0.5f - 150.f,
                           by + bh - 74.f, 300.f, 48.f, BTN_PRIMARY, in, 16, &uiCache);
            }
        }

        // 4. RESULT SCREEN
        if (currentScreen == SCREEN_RESULT) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            bool failed = (currentLevelIdx >= 0 && levels[currentLevelIdx].stars == 0);
            float cardW = 760.f, cardH = 520.f;
            float cardX = W * 0.5f - cardW * 0.5f, cardY = 170.f;
            Ui::panel(window, cardX, cardY, cardW, cardH, failed ? Ui::red() : sf::Color(0, 255, 65), 240, &uiCache);

            Ui::textCenter(window, font,
                           failed ? "ALARM TRIGGERED - LEVEL FAILED" : "LEVEL COMPLETE",
                           36, failed ? Ui::red() : sf::Color(0, 255, 65), W * 0.5f, cardY + 34.f, true, &uiCache);

            if (currentLevelIdx >= 0) {
                int bc = animCtrl.getLog().getBatchCount();
                int ec = animCtrl.getLog().getTotalEventCount();
                int st = levels[currentLevelIdx].stars;
                if (cachedResIdx != currentLevelIdx || cachedResScore != score || cachedResAlarm != alarmSteps ||
                    cachedResBatches != bc || cachedResEvs != ec || cachedResStars != st) {
                    cachedResIdx = currentLevelIdx;
                    cachedResScore = score;
                    cachedResAlarm = alarmSteps;
                    cachedResBatches = bc;
                    cachedResEvs = ec;
                    cachedResStars = st;
                    cachedResStats = "Score: " + std::to_string(score) +
                                     "   |   Alarm Steps: " + std::to_string(alarmSteps) +
                                     "   |   Ops Logged: " + std::to_string(bc) +
                                     " (" + std::to_string(ec) + " events)";
                }
                Ui::textCenter(window, font, levels[currentLevelIdx].title, TXT_H2, sf::Color::White,
                               W * 0.5f, cardY + 96.f, true, &uiCache);
                Ui::stars(window, W * 0.5f, cardY + 166.f, st, 3, 22.f, &uiCache);
                Ui::textCenter(window, font, cachedResStats, TXT_BODY, Ui::textBody(), W * 0.5f, cardY + 230.f, false, &uiCache);
            }

            Ui::textWrap(window, font, status, TXT_BODY,
                         failed ? sf::Color(255, 175, 175) : Ui::green(),
                         cardX + 60.f, cardY + 280.f, cardW - 120.f, false, &uiCache);

            Ui::button(window, font, "Retry Level [R]", W * 0.5f - 230.f, cardY + 365.f, 210.f, 52.f, BTN_SECONDARY, in, 16, &uiCache);
            Ui::button(window, font, "Next Level [N]",  W * 0.5f + 20.f,  cardY + 365.f, 210.f, 52.f, BTN_PRIMARY, in, 16, &uiCache);
            Ui::button(window, font, "Campaign Map [Esc]", W * 0.5f - 120.f, cardY + 436.f, 240.f, 42.f, BTN_GHOST, in, 14, &uiCache);
        }

        // 5. SANDBOX MODE
        if (currentScreen == SCREEN_SANDBOX) {
            if (missionBgSprite.has_value()) {
                window.draw(*missionBgSprite);
            }
            Ui::screenHeader(window, font, "SANDBOX SIMULATOR // AVL TREE + MAX HEAP",
                             "Interactive free-play over real AVLTree & MaxHeap instances with EventLog & Undo.", &uiCache);
            Ui::button(window, font, "Back to Menu [Esc]", 1370.f, 26.f, 190.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            Ui::panel(window, 30.f,  95.f, 560.f, 540.f, sf::Color(0, 255, 65), 248, &uiCache);
            Ui::panel(window, 605.f, 95.f, 565.f, 540.f, sf::Color(0, 255, 65), 248, &uiCache);

            if (!animCtrl.hasEvents() && needLayout) {
                layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
                layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
                layoutDirty = false;
            } else if (needLayout) {
                if (animCtrl.getRenderLayout().structureType != STRUCT_AVL) layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
                if (animCtrl.getRenderLayout().structureType != STRUCT_MAX_HEAP) layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
                layoutDirty = false;
            }
            bool animAvl  = (animCtrl.hasEvents() && animCtrl.getRenderLayout().structureType == STRUCT_AVL);
            bool animHeap = (animCtrl.hasEvents() && animCtrl.getRenderLayout().structureType == STRUCT_MAX_HEAP);
            const LayoutResult& sbAvlL  = animAvl  ? animCtrl.getRenderLayout() : avlLayout;
            const LayoutResult& sbHeapL = animHeap ? animCtrl.getRenderLayout() : heapLayout;

            RenderHighlightState sbAvlHl, sbHeapHl;
            sbAvlHl.showBadges  = showNodeBadges;
            sbHeapHl.showBadges = showNodeBadges;
            if (animAvl)  animCtrl.fillHighlightState(sbAvlHl);
            if (animHeap) animCtrl.fillHighlightState(sbHeapHl);
            sf::Vector2f wPosAvl(0.f, 0.f), wPosHeap(0.f, 0.f);
            if (mapHoverToCam(sbAvlCam, in.mx, in.my, wPosAvl)) {
                int sbAvlHov = sbAvlL.hitTestNode(wPosAvl.x, wPosAvl.y);
                if (sbAvlHov >= 0) sbAvlHl.hoveredNodeId = sbAvlL.nodes[sbAvlHov].id;
            }
            if (mapHoverToCam(sbHeapCam, in.mx, in.my, wPosHeap)) {
                int sbHeapHov = sbHeapL.hitTestNode(wPosHeap.x, wPosHeap.y);
                if (sbHeapHov >= 0) sbHeapHl.hoveredNodeId = sbHeapL.nodes[sbHeapHov].id;
                else {
                    int cIdx = sbHeapL.hitTestCell(wPosHeap.x, wPosHeap.y);
                    if (cIdx >= 0) sbHeapHl.hoveredNodeId = cIdx;
                }
            }

            window.setView(sbAvlCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, sbAvlCam.view);
            guiRenderer.drawTreeLayout(window, font, sbAvlL, sbAvlHl, true, false);

            window.setView(sbHeapCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, sbHeapCam.view);
            guiRenderer.drawHeapLayout(window, font, sbHeapL, sbHeapHl, 619.f, 555.f);

            window.setView(window.getDefaultView());
            Ui::text(window, font, sbAvlCam.zoomText(), 10, Ui::textMuted(), 40.f, 100.f, false, &uiCache);
            Ui::text(window, font, sbHeapCam.zoomText(), 10, Ui::textMuted(), 615.f, 100.f, false, &uiCache);
            drawCamTooltip(sbAvlCam, sbAvlL, sbAvlHl.hoveredNodeId);
            drawCamTooltip(sbHeapCam, sbHeapL, sbHeapHl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, 95.f, LOG_W, 540.f,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, 95.f, LOG_W, 540.f,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            Ui::panel(window, 30.f, 650.f, 1540.f, 220.f, sf::Color(60, 85, 125), 235, &uiCache);
            Ui::button(window, font, "[I] AVL Insert",   54.f,   674.f, 190.f, 46.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[D] AVL Delete",   264.f,  674.f, 190.f, 46.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[K] AVL kth",      474.f,  674.f, 190.f, 46.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[E] Heap Extract", 684.f,  674.f, 200.f, 46.f, BTN_PRIMARY, in, 14, &uiCache);
            Ui::button(window, font, "[Z] Undo",         904.f,  674.f, 170.f, 46.f, undoStack.isEmpty() ? BTN_DISABLED : BTN_GHOST, in, 14, &uiCache);
            Ui::button(window, font, "[R] Reset",        1094.f, 674.f, 170.f, 46.f, BTN_DANGER, in, 14, &uiCache);

            Ui::textFit(window, font, status, TXT_BODY, sf::Color::White, 54.f, 746.f, 1480.f, false, &uiCache);
            Ui::textFit(window, font, "Shortcuts: [I] Insert AVL  |  [D] Delete AVL  |  [K] kth  |  [E] Extract Heap  |  [Z] Undo  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 820.f, 1480.f, false, &uiCache);
        }

        // 6. ACT 1 EXPLORER DEMO
        if (currentScreen == SCREEN_ACT1_DEMO) {
            if (needLayout || cachedAct1Stats.empty()) {
                cachedAct1Stats = "Nodes: " + std::to_string(act1Tree.getSize()) +
                                  "  |  Height: " + std::to_string(act1Tree.height()) +
                                  "  |  Diameter: " + std::to_string(act1Tree.diameter()) +
                                  "  |  Symmetric: " + (act1Tree.isSymmetric() ? "YES" : "NO");
            }
            Ui::screenHeader(window, font, "ACT 1 EXPLORER // GENERAL BINARY TREE", cachedAct1Stats, &uiCache);
            Ui::button(window, font, "Campaign Map [Esc]", 1350.f, 26.f, 210.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            Ui::panel(window, 30.f, 110.f, 1140.f, 470.f, sf::Color(0, 255, 65), 248, &uiCache);
            if (!animCtrl.hasEvents() && needLayout) {
                layoutBtForScreen(true);
                layoutDirty = false;
            }
            const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : btLayout;
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.errorFlashNodeId = wrongFlashNodeId;
            hl.showBadges = showNodeBadges;
            sf::Vector2f wPos(0.f, 0.f);
            if (mapHoverToCam(demoCam, in.mx, in.my, wPos)) {
                int hovIdx = drawL.hitTestNode(wPos.x, wPos.y);
                if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
            }
            if (animCtrl.hasEvents()) {
                animCtrl.fillHighlightState(hl);
            } else {
                int hlIdx1 = drawL.findNodeIndexByKey(act1Hl1);
                int hlIdx2 = drawL.findNodeIndexByKey(act1Hl2);
                if (hlIdx1 >= 0) { hl.primaryNodeId = drawL.nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                if (hlIdx2 >= 0) hl.secondaryNodeId = drawL.nodes[hlIdx2].id;
            }
            window.setView(demoCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, demoCam.view);
            guiRenderer.drawTreeLayout(window, font, drawL, hl, false, true);
            window.setView(window.getDefaultView());
            Ui::text(window, font, demoCam.zoomText(), 11, Ui::textMuted(), 42.f, 116.f, false, &uiCache);
            drawCamTooltip(demoCam, drawL, hl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            Ui::panel(window, 30.f, 600.f, 1540.f, 270.f, sf::Color(60, 85, 125), 235, &uiCache);
            Ui::button(window, font, "[T] Traversals",    54.f,   620.f, 200.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[M] Mirror Tree",   274.f,  620.f, 200.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[R] Rebuild Pre+In",494.f,  620.f, 210.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[H] Height & Diam", 724.f,  620.f, 210.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[L] LCA & Path",    954.f,  620.f, 190.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[S] Symmetry",      1164.f, 620.f, 180.f, 44.f, BTN_GHOST, in, 14, &uiCache);
            Ui::button(window, font, "[Z] Reset",         1364.f, 620.f, 170.f, 44.f, BTN_DANGER, in, 14, &uiCache);

            Ui::textFit(window, font, status, TXT_BODY, sf::Color(0, 255, 65), 54.f, 690.f, 1480.f, false, &uiCache);
            if (!act1TraversalStr.empty()) {
                Ui::textWrap(window, font, act1TraversalStr, TXT_HINT, sf::Color(120, 235, 145), 54.f, 728.f, 1480.f, false, &uiCache);
            }
            Ui::textFit(window, font, "Click any node to compute root-to-node path  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 830.f, 1480.f, false, &uiCache);
        }

        // 7. ACT 2 EXPLORER DEMO
        if (currentScreen == SCREEN_ACT2_DEMO) {
            if (needLayout || cachedAct2Stats.empty()) {
                cachedAct2Stats = "Nodes: " + std::to_string(act2Bst.getSize()) +
                                  "  |  Height: " + std::to_string(act2Bst.getHeight()) +
                                  "  |  Valid BST: " + (act2Bst.isValidBST() ? "YES" : "NO");
            }
            Ui::screenHeader(window, font, "ACT 2 EXPLORER // BINARY SEARCH TREE", cachedAct2Stats, &uiCache);
            Ui::button(window, font, "Campaign Map [Esc]", 1350.f, 26.f, 210.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            Ui::panel(window, 30.f, 110.f, 1140.f, 470.f, sf::Color(0, 255, 65), 248, &uiCache);
            if (!animCtrl.hasEvents() && needLayout) {
                layoutBstForScreen(true);
                layoutDirty = false;
            }
            const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : bstLayout;
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.errorFlashNodeId = wrongFlashNodeId;
            hl.showBadges = showNodeBadges;
            sf::Vector2f wPos(0.f, 0.f);
            if (mapHoverToCam(demoCam, in.mx, in.my, wPos)) {
                int hovIdx = drawL.hitTestNode(wPos.x, wPos.y);
                if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
            }
            if (animCtrl.hasEvents()) {
                animCtrl.fillHighlightState(hl);
            } else {
                int hlIdx1 = drawL.findNodeIndexByKey(act2Hl1);
                int hlIdx2 = drawL.findNodeIndexByKey(act2Hl2);
                if (hlIdx1 >= 0) { hl.primaryNodeId = drawL.nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                if (hlIdx2 >= 0) hl.secondaryNodeId = drawL.nodes[hlIdx2].id;
            }
            window.setView(demoCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, demoCam.view);
            guiRenderer.drawTreeLayout(window, font, drawL, hl, false, true);
            window.setView(window.getDefaultView());
            Ui::text(window, font, demoCam.zoomText(), 11, Ui::textMuted(), 42.f, 116.f, false, &uiCache);
            drawCamTooltip(demoCam, drawL, hl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            Ui::panel(window, 30.f, 600.f, 1540.f, 270.f, sf::Color(60, 85, 125), 235, &uiCache);
            Ui::button(window, font, "[I] Insert",      54.f,   620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[D] Delete",      222.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[S] Search",      390.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[M] Min/Max",     558.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[P] Succ/Pred",   726.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[R] Range",       894.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[K] kth",         1062.f, 620.f, 145.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[C] Chain",       1220.f, 620.f, 145.f, 44.f, BTN_GHOST, in, 14, &uiCache);
            Ui::button(window, font, "[Z] Reset",       1378.f, 620.f, 156.f, 44.f, BTN_DANGER, in, 14, &uiCache);

            Ui::textFit(window, font, status, TXT_BODY, sf::Color(0, 255, 65), 54.f, 690.f, 1480.f, false, &uiCache);
            if (!act2ExtraInfo.empty()) {
                Ui::textFit(window, font, act2ExtraInfo, TXT_HINT, sf::Color(120, 235, 145), 54.f, 730.f, 1480.f, false, &uiCache);
            }
            Ui::textFit(window, font, "Click any BST node to search & inspect Pred/Succ  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 830.f, 1480.f, false, &uiCache);
        }

        // 8. REAL PERSISTENT LEADERBOARD (Item 3)
        if (currentScreen == SCREEN_LEADERBOARD) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color lbBright(0, 255, 65);
            const sf::Color lbDim(74, 122, 74);
            const sf::Color lbSubtle(120, 195, 120);

            Ui::text(window, menuFont, "OPERATOR LEADERBOARD", 34, lbBright, 50.f, 24.f, true, &uiCache);
            Ui::text(window, menuFont, "Recorded campaign heist scores loaded from scores.txt", 14, sf::Color::White, 50.f, 70.f, false, &uiCache);
            sf::RectangleShape hdrLine({1150.f, 1.f});
            hdrLine.setPosition({50.f, 102.f});
            hdrLine.setFillColor(sf::Color(0, 255, 65, 140));
            window.draw(hdrLine);

            float mw = 920.f, mh = 600.f;
            float mx0 = static_cast<float>(W) * 0.5f - mw * 0.5f, my0 = 130.f;
            sf::RectangleShape lbPanel({mw, mh});
            lbPanel.setPosition({mx0, my0});
            lbPanel.setFillColor(sf::Color(6, 14, 10, 228));
            lbPanel.setOutlineColor(lbBright);
            lbPanel.setOutlineThickness(1.5f);
            window.draw(lbPanel);

            // Corner accents
            const float cLen = 14.f, cThk = 3.f;
            sf::RectangleShape lc1({cLen, cThk}); lc1.setPosition({mx0 - 1.f, my0 - 1.f}); lc1.setFillColor(lbBright); window.draw(lc1);
            sf::RectangleShape lc2({cThk, cLen}); lc2.setPosition({mx0 - 1.f, my0 - 1.f}); lc2.setFillColor(lbBright); window.draw(lc2);
            sf::RectangleShape lc3({cLen, cThk}); lc3.setPosition({mx0 + mw - cLen + 1.f, my0 - 1.f}); lc3.setFillColor(lbBright); window.draw(lc3);
            sf::RectangleShape lc4({cThk, cLen}); lc4.setPosition({mx0 + mw - cThk + 1.f, my0 - 1.f}); lc4.setFillColor(lbBright); window.draw(lc4);
            sf::RectangleShape lc5({cLen, cThk}); lc5.setPosition({mx0 - 1.f, my0 + mh - cThk + 1.f}); lc5.setFillColor(lbBright); window.draw(lc5);
            sf::RectangleShape lc6({cThk, cLen}); lc6.setPosition({mx0 - 1.f, my0 + mh - cLen + 1.f}); lc6.setFillColor(lbBright); window.draw(lc6);
            sf::RectangleShape lc7({cLen, cThk}); lc7.setPosition({mx0 + mw - cLen + 1.f, my0 + mh - cThk + 1.f}); lc7.setFillColor(lbBright); window.draw(lc7);
            sf::RectangleShape lc8({cThk, cLen}); lc8.setPosition({mx0 + mw - cThk + 1.f, my0 + mh - cLen + 1.f}); lc8.setFillColor(lbBright); window.draw(lc8);

            if (scoreLog.getCount() == 0) {
                Ui::textCenter(window, menuFont, "No heists recorded yet", 24, lbBright,
                               static_cast<float>(W) * 0.5f, my0 + 200.f, true, &uiCache);
                Ui::textCenter(window, menuFont, "Complete any campaign level to record your score here.",
                               15, lbSubtle, static_cast<float>(W) * 0.5f, my0 + 255.f, false, &uiCache);
            } else {
                Ui::text(window, menuFont, "RANK",  14, sf::Color(255, 210, 40), mx0 + 36.f,  my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "LEVEL", 14, sf::Color(255, 210, 40), mx0 + 120.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "SCORE", 14, sf::Color(255, 210, 40), mx0 + 520.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "ALARM", 14, sf::Color(255, 210, 40), mx0 + 660.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "STARS", 14, sf::Color(255, 210, 40), mx0 + 785.f, my0 + 24.f, true, &uiCache);

                sf::RectangleShape thLine({mw - 48.f, 1.f});
                thLine.setPosition({mx0 + 24.f, my0 + 52.f});
                thLine.setFillColor(sf::Color(0, 255, 65, 90));
                window.draw(thLine);

                int rows = minInt(10, scoreLog.getCount());
                if (cachedLbCount != scoreLog.getCount()) {
                    cachedLbCount = scoreLog.getCount();
                    for (int r = 0; r < rows; ++r) {
                        int idx = scoreLog.topIndex(r);
                        if (idx < 0) break;
                        const ScoreEntry& e = scoreLog.get(idx);
                        cachedLbRank[r]  = "#" + std::to_string(r + 1);
                        cachedLbScore[r] = std::to_string(e.score) + " pts";
                        cachedLbAlarm[r] = std::to_string(e.alarm);
                    }
                }
                for (int r = 0; r < rows; ++r) {
                    int idx = scoreLog.topIndex(r);
                    if (idx < 0) break;
                    const ScoreEntry& e = scoreLog.get(idx);
                    float ry = my0 + 68.f + static_cast<float>(r) * 46.f;
                    Ui::text(window, menuFont, cachedLbRank[r], 15, lbBright, mx0 + 36.f, ry, true, &uiCache);
                    Ui::textFit(window, menuFont, e.levelTitle, 15, sf::Color(220, 240, 225), mx0 + 120.f, ry, 380.f, false, &uiCache);
                    Ui::text(window, menuFont, cachedLbScore[r], 15, lbBright, mx0 + 520.f, ry, false, &uiCache);
                    Ui::text(window, menuFont, cachedLbAlarm[r], 15, sf::Color(220, 240, 225), mx0 + 675.f, ry, false, &uiCache);
                    Ui::stars(window, mx0 + 820.f, ry + 9.f, e.stars, 3, 7.f, &uiCache);

                    if (r < rows - 1) {
                        sf::RectangleShape rowLine({mw - 48.f, 1.f});
                        rowLine.setPosition({mx0 + 24.f, ry + 34.f});
                        rowLine.setFillColor(sf::Color(0, 255, 65, 35));
                        window.draw(rowLine);
                    }
                }
            }

            bool endHov = Ui::inside(in.mx, in.my, static_cast<float>(W) * 0.5f - 310.f, 770.f, 290.f, 48.f);
            sf::RectangleShape endBtn({290.f, 48.f});
            endBtn.setPosition({static_cast<float>(W) * 0.5f - 310.f, 770.f});
            endBtn.setFillColor(endHov ? sf::Color(10, 40, 18, 235) : sf::Color(6, 16, 10, 225));
            endBtn.setOutlineColor(endHov ? lbBright : lbDim);
            endBtn.setOutlineThickness(endHov ? 2.f : 1.5f);
            window.draw(endBtn);
            Ui::textCenter(window, menuFont, "Endless Leaderboard [E]", 15,
                           endHov ? sf::Color::White : sf::Color::Yellow,
                           static_cast<float>(W) * 0.5f - 165.f, 784.f, endHov, &uiCache);

            bool retHov = Ui::inside(in.mx, in.my, static_cast<float>(W) * 0.5f + 20.f, 770.f, 290.f, 48.f);
            sf::RectangleShape retBtn({290.f, 48.f});
            retBtn.setPosition({static_cast<float>(W) * 0.5f + 20.f, 770.f});
            retBtn.setFillColor(retHov ? sf::Color(12, 54, 22, 240) : sf::Color(8, 28, 14, 230));
            retBtn.setOutlineColor(lbBright);
            retBtn.setOutlineThickness(2.f);
            window.draw(retBtn);
            Ui::textCenter(window, menuFont, "Return to Main Menu [ESC]", 15,
                           retHov ? sf::Color::White : sf::Color::Yellow,
                           static_cast<float>(W) * 0.5f + 165.f, 784.f, true, &uiCache);
        }

        // 9. REAL WORKING SETTINGS (Item 4)
        if (currentScreen == SCREEN_SETTINGS) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color setBright(0, 255, 65);
            const sf::Color setDim(74, 122, 74);
            const sf::Color setSubtle(120, 195, 120);

            Ui::text(window, menuFont, "SYSTEM SETTINGS", 34, setBright, 50.f, 24.f, true, &uiCache);
            Ui::text(window, menuFont, "Every setting below takes effect immediately across the interface.", 14, sf::Color::White, 50.f, 70.f, false, &uiCache);
            sf::RectangleShape hdrLine({1150.f, 1.f});
            hdrLine.setPosition({50.f, 102.f});
            hdrLine.setFillColor(sf::Color(0, 255, 65, 140));
            window.draw(hdrLine);

            float mw = 760.f, mh = 560.f;
            float sx0 = static_cast<float>(W) * 0.5f - mw * 0.5f, sy0 = 140.f;
            sf::RectangleShape setPanel({mw, mh});
            setPanel.setPosition({sx0, sy0});
            setPanel.setFillColor(sf::Color(6, 14, 10, 228));
            setPanel.setOutlineColor(setBright);
            setPanel.setOutlineThickness(1.5f);
            window.draw(setPanel);

            // Corner accents
            const float cLen = 14.f, cThk = 3.f;
            sf::RectangleShape sc1({cLen, cThk}); sc1.setPosition({sx0 - 1.f, sy0 - 1.f}); sc1.setFillColor(setBright); window.draw(sc1);
            sf::RectangleShape sc2({cThk, cLen}); sc2.setPosition({sx0 - 1.f, sy0 - 1.f}); sc2.setFillColor(setBright); window.draw(sc2);
            sf::RectangleShape sc3({cLen, cThk}); sc3.setPosition({sx0 + mw - cLen + 1.f, sy0 - 1.f}); sc3.setFillColor(setBright); window.draw(sc3);
            sf::RectangleShape sc4({cThk, cLen}); sc4.setPosition({sx0 + mw - cThk + 1.f, sy0 - 1.f}); sc4.setFillColor(setBright); window.draw(sc4);
            sf::RectangleShape sc5({cLen, cThk}); sc5.setPosition({sx0 - 1.f, sy0 + mh - cThk + 1.f}); sc5.setFillColor(setBright); window.draw(sc5);
            sf::RectangleShape sc6({cThk, cLen}); sc6.setPosition({sx0 - 1.f, sy0 + mh - cLen + 1.f}); sc6.setFillColor(setBright); window.draw(sc6);
            sf::RectangleShape sc7({cLen, cThk}); sc7.setPosition({sx0 + mw - cLen + 1.f, sy0 + mh - cThk + 1.f}); sc7.setFillColor(setBright); window.draw(sc7);
            sf::RectangleShape sc8({cThk, cLen}); sc8.setPosition({sx0 + mw - cThk + 1.f, sy0 + mh - cLen + 1.f}); sc8.setFillColor(setBright); window.draw(sc8);

            if (cachedSetFpsVal != targetFps) {
                cachedSetFpsVal = targetFps;
                cachedSetFpsLimStr = "Frame Rate Limit: " + std::to_string(targetFps) + " FPS";
                cachedSetFpsBtnStr = "Frame Rate Cap: " + std::to_string(targetFps) + " FPS (Click to toggle 60/120)";
            }

            float bw = 560.f, bh = 52.f, bx = static_cast<float>(W) * 0.5f - bw * 0.5f;
            Ui::text(window, menuFont, cachedSetResStr, 15, sf::Color(255, 210, 40), bx, sy0 + 36.f, false, &uiCache);
            Ui::text(window, menuFont, cachedSetFpsLimStr, 15, sf::Color(255, 210, 40), bx, sy0 + 68.f, false, &uiCache);

            auto drawSettingToggle = [&](const std::string& label, float yOff, bool active) {
                float by = sy0 + yOff;
                bool hov = Ui::inside(in.mx, in.my, bx, by, bw, bh);
                sf::RectangleShape box({bw, bh});
                box.setPosition({bx, by});
                box.setFillColor(hov ? sf::Color(10, 38, 18, 235) : sf::Color(5, 16, 9, 220));
                box.setOutlineColor(hov ? setBright : (active ? sf::Color(0, 255, 65, 160) : setDim));
                box.setOutlineThickness(hov ? 2.f : 1.f);
                window.draw(box);
                Ui::textCenter(window, menuFont, label, 15,
                               hov ? setBright : (active ? sf::Color(220, 245, 225) : setSubtle),
                               bx + bw * 0.5f, by + 16.f, hov, &uiCache);
            };

            drawSettingToggle(soundEnabled ? "Audio FX Feedback: ENABLED" : "Audio FX Feedback: MUTED", 125.f, soundEnabled);
            drawSettingToggle(showNodeBadges ? "Node Badges (h / BF / sz): VISIBLE" : "Node Badges (h / BF / sz): HIDDEN", 195.f, showNodeBadges);
            drawSettingToggle(showBgGrid ? "Blueprint Grid Overlay: VISIBLE" : "Blueprint Grid Overlay: HIDDEN", 265.f, showBgGrid);
            drawSettingToggle(autoPlayEvents ? "Operation Animation Mode: AUTO-PLAY" : "Operation Animation Mode: STEP-BY-STEP", 335.f, autoPlayEvents);
            drawSettingToggle(cachedSetFpsBtnStr, 405.f, true);

            bool retHov = Ui::inside(in.mx, in.my, static_cast<float>(W) * 0.5f - 150.f, 740.f, 300.f, 48.f);
            sf::RectangleShape retBtn({300.f, 48.f});
            retBtn.setPosition({static_cast<float>(W) * 0.5f - 150.f, 740.f});
            retBtn.setFillColor(retHov ? sf::Color(12, 46, 20, 235) : sf::Color(6, 18, 10, 225));
            retBtn.setOutlineColor(setBright);
            retBtn.setOutlineThickness(retHov ? 2.f : 1.5f);
            window.draw(retBtn);
            Ui::textCenter(window, menuFont, "Return to Main Menu [ESC]", 15,
                           retHov ? sf::Color::White : sf::Color::Yellow,
                           static_cast<float>(W) * 0.5f, 754.f, retHov, &uiCache);
        }

        // 10. ENDLESS SURVIVAL MODE (Step F)
        if (currentScreen == SCREEN_ENDLESS) {
            if (missionBgSprite.has_value()) {
                window.draw(*missionBgSprite);
            }
            int opsNow = endlessSession.getOpsCompleted();
            int nodesNow = avl.getSize();
            float winSec = endlessSession.getOpWindowSec();
            float remSec = endlessSession.getOpTimeRemaining();
            float opsMin = EndlessSession::computeOpsPerMin(winSec);
            int rateTenths = static_cast<int>(opsMin * 10.0f + 0.5f);

            if (cachedEndlessOps != opsNow || cachedEndlessNodes != nodesNow || cachedEndlessRateTenths != rateTenths) {
                cachedEndlessOps = opsNow;
                cachedEndlessNodes = nodesNow;
                cachedEndlessRateTenths = rateTenths;
                cachedEndlessSub = "Seed: " + std::to_string(endlessSession.getInitialSeed()) +
                                   "  |  Ops: " + std::to_string(opsNow) +
                                   "  |  Nodes: " + std::to_string(nodesNow) + " (H=" + std::to_string(avl.getHeight()) +
                                   ")  |  Rate: " + std::to_string(rateTenths / 10) + "." +
                                   std::to_string(rateTenths % 10) + " ops/min";
            }

            Ui::textFit(window, font, "ENDLESS SURVIVAL // LIVE AVL STREAM", 24, sf::Color(255, 210, 40),
                        30.f, 12.f, 620.f, true, &uiCache);
            Ui::textFit(window, font, cachedEndlessSub, 14, Ui::textBody(),
                        30.f, 46.f, 620.f, false, &uiCache);

            Ui::button(window, font, endlessPaused ? "Resume [P]" : "Pause [P]",
                       670.f, 18.f, 125.f, 34.f, endlessPaused ? BTN_PRIMARY : BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Restart [R]", 805.f, 18.f, 125.f, 34.f, BTN_GHOST, in, 13, &uiCache);
            Ui::button(window, font, "Menu [Esc]",  940.f, 18.f, 125.f, 34.f, BTN_GHOST, in, 13, &uiCache);

            float meterX = 1090.f, meterW = 480.f;
            float ratio = clampFloat((winSec > 0.f) ? (remSec / winSec) : 0.f, 0.f, 1.f);
            sf::RectangleShape& mRect = uiCache.rect;
            mRect.setSize({meterW, 20.f});
            mRect.setOrigin({0.f, 0.f});
            mRect.setPosition({meterX, 16.f});
            mRect.setFillColor(sf::Color(35, 42, 58));
            mRect.setOutlineThickness(1.5f);
            mRect.setOutlineColor(sf::Color(100, 115, 140));
            window.draw(mRect);

            mRect.setSize({meterW * ratio, 20.f});
            mRect.setPosition({meterX, 16.f});
            mRect.setOutlineThickness(0.f);
            mRect.setFillColor(ratio < 0.25f ? Ui::red() : sf::Color(255, 210, 40));
            window.draw(mRect);

            int remMs = static_cast<int>(remSec * 1000.0f + 0.5f);
            int winMs = static_cast<int>(winSec * 1000.0f + 0.5f);
            std::string hudTimerStr = "Survival: " + EndlessLeaderboard::formatTimeMs(endlessSession.getSurvivalMs()) +
                                      "   |   Deadline: " + EndlessLeaderboard::formatTimeMs(remMs) +
                                      " / " + EndlessLeaderboard::formatTimeMs(winMs);
            Ui::textFit(window, font, hudTimerStr, 14, sf::Color::White, meterX, 44.f, meterW, true, &uiCache);

            // Canvas + AVL render
            Ui::panel(window, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, sf::Color(0, 255, 65), 248, &uiCache);
            if (!animCtrl.hasEvents() && needLayout) {
                layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                layoutDirty = false;
            }
            const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : avlLayout;
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.showBadges = true;
            sf::Vector2f wPos(0.f, 0.f);
            if (mapHoverToCam(endlessCam, in.mx, in.my, wPos)) {
                int hovIdx = drawL.hitTestNode(wPos.x, wPos.y);
                if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
            }
            if (animCtrl.hasEvents()) animCtrl.fillHighlightState(hl);

            window.setView(endlessCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, endlessCam.view);
            guiRenderer.drawTreeLayout(window, font, drawL, hl, true, false);
            window.setView(window.getDefaultView());
            Ui::text(window, font, endlessCam.zoomText(), 11, Ui::textMuted(),
                     CANVAS_X + 12.f, CANVAS_Y + 6.f, false, &uiCache);
            drawCamTooltip(endlessCam, drawL, hl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            // Bottom Pending Operation & Persistence Panel
            Ui::panel(window, 30.f, 660.f, 1540.f, 210.f, sf::Color(60, 85, 125), 235, &uiCache);
            const EndlessOp& op = endlessSession.getPendingOp();
            if (cachedEndlessOpType != op.type || cachedEndlessOpKey != op.key || cachedEndlessPaused != endlessPaused) {
                cachedEndlessOpType = op.type;
                cachedEndlessOpKey = op.key;
                cachedEndlessPaused = endlessPaused;
                if (endlessPaused) {
                    cachedEndlessPrompt = "PAUSED - Press [P] to resume live AVL stream timer";
                } else {
                    cachedEndlessPrompt = std::string("INCOMING OP: ") +
                                          (op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                                          std::to_string(op.key) +
                                          "  - Click [Execute] to apply real AVL operation & check invariants!";
                }
            }
            Ui::textFit(window, font, cachedEndlessPrompt, TXT_H2, sf::Color(255, 210, 40), 54.f, 678.f, 1040.f, true, &uiCache);
            Ui::textWrap(window, font, status, TXT_BODY, Ui::textBody(), 54.f, 720.f, 1040.f, false, &uiCache);
            Ui::textFit(window, font, "[Enter / X] Execute Pending Op  |  [P] Pause  |  [F5] Save Run  |  [F9] Load Run  |  [L] Leaderboard  |  [Esc] Main Menu",
                        TXT_HINT, Ui::textMuted(), 54.f, 832.f, 1480.f, false, &uiCache);

            std::string execBtnLbl = std::string("EXECUTE: ") +
                                     (op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                                     std::to_string(op.key) + " [Enter]";
            int execStyle = endlessPaused ? BTN_DISABLED : (ratio < 0.25f ? BTN_DANGER : BTN_PRIMARY);
            Ui::button(window, font, execBtnLbl, 1120.f, 678.f, 428.f, 54.f, execStyle, in, 15, &uiCache);
            Ui::button(window, font, "Save Run [F5]", 1120.f, 748.f, 136.f, 46.f, BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Load Run [F9]", 1266.f, 748.f, 136.f, 46.f, BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Scores [L]",    1412.f, 748.f, 136.f, 46.f, BTN_GHOST,     in, 13, &uiCache);
        }

        // 11. ENDLESS MODE LEADERBOARD (endless_leaderboard.txt)
        if (currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color elbBright(0, 255, 65);
            const sf::Color elbSubtle(120, 195, 120);

            Ui::text(window, menuFont, "ENDLESS SURVIVAL LEADERBOARD", 34, elbBright, 50.f, 20.f, true, &menuUiCache);
            Ui::text(window, menuFont, "Survival times ranked descending from endless_leaderboard.txt", 14, sf::Color::White, 50.f, 64.f, false, &menuUiCache);
            sf::RectangleShape hdrLine({1150.f, 1.f});
            hdrLine.setPosition({50.f, 92.f});
            hdrLine.setFillColor(sf::Color(0, 255, 65, 140));
            window.draw(hdrLine);

            float mw = 920.f;
            float mx0 = static_cast<float>(W) * 0.5f - mw * 0.5f;
            if (endlessShowLastResult) {
                Ui::panel(window, mx0, 104.f, mw, 86.f, Ui::red(), 235, &uiCache);
                std::string runSum = "ENDLESS RUN ENDED: " + EndlessLeaderboard::formatTimeMs(endlessLastEntry.survivalMs) +
                                     " survived  |  " + std::to_string(endlessLastEntry.opsCompleted) +
                                     " ops  |  Seed #" + std::to_string(endlessLastEntry.seed);
                Ui::textFit(window, menuFont, runSum, TXT_H2, sf::Color(255, 110, 110), mx0 + 28.f, 116.f, mw - 56.f, true, &menuUiCache);
                Ui::textFit(window, menuFont, endlessLastReason, TXT_HINT, sf::Color(255, 200, 200),
                            mx0 + 28.f, 154.f, mw - 56.f, false, &menuUiCache);
            }

            float my0 = endlessShowLastResult ? 204.f : 130.f;
            float mh  = endlessShowLastResult ? 540.f : 600.f;
            Ui::panel(window, mx0, my0, mw, mh, elbBright, 232, &uiCache);

            if (endlessLb.getCount() == 0) {
                Ui::textCenter(window, menuFont, "No endless survival runs recorded yet", 24, elbBright,
                               static_cast<float>(W) * 0.5f, my0 + 180.f, true, &menuUiCache);
                Ui::textCenter(window, menuFont, "Start Endless Survival Mode from the Main Menu to set a survival record!",
                               15, elbSubtle, static_cast<float>(W) * 0.5f, my0 + 235.f, false, &menuUiCache);
            } else {
                Ui::text(window, menuFont, "RANK",            14, sf::Color(255, 210, 40), mx0 + 36.f,  my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "OPERATOR / DATE", 14, sf::Color(255, 210, 40), mx0 + 120.f, my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "SURVIVAL TIME",   14, sf::Color(255, 210, 40), mx0 + 500.f, my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "OPS",             14, sf::Color(255, 210, 40), mx0 + 670.f, my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "SEED",            14, sf::Color(255, 210, 40), mx0 + 760.f, my0 + 22.f, true, &menuUiCache);

                sf::RectangleShape thLine({mw - 48.f, 1.f});
                thLine.setPosition({mx0 + 24.f, my0 + 48.f});
                thLine.setFillColor(sf::Color(0, 255, 65, 90));
                window.draw(thLine);

                int rows = minInt(10, endlessLb.getCount());
                for (int r = 0; r < rows; ++r) {
                    const EndlessLeaderboardEntry& e = endlessLb.get(r);
                    float ry = my0 + 58.f + static_cast<float>(r) * 44.f;
                    Ui::text(window, menuFont, "#" + std::to_string(r + 1), 15, elbBright,
                             mx0 + 36.f, ry, true, &menuUiCache);
                    Ui::textFit(window, menuFont, e.nameDate, 15, sf::Color(220, 240, 225),
                                mx0 + 120.f, ry, 360.f, false, &menuUiCache);
                    Ui::text(window, menuFont, EndlessLeaderboard::formatTimeMs(e.survivalMs), 15, elbBright,
                             mx0 + 515.f, ry, false, &menuUiCache);
                    Ui::text(window, menuFont, std::to_string(e.opsCompleted), 15, sf::Color(220, 240, 225),
                             mx0 + 675.f, ry, false, &menuUiCache);
                    Ui::text(window, menuFont, std::to_string(e.seed), 15, elbSubtle,
                             mx0 + 760.f, ry, false, &menuUiCache);
                }
            }

            Ui::button(window, font, "New Endless Run [R]", 270.f,  770.f, 250.f, 48.f, BTN_PRIMARY,   in, 14, &uiCache);
            Ui::button(window, font, "Replay Seed [S]",     540.f,  770.f, 250.f, 48.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "Campaign Scores [C]", 810.f,  770.f, 250.f, 48.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "Main Menu [Esc]",     1080.f, 770.f, 250.f, 48.f, BTN_GHOST,     in, 14, &uiCache);
        }

        cachedLayoutScreen = currentScreen;
        cachedLayoutLid = curLid;

        if (!profileFps && verifyDir.empty()) {
            Ui::text(window, font, liveFpsStr, 11, Ui::textMuted(), 1530.f, 2.f, false, &uiCache);
        }

        // Toasts rendered on top of every screen (Item 2)
        toastQueue.draw(window, font, static_cast<float>(W));
        if (outCpuMs) *outCpuMs = static_cast<double>(frameCpuClk.getElapsedTime().asMicroseconds()) / 1000.0;
        if (!capturePath.empty()) {
            sf::Texture tex(sf::Vector2u(static_cast<unsigned>(W), static_cast<unsigned>(H)));
            tex.update(window);
            (void)tex.copyToImage().saveToFile(capturePath);
        }
        sf::Clock dispClk;
        window.display();
        if (outDispMs) *outDispMs = static_cast<double>(dispClk.getElapsedTime().asMicroseconds()) / 1000.0;
    };
