
    // =========================================================================
    // INTERACTIVE MAIN LOOP
    // =========================================================================
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        fpsFrameAccum++;
        float fpsSec = fpsReportClock.getElapsedTime().asSeconds();
        if (fpsSec >= 0.25f) {
            int instFps = static_cast<int>(static_cast<float>(fpsFrameAccum) / fpsSec + 0.5f);
            liveFpsStr = "FPS: " + std::to_string(instFps);
            fpsFrameAccum = 0;
            fpsReportClock.restart();
        }
        if (dt > 0.05f) dt = 0.05f;
        dialAngle += 50.f * dt;
        animCtrl.update(dt);
        toastQueue.update(dt);
        if (wrongFlashTimer > 0.f) {
            wrongFlashTimer -= dt;
            if (wrongFlashTimer <= 0.f) {
                wrongFlashTimer = 0.f;
                wrongFlashNodeId = -1;
            }
        }
        if (act2TwoChildPhase2Pending && animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            startAct2TwoChildPhase2();
        }
        if (act3NextPuzzlePending && animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            loadNextAct3Puzzle();
        }
        if (pendingWin && !act2TwoChildPhase2Pending && !act3NextPuzzlePending &&
            animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            completePendingWin();
        }
        if (currentScreen == SCREEN_ENDLESS && endlessSession.isActive() && !endlessPaused) {
            if (endlessSession.tick(dt)) {
                finishEndlessGameOver();
            }
        }

        uiAnimTimer += dt;
        if (currentScreen == SCREEN_SPLASH) {
            splashTimer += dt;
            if (splashTimer >= 3.8f) {
                currentScreen = SCREEN_MAIN_MENU;
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        UiInput in;
        in.mx = static_cast<float>(mPos.x);
        in.my = static_cast<float>(mPos.y);
        in.down = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        if (draggingSpeedSlider) updateSpeedFromMouse(in.mx, sliderPanelX, sliderPanelW);
        if (draggingScrubber)    updateScrubberFromMouse(in.mx, sliderPanelX, sliderPanelW);

        auto updateCamPan = [&](CanvasCamera& cam) {
            if (!cam.panning) return;
            bool anyBtn = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ||
                          sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) ||
                          sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
            if (!anyBtn) { cam.panning = false; return; }
            if (mPos != cam.lastPanPixel) {
                sf::Vector2f w0 = window.mapPixelToCoords(cam.lastPanPixel, cam.view);
                sf::Vector2f w1 = window.mapPixelToCoords(mPos, cam.view);
                cam.view.move(w0 - w1);
                cam.clampCenter();
                cam.lastPanPixel = mPos;
            }
        };
        if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) {
            if (dragDrop.isDragging()) {
                sf::Vector2f wDrag = window.mapPixelToCoords(mPos, levelCam.view);
                dragDrop.move(wDrag.x, wDrag.y);
            }
            updateCamPan(levelCam);
        }
        else if (currentScreen == SCREEN_ENDLESS) updateCamPan(endlessCam);
        else if (currentScreen == SCREEN_SANDBOX) { updateCamPan(sbAvlCam); updateCamPan(sbHeapCam); }
        else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) updateCamPan(demoCam);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) { doSaveGame(); window.close(); }
            if (const auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                if (currentScreen == SCREEN_MAIN_MENU) {
                    sf::Vector2f mp = window.mapPixelToCoords(mm->position);
                    for (int i = 0; i < 6; ++i) {
                        float itemY = 300.f + static_cast<float>(i) * 70.f;
                        if (Ui::inside(mp.x, mp.y, 90.f, itemY - 10.f, 430.f, 54.f)) {
                            menuSelectedIdx = i;
                            break;
                        }
                    }
                }
            }
            if (const auto* mr = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mr->button == sf::Mouse::Button::Left && dragDrop.isDragging()) {
                    sf::Vector2f wRel = window.mapPixelToCoords(mr->position, levelCam.view);
                    DropResult dropRes = dragDrop.release(wRel.x, wRel.y);
                    if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0 &&
                        levels[currentLevelIdx].id == LEVEL_1_2) {
                        handleAct1BlueprintDrop(dropRes);
                    }
                }
                draggingSpeedSlider = false;
                draggingScrubber = false;
                levelCam.panning = false;
                endlessCam.panning = false;
                demoCam.panning = false;
                sbAvlCam.panning = false;
                sbHeapCam.panning = false;
            }

            if (const auto* mw = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mw->wheel == sf::Mouse::Wheel::Vertical) {
                    if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) {
                        levelCam.handleZoom(window, mw->position, mw->delta);
                    } else if (currentScreen == SCREEN_ENDLESS) {
                        endlessCam.handleZoom(window, mw->position, mw->delta);
                    } else if (currentScreen == SCREEN_SANDBOX) {
                        if (!sbAvlCam.handleZoom(window, mw->position, mw->delta)) {
                            sbHeapCam.handleZoom(window, mw->position, mw->delta);
                        }
                    } else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) {
                        demoCam.handleZoom(window, mw->position, mw->delta);
                    }
                }
            }

            if (const auto* te = event->getIf<sf::Event::TextEntered>()) {
                if (currentScreen == SCREEN_CAMPAIGN_MAP && demoPromptOpen) {
                    std::uint32_t uc = te->unicode;
                    if (uc == 8) {
                        if (demoPasswordLen > 0) {
                            --demoPasswordLen;
                            demoPasswordBuf[demoPasswordLen] = '\0';
                        }
                    } else if (uc >= 32 && uc < 127) {
                        if (demoPasswordLen < 32) {
                            demoPasswordBuf[demoPasswordLen++] = static_cast<char>(uc);
                            demoPasswordBuf[demoPasswordLen] = '\0';
                        }
                    }
                    continue;
                }
            }

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                    continue;
                }
                if (kp->code == sf::Keyboard::Key::Escape) {
                    playSfx(escSnd);
                    if (currentScreen == SCREEN_CAMPAIGN_MAP && demoPromptOpen) {
                        demoPromptOpen = false;
                        demoPasswordLen = 0;
                        demoPasswordBuf[0] = '\0';
                        continue;
                    }
                    if (currentScreen == SCREEN_LEVEL_PLAY || currentScreen == SCREEN_RESULT) {
                        dragDrop.cancel();
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                    } else if (currentScreen == SCREEN_ENDLESS) {
                        endlessSession.stop();
                        endlessPaused = false;
                        doSaveGame();
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else if (currentScreen == SCREEN_CAMPAIGN_MAP || currentScreen == SCREEN_SANDBOX ||
                               currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO ||
                               currentScreen == SCREEN_LEADERBOARD || currentScreen == SCREEN_SETTINGS ||
                               currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
                        resetConfirmPending = false;
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else {
                        doSaveGame();
                        window.close();
                    }
                    continue;
                }
                if (currentScreen == SCREEN_CAMPAIGN_MAP && demoPromptOpen) {
                    if (kp->code == sf::Keyboard::Key::Enter) {
                        submitDemoPassword();
                    }
                    continue;
                }

                if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    if (showingBriefing) {
                        if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) {
                            showingBriefing = false;
                            playSfx(missionStartSnd);
                        }
                        continue;
                    }
                    int lid = levels[currentLevelIdx].id;
                    if (kp->code == sf::Keyboard::Key::H)     useHint();
                    if (kp->code == sf::Keyboard::Key::Z)     performUndo();
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (lid == LEVEL_1_1 && kp->code == sf::Keyboard::Key::C) {
                        cycleAct1TravMode();
                    } else if (lid == LEVEL_1_3) {
                        if (kp->code == sf::Keyboard::Key::Up || kp->code == sf::Keyboard::Key::Equal ||
                            kp->code == sf::Keyboard::Key::Add) {
                            act1StepperVal = minInt(20, act1StepperVal + 1);
                        } else if (kp->code == sf::Keyboard::Key::Down || kp->code == sf::Keyboard::Key::Hyphen ||
                                   kp->code == sf::Keyboard::Key::Subtract) {
                            act1StepperVal = maxInt(1, act1StepperVal - 1);
                        } else if (kp->code == sf::Keyboard::Key::Enter) {
                            submitAct1HeightOrDiameter();
                        } else {
                            int numCode = static_cast<int>(kp->code);
                            int n1 = static_cast<int>(sf::Keyboard::Key::Num1);
                            int n9 = static_cast<int>(sf::Keyboard::Key::Num9);
                            if (numCode >= n1 && numCode <= n9) {
                                act1StepperVal = (numCode - n1) + 1;
                            }
                        }
                    } else if (lid == LEVEL_1_5 && (kp->code == sf::Keyboard::Key::A || kp->code == sf::Keyboard::Key::M)) {
                        triggerStructuralAction(1);
                    } else if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
                        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::L) {
                            handleAct2CompareChoice(1);
                        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::F ||
                                   kp->code == sf::Keyboard::Key::Enter) {
                            handleAct2CompareChoice(2);
                        } else if (kp->code == sf::Keyboard::Key::Num3 || kp->code == sf::Keyboard::Key::D) {
                            handleAct2CompareChoice(3);
                        }
                    } else if (lid == LEVEL_2_3) {
                        if (kp->code == sf::Keyboard::Key::Num1) handleAct2DeleteCase(1);
                        else if (kp->code == sf::Keyboard::Key::Num2) handleAct2DeleteCase(2);
                        else if (kp->code == sf::Keyboard::Key::Num3) handleAct2DeleteCase(3);
                    } else if (lid == LEVEL_2_4) {
                        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::R) {
                            handleAct2RangeDecision(RQ_ACT_REPORT);
                        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::L) {
                            handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT);
                        } else if (kp->code == sf::Keyboard::Key::Num3 || kp->code == sf::Keyboard::Key::P) {
                            handleAct2RangeDecision(RQ_ACT_PRUNE_RIGHT);
                        }
                    } else if (lid == LEVEL_3_2 && !pendingWin) {
                        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::L) {
                            handleAct3SingleRotation(1);
                        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::R) {
                            handleAct3SingleRotation(2);
                        }
                    } else if (lid == LEVEL_3_3 && manualDoubleStep == 0 && !pendingWin) {
                        if (kp->code == sf::Keyboard::Key::Num1) handleAct3DoubleCase(1);
                        else if (kp->code == sf::Keyboard::Key::Num2) handleAct3DoubleCase(2);
                    } else if (lid == LEVEL_3_4 && !pendingWin) {
                        if (kp->code == sf::Keyboard::Key::Num1) handleAct3Prediction("LL");
                        else if (kp->code == sf::Keyboard::Key::Num2) handleAct3Prediction("RR");
                        else if (kp->code == sf::Keyboard::Key::Num3) handleAct3Prediction("LR");
                        else if (kp->code == sf::Keyboard::Key::Num4) handleAct3Prediction("RL");
                        else if (kp->code == sf::Keyboard::Key::Num5 || kp->code == sf::Keyboard::Key::N) {
                            handleAct3Prediction("None");
                        }
                    } else if (lid == LEVEL_4_5 && kp->code == sf::Keyboard::Key::B) {
                        triggerStructuralAction(1);
                    }
                } else if (currentScreen == SCREEN_RESULT) {
                    if (kp->code == sf::Keyboard::Key::R) startLevel(currentLevelIdx);
                    if (kp->code == sf::Keyboard::Key::N) {
                        if (currentLevelIdx + 1 < TOTAL_LEVELS && levels[currentLevelIdx + 1].unlocked) {
                            startLevel(currentLevelIdx + 1);
                        } else {
                            currentScreen = SCREEN_CAMPAIGN_MAP;
                        }
                    }
                } else if (currentScreen == SCREEN_MAIN_MENU) {
                    auto activateMenuIdx = [&](int idx) {
                        playSfx(menuClickSnd);
                        if (idx == 0) { currentScreen = SCREEN_CAMPAIGN_MAP; status = "Heist Wings Overview"; }
                        else if (idx == 1) startEndlessMode(0u);
                        else if (idx == 2) enterSandbox();
                        else if (idx == 3) currentScreen = SCREEN_LEADERBOARD;
                        else if (idx == 4) currentScreen = SCREEN_SETTINGS;
                        else if (idx == 5) window.close();
                    };
                    if (kp->code == sf::Keyboard::Key::Up || kp->code == sf::Keyboard::Key::W) {
                        menuSelectedIdx = (menuSelectedIdx + 5) % 6;
                    } else if (kp->code == sf::Keyboard::Key::Down || kp->code == sf::Keyboard::Key::S) {
                        menuSelectedIdx = (menuSelectedIdx + 1) % 6;
                    } else if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) {
                        activateMenuIdx(menuSelectedIdx);
                    } else if (kp->code == sf::Keyboard::Key::Num1) { menuSelectedIdx = 0; activateMenuIdx(0); }
                    else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::E) { menuSelectedIdx = 1; activateMenuIdx(1); }
                    else if (kp->code == sf::Keyboard::Key::Num3) { menuSelectedIdx = 2; activateMenuIdx(2); }
                    else if (kp->code == sf::Keyboard::Key::Num4) { menuSelectedIdx = 3; activateMenuIdx(3); }
                    else if (kp->code == sf::Keyboard::Key::Num5) { menuSelectedIdx = 4; activateMenuIdx(4); }
                    else if (kp->code == sf::Keyboard::Key::Num6) { menuSelectedIdx = 5; activateMenuIdx(5); }
                } else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    if (kp->code == sf::Keyboard::Key::Up || kp->code == sf::Keyboard::Key::W) {
                        selectedActIdx = (selectedActIdx + 3) % 4;
                    } else if (kp->code == sf::Keyboard::Key::Down || kp->code == sf::Keyboard::Key::S) {
                        selectedActIdx = (selectedActIdx + 1) % 4;
                    } else if (kp->code == sf::Keyboard::Key::Left || kp->code == sf::Keyboard::Key::A) {
                        selectedLevelInAct = (selectedLevelInAct + 4) % 5;
                    } else if (kp->code == sf::Keyboard::Key::Right || kp->code == sf::Keyboard::Key::D) {
                        selectedLevelInAct = (selectedLevelInAct + 1) % 5;
                    } else if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) {
                        int li = selectedActIdx * 5 + selectedLevelInAct;
                        if (levels[li].unlocked) startLevel(li);
                        else pushToast("Locked! Clear previous level first", TOAST_KIND_INFO);
                    }
                } else if (currentScreen == SCREEN_LEADERBOARD) {
                    if (kp->code == sf::Keyboard::Key::E) {
                        endlessShowLastResult = false;
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                    }
                } else if (currentScreen == SCREEN_ENDLESS) {
                    if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::X) {
                        triggerEndlessExecute();
                    } else if (kp->code == sf::Keyboard::Key::P) {
                        endlessPaused = !endlessPaused;
                        pushToast(endlessPaused ? "Endless paused" : "Endless resumed!", TOAST_KIND_WARNING);
                    } else if (kp->code == sf::Keyboard::Key::F5) {
                        saveEndlessRun();
                    } else if (kp->code == sf::Keyboard::Key::F9) {
                        loadEndlessRun();
                    } else if (kp->code == sf::Keyboard::Key::L) {
                        endlessSession.stop();
                        endlessPaused = false;
                        endlessShowLastResult = false;
                        doSaveGame();
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                    } else if (kp->code == sf::Keyboard::Key::R) {
                        startEndlessMode(0u);
                    } else if (kp->code == sf::Keyboard::Key::Right) {
                        animCtrl.setPlaying(false);
                        stepAnimForward();
                    } else if (kp->code == sf::Keyboard::Key::Space) {
                        animCtrl.togglePlayPause();
                    }
                } else if (currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
                    if (kp->code == sf::Keyboard::Key::R) startEndlessMode(0u);
                    else if (kp->code == sf::Keyboard::Key::S) startEndlessMode(endlessLastSeed);
                    else if (kp->code == sf::Keyboard::Key::C) currentScreen = SCREEN_LEADERBOARD;
                } else if (currentScreen == SCREEN_SANDBOX) {
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (kp->code == sf::Keyboard::Key::Z) performUndo();
                    if (kp->code == sf::Keyboard::Key::I) triggerSandboxAction(1);
                    if (kp->code == sf::Keyboard::Key::D) triggerSandboxAction(2);
                    if (kp->code == sf::Keyboard::Key::K) triggerSandboxAction(3);
                    if (kp->code == sf::Keyboard::Key::E) triggerSandboxAction(4);
                    if (kp->code == sf::Keyboard::Key::R) triggerSandboxAction(5);
                } else if (currentScreen == SCREEN_ACT1_DEMO) {
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (kp->code == sf::Keyboard::Key::T) triggerAct1Action(1);
                    if (kp->code == sf::Keyboard::Key::M) triggerAct1Action(2);
                    if (kp->code == sf::Keyboard::Key::R) triggerAct1Action(3);
                    if (kp->code == sf::Keyboard::Key::H) triggerAct1Action(4);
                    if (kp->code == sf::Keyboard::Key::L) triggerAct1Action(5);
                    if (kp->code == sf::Keyboard::Key::S) triggerAct1Action(6);
                    if (kp->code == sf::Keyboard::Key::Z) triggerAct1Action(7);
                } else if (currentScreen == SCREEN_ACT2_DEMO) {
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (kp->code == sf::Keyboard::Key::I) triggerAct2Action(1);
                    if (kp->code == sf::Keyboard::Key::D) triggerAct2Action(2);
                    if (kp->code == sf::Keyboard::Key::S) triggerAct2Action(3);
                    if (kp->code == sf::Keyboard::Key::M) triggerAct2Action(4);
                    if (kp->code == sf::Keyboard::Key::P) triggerAct2Action(5);
                    if (kp->code == sf::Keyboard::Key::R) triggerAct2Action(6);
                    if (kp->code == sf::Keyboard::Key::K) triggerAct2Action(7);
                    if (kp->code == sf::Keyboard::Key::C) triggerAct2Action(8);
                    if (kp->code == sf::Keyboard::Key::Z) triggerAct2Action(9);
                }
            }

            if (const auto* mc = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(mc->position.x);
                float my = static_cast<float>(mc->position.y);
                bool isLeft   = (mc->button == sf::Mouse::Button::Left);
                bool isPanBtn = (mc->button == sf::Mouse::Button::Right || mc->button == sf::Mouse::Button::Middle);

                // Right/Middle button inside canvas: double-click resets view, drag pans (Part B)
                if (isPanBtn) {
                    auto handlePanPress = [&](CanvasCamera& cam) -> bool {
                        if (!cam.containsPixel(mx, my)) return false;
                        if (cam.checkDoubleClick(mc->position)) {
                            cam.reset();
                            pushToast("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            cam.panning = true;
                            cam.lastPanPixel = mc->position;
                        }
                        return true;
                    };
                    if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) handlePanPress(levelCam);
                    else if (currentScreen == SCREEN_ENDLESS) handlePanPress(endlessCam);
                    else if (currentScreen == SCREEN_SANDBOX) {
                        if (!handlePanPress(sbAvlCam)) handlePanPress(sbHeapCam);
                    } else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) {
                        handlePanPress(demoCam);
                    }
                    continue;
                }
                if (!isLeft) continue;

                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                }
                else if (currentScreen == SCREEN_MAIN_MENU) {
                    for (int i = 0; i < 6; ++i) {
                        float itemY = 300.f + static_cast<float>(i) * 70.f;
                        if (Ui::inside(mx, my, 90.f, itemY - 10.f, 430.f, 54.f)) {
                            playSfx(menuClickSnd);
                            menuSelectedIdx = i;
                            if (i == 0) {
                                currentScreen = SCREEN_CAMPAIGN_MAP;
                                status = "Heist Wings Overview";
                            } else if (i == 1) {
                                startEndlessMode(0u);
                            } else if (i == 2) {
                                enterSandbox();
                            } else if (i == 3) {
                                currentScreen = SCREEN_LEADERBOARD;
                            } else if (i == 4) {
                                currentScreen = SCREEN_SETTINGS;
                            } else if (i == 5) {
                                window.close();
                            }
                            break;
                        }
                    }
                }
                else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    if (demoPromptOpen) {
                        float mw = 460.f, mh = 210.f;
                        float mx0 = (static_cast<float>(W) - mw) * 0.5f;
                        float my0 = (static_cast<float>(H) - mh) * 0.5f;
                        if (Ui::inside(mx, my, mx0 + 36.f, my0 + 148.f, 180.f, 38.f)) {
                            submitDemoPassword();
                        } else if (Ui::inside(mx, my, mx0 + mw - 216.f, my0 + 148.f, 180.f, 38.f)) {
                            demoPromptOpen = false;
                            demoPasswordLen = 0;
                            demoPasswordBuf[0] = '\0';
                        }
                        continue;
                    }
                    if (Ui::inside(mx, my, static_cast<float>(W) * 0.5f - 135.f, static_cast<float>(H) - 54.f, 270.f, 34.f)) {
                        playSfx(campaignClickSnd);
                        demoPromptOpen = true;
                        demoPasswordLen = 0;
                        demoPasswordBuf[0] = '\0';
                        continue;
                    }
                    if (Ui::inside(mx, my, 1350.f, 20.f, 220.f, 38.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                        continue;
                    }
                    for (int a = 0; a < 4; ++a) {
                        float bx = 44.f;
                        float by = 190.f + static_cast<float>(a) * 124.f;
                        if (Ui::inside(mx, my, bx, by, 220.f, 66.f)) {
                            playSfx(campaignClickSnd);
                            selectedActIdx = a;
                            break;
                        }
                    }
                    const float nodeX[5] = { 400.f, 520.f, 640.f, 760.f, 880.f };
                    const float nodeY[5] = { 360.f, 460.f, 295.f, 450.f, 305.f };
                    const float nodeR = 36.f;
                    for (int i = 0; i < 5; ++i) {
                        float dx = mx - nodeX[i];
                        float dy = my - nodeY[i];
                        if (dx * dx + dy * dy <= nodeR * nodeR) {
                            playSfx(campaignClickSnd);
                            selectedLevelInAct = i;
                            int li = selectedActIdx * 5 + i;
                            if (levels[li].unlocked) startLevel(li);
                            else pushToast("Locked! Clear previous level first", TOAST_KIND_INFO);
                            break;
                        }
                    }
                }
                else if (currentScreen == SCREEN_LEADERBOARD) {
                    if (Ui::inside(mx, my, W * 0.5f - 310.f, 770.f, 290.f, 48.f)) {
                        endlessShowLastResult = false;
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                    } else if (Ui::inside(mx, my, W * 0.5f + 20.f, 770.f, 290.f, 48.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                }
                else if (currentScreen == SCREEN_ENDLESS) {
                    if (Ui::inside(mx, my, 670.f, 18.f, 125.f, 34.f)) {
                        endlessPaused = !endlessPaused;
                        pushToast(endlessPaused ? "Endless paused" : "Endless resumed!", TOAST_KIND_WARNING);
                        continue;
                    }
                    if (Ui::inside(mx, my, 805.f, 18.f, 125.f, 34.f)) { startEndlessMode(0u); continue; }
                    if (Ui::inside(mx, my, 940.f, 18.f, 125.f, 34.f)) {
                        endlessSession.stop();
                        endlessPaused = false;
                        doSaveGame();
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                        continue;
                    }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, LOG_Y, LOG_W, LOG_H)) continue;
                    if (Ui::inside(mx, my, 1120.f, 678.f, 428.f, 54.f)) { triggerEndlessExecute(); continue; }
                    if (Ui::inside(mx, my, 1120.f, 748.f, 136.f, 46.f)) { saveEndlessRun(); continue; }
                    if (Ui::inside(mx, my, 1266.f, 748.f, 136.f, 46.f)) { loadEndlessRun(); continue; }
                    if (Ui::inside(mx, my, 1412.f, 748.f, 136.f, 46.f)) {
                        endlessSession.stop();
                        endlessPaused = false;
                        endlessShowLastResult = false;
                        doSaveGame();
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                        continue;
                    }
                    if (endlessCam.containsPixel(mx, my)) {
                        if (endlessCam.checkDoubleClick(mc->position)) {
                            endlessCam.reset();
                            pushToast("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            endlessCam.panning = true;
                            endlessCam.lastPanPixel = mc->position;
                        }
                    }
                }
                else if (currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
                    if (Ui::inside(mx, my, 270.f, 770.f, 250.f, 48.f)) startEndlessMode(0u);
                    else if (Ui::inside(mx, my, 540.f, 770.f, 250.f, 48.f)) startEndlessMode(endlessLastSeed);
                    else if (Ui::inside(mx, my, 810.f, 770.f, 250.f, 48.f)) currentScreen = SCREEN_LEADERBOARD;
                    else if (Ui::inside(mx, my, 1080.f, 770.f, 250.f, 48.f)) currentScreen = SCREEN_MAIN_MENU;
                }
                else if (currentScreen == SCREEN_SETTINGS) {
                    float sy0 = 140.f, bw = 560.f, bh = 52.f, bx = W * 0.5f - bw * 0.5f;
                    if (Ui::inside(mx, my, bx, sy0 + 125.f, bw, bh)) { soundEnabled   = !soundEnabled; resetConfirmPending = false; }
                    if (Ui::inside(mx, my, bx, sy0 + 195.f, bw, bh)) { showNodeBadges = !showNodeBadges; resetConfirmPending = false; }
                    if (Ui::inside(mx, my, bx, sy0 + 265.f, bw, bh)) { showBgGrid     = !showBgGrid; resetConfirmPending = false; }
                    if (Ui::inside(mx, my, bx, sy0 + 335.f, bw, bh)) { autoPlayEvents = !autoPlayEvents; resetConfirmPending = false; }
                    if (Ui::inside(mx, my, bx, sy0 + 405.f, bw, bh)) {
                        targetFps = (targetFps == 60) ? 120 : 60;
                        window.setFramerateLimit(static_cast<unsigned int>(targetFps));
                        resetConfirmPending = false;
                    }
                    if (Ui::inside(mx, my, bx, sy0 + 485.f, bw, bh)) {
                        if (!resetConfirmPending) {
                            resetConfirmPending = true;
                            pushToast("Click Reset Progress again to confirm!", TOAST_KIND_WARNING);
                        } else {
                            resetConfirmPending = false;
                            demoAllUnlocked = false;
                            for (int i = 0; i < TOTAL_LEVELS; ++i) {
                                levels[i].unlocked = (i == 0);
                                levels[i].stars = 0;
                                campCachedUnl[i] = false;
                                campCachedStars[i] = -1;
                                campCachedLabels[i].clear();
                            }
                            avl.clear();
                            endlessSession = EndlessSession();
                            endlessLastSeed = 1337u;
                            endlessLastEntry = EndlessLeaderboardEntry();
                            endlessPaused = false;
                            endlessShowLastResult = false;
                            endlessLastReason = "";
                            doSaveGame();
                            pushToast("Progress reset", TOAST_KIND_SUCCESS);
                        }
                    }
                    if (Ui::inside(mx, my, W * 0.5f - 150.f, 740.f, 300.f, 48.f)) {
                        resetConfirmPending = false;
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                }
                else if (currentScreen == SCREEN_RESULT) {
                    float cardY = 170.f;
                    if (Ui::inside(mx, my, W * 0.5f - 230.f, cardY + 365.f, 210.f, 52.f)) startLevel(currentLevelIdx);
                    if (Ui::inside(mx, my, W * 0.5f + 20.f, cardY + 365.f, 210.f, 52.f)) {
                        if (currentLevelIdx + 1 < TOTAL_LEVELS && levels[currentLevelIdx + 1].unlocked) {
                            startLevel(currentLevelIdx + 1);
                        } else {
                            currentScreen = SCREEN_CAMPAIGN_MAP;
                        }
                    }
                    if (Ui::inside(mx, my, W * 0.5f - 120.f, cardY + 436.f, 240.f, 42.f)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Heist Wings Overview";
                    }
                }
                else if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    if (showingBriefing) {
                        float bh = 420.f, by = H * 0.5f - bh * 0.5f;
                        if (Ui::inside(mx, my, W * 0.5f - 150.f, by + bh - 74.f, 300.f, 48.f)) {
                            showingBriefing = false;
                            playSfx(missionStartSnd);
                        }
                        continue;
                    }
                    if (Ui::inside(mx, my, 722.f, 18.f, 115.f, 34.f)) { useHint(); continue; }
                    if (Ui::inside(mx, my, 850.f, 18.f, 115.f, 34.f)) { performUndo(); continue; }
                    if (Ui::inside(mx, my, 978.f, 18.f, 115.f, 34.f)) {
                        dragDrop.cancel();
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                        continue;
                    }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, LOG_Y, LOG_W, LOG_H)) continue;

                    int lid = levels[currentLevelIdx].id;

                    // Bottom-panel controls for specific levels
                    if (lid == LEVEL_1_1 && act1SeqIdx == 0 && !pendingWin &&
                        Ui::inside(mx, my, 1220.f, 705.f, 310.f, 52.f)) {
                        cycleAct1TravMode();
                        continue;
                    } else if (lid == LEVEL_1_3) {
                        if (Ui::inside(mx, my, 1130.f, 705.f, 60.f, 52.f)) {
                            act1StepperVal = maxInt(1, act1StepperVal - 1);
                            continue;
                        }
                        if (Ui::inside(mx, my, 1360.f, 705.f, 60.f, 52.f)) {
                            act1StepperVal = minInt(20, act1StepperVal + 1);
                            continue;
                        }
                        if (Ui::inside(mx, my, 1430.f, 705.f, 120.f, 52.f)) {
                            submitAct1HeightOrDiameter();
                            continue;
                        }
                    } else if (lid == LEVEL_1_5 && !act1Mirrored && !pendingWin &&
                               Ui::inside(mx, my, 1220.f, 705.f, 310.f, 52.f)) {
                        triggerStructuralAction(1);
                        continue;
                    } else if ((lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 136.f, 52.f)) { handleAct2CompareChoice(1); continue; }
                        if (Ui::inside(mx, my, 1268.f, 705.f, 136.f, 52.f)) { handleAct2CompareChoice(2); continue; }
                        if (Ui::inside(mx, my, 1414.f, 705.f, 136.f, 52.f)) { handleAct2CompareChoice(3); continue; }
                    } else if (lid == LEVEL_2_3 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 136.f, 52.f)) { handleAct2DeleteCase(1); continue; }
                        if (Ui::inside(mx, my, 1268.f, 705.f, 136.f, 52.f)) { handleAct2DeleteCase(2); continue; }
                        if (Ui::inside(mx, my, 1414.f, 705.f, 136.f, 52.f)) { handleAct2DeleteCase(3); continue; }
                    } else if (lid == LEVEL_2_4 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 136.f, 52.f)) { handleAct2RangeDecision(RQ_ACT_REPORT);      continue; }
                        if (Ui::inside(mx, my, 1268.f, 705.f, 136.f, 52.f)) { handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT);  continue; }
                        if (Ui::inside(mx, my, 1414.f, 705.f, 136.f, 52.f)) { handleAct2RangeDecision(RQ_ACT_PRUNE_RIGHT); continue; }
                    } else if (lid == LEVEL_3_2 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 206.f, 52.f)) { handleAct3SingleRotation(1); continue; }
                        if (Ui::inside(mx, my, 1344.f, 705.f, 206.f, 52.f)) { handleAct3SingleRotation(2); continue; }
                    } else if (lid == LEVEL_3_3 && manualDoubleStep == 0 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 206.f, 52.f)) { handleAct3DoubleCase(1); continue; }
                        if (Ui::inside(mx, my, 1344.f, 705.f, 206.f, 52.f)) { handleAct3DoubleCase(2); continue; }
                    } else if (lid == LEVEL_3_4 && !pendingWin) {
                        if (Ui::inside(mx, my, 1038.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("LL");   continue; }
                        if (Ui::inside(mx, my, 1142.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("RR");   continue; }
                        if (Ui::inside(mx, my, 1246.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("LR");   continue; }
                        if (Ui::inside(mx, my, 1350.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("RL");   continue; }
                        if (Ui::inside(mx, my, 1454.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("None"); continue; }
                    } else if (lid == LEVEL_4_5 && !act4Built && Ui::inside(mx, my, 1220.f, 705.f, 310.f, 52.f)) {
                        triggerStructuralAction(1);
                        continue;
                    }

                    // Canvas node/cell hit-testing across all 4 structures
                    const LayoutResult* activeL = nullptr;
                    if (lid >= LEVEL_1_1 && lid <= LEVEL_1_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &btLayout;
                    } else if (lid >= LEVEL_2_1 && lid <= LEVEL_2_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &bstLayout;
                    } else if (lid >= LEVEL_3_1 && lid <= LEVEL_3_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &avlLayout;
                    } else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &heapLayout;
                    }

                    if (activeL && levelCam.containsPixel(mx, my)) {
                        sf::Vector2f wClick = window.mapPixelToCoords(mc->position, levelCam.view);
                        if (dragDrop.press(wClick.x, wClick.y)) {
                            continue;
                        }
                        int hitIdx = (lid == LEVEL_1_2 && !act1Reconstructed) ? -1 : activeL->hitTestNode(wClick.x, wClick.y);
                        if (hitIdx >= 0) {
                            handleLevelNodeClick(activeL->nodes[hitIdx].id, activeL->nodes[hitIdx].key);
                        } else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                            int cellIdx = activeL->hitTestCell(wClick.x, wClick.y);
                            if (cellIdx >= 0 && cellIdx < activeL->nodeCount) {
                                handleLevelNodeClick(activeL->nodes[cellIdx].id, activeL->nodes[cellIdx].key);
                            } else {
                                if (levelCam.checkDoubleClick(mc->position)) {
                                    levelCam.reset();
                                    pushToast("Canvas camera reset", TOAST_KIND_SUCCESS);
                                } else {
                                    levelCam.panning = true;
                                    levelCam.lastPanPixel = mc->position;
                                }
                            }
                        } else {
                            if (levelCam.checkDoubleClick(mc->position)) {
                                levelCam.reset();
                                pushToast("Canvas camera reset", TOAST_KIND_SUCCESS);
                            } else {
                                levelCam.panning = true;
                                levelCam.lastPanPixel = mc->position;
                            }
                        }
                    }
                }
                else if (currentScreen == SCREEN_SANDBOX) {
                    if (Ui::inside(mx, my, 1370.f, 26.f, 190.f, 38.f)) { currentScreen = SCREEN_MAIN_MENU; continue; }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, 95.f, LOG_W, 540.f)) continue;
                    if (sbAvlCam.containsPixel(mx, my)) {
                        if (sbAvlCam.checkDoubleClick(mc->position)) {
                            sbAvlCam.reset();
                            pushToast("AVL camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            sbAvlCam.panning = true;
                            sbAvlCam.lastPanPixel = mc->position;
                        }
                    } else if (sbHeapCam.containsPixel(mx, my)) {
                        if (sbHeapCam.checkDoubleClick(mc->position)) {
                            sbHeapCam.reset();
                            pushToast("Heap camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            sbHeapCam.panning = true;
                            sbHeapCam.lastPanPixel = mc->position;
                        }
                    }
                    if (Ui::inside(mx, my, 54.f,   674.f, 190.f, 46.f)) triggerSandboxAction(1);
                    if (Ui::inside(mx, my, 264.f,  674.f, 190.f, 46.f)) triggerSandboxAction(2);
                    if (Ui::inside(mx, my, 474.f,  674.f, 190.f, 46.f)) triggerSandboxAction(3);
                    if (Ui::inside(mx, my, 684.f,  674.f, 200.f, 46.f)) triggerSandboxAction(4);
                    if (Ui::inside(mx, my, 904.f,  674.f, 170.f, 46.f)) performUndo();
                    if (Ui::inside(mx, my, 1094.f, 674.f, 170.f, 46.f)) triggerSandboxAction(5);
                }
                else if (currentScreen == SCREEN_ACT1_DEMO) {
                    if (Ui::inside(mx, my, 1350.f, 26.f, 210.f, 38.f)) { currentScreen = SCREEN_CAMPAIGN_MAP; continue; }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, 110.f, LOG_W, 470.f)) continue;
                    if (demoCam.containsPixel(mx, my)) {
                        const LayoutResult& activeDemoL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : btLayout;
                        sf::Vector2f wClick = window.mapPixelToCoords(mc->position, demoCam.view);
                        int hitIdx = activeDemoL.hitTestNode(wClick.x, wClick.y);
                        if (hitIdx >= 0) {
                            const NodeLayout& clicked = activeDemoL.nodes[hitIdx];
                            selectedNodeId = clicked.id;
                            int pLen = 0;
                            int* p = act1Tree.pathTo(clicked.key, pLen);
                            std::string pStr = "Path to " + std::to_string(clicked.key) + ": ";
                            if (p) {
                                for (int i = 0; i < pLen; ++i) pStr += std::to_string(p[i]) + (i + 1 < pLen ? "->" : "");
                                delete[] p;
                            }
                            act1Hl1 = clicked.key;
                            status = pStr;
                        } else if (demoCam.checkDoubleClick(mc->position)) {
                            demoCam.reset();
                            pushToast("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            demoCam.panning = true;
                            demoCam.lastPanPixel = mc->position;
                        }
                    }
                    if (Ui::inside(mx, my, 54.f,   620.f, 200.f, 44.f)) triggerAct1Action(1);
                    if (Ui::inside(mx, my, 274.f,  620.f, 200.f, 44.f)) triggerAct1Action(2);
                    if (Ui::inside(mx, my, 494.f,  620.f, 210.f, 44.f)) triggerAct1Action(3);
                    if (Ui::inside(mx, my, 724.f,  620.f, 210.f, 44.f)) triggerAct1Action(4);
                    if (Ui::inside(mx, my, 954.f,  620.f, 190.f, 44.f)) triggerAct1Action(5);
                    if (Ui::inside(mx, my, 1164.f, 620.f, 180.f, 44.f)) triggerAct1Action(6);
                    if (Ui::inside(mx, my, 1364.f, 620.f, 170.f, 44.f)) triggerAct1Action(7);
                }
                else if (currentScreen == SCREEN_ACT2_DEMO) {
                    if (Ui::inside(mx, my, 1350.f, 26.f, 210.f, 38.f)) { currentScreen = SCREEN_CAMPAIGN_MAP; continue; }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, 110.f, LOG_W, 470.f)) continue;
                    if (demoCam.containsPixel(mx, my)) {
                        const LayoutResult& activeDemoL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : bstLayout;
                        sf::Vector2f wClick = window.mapPixelToCoords(mc->position, demoCam.view);
                        int hitIdx = activeDemoL.hitTestNode(wClick.x, wClick.y);
                        if (hitIdx >= 0) {
                            const NodeLayout& clicked = activeDemoL.nodes[hitIdx];
                            selectedNodeId = clicked.id;
                            LayoutResult before;
                            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
                            EventBatch batch;
                            act2Bst.search(clicked.key, batch);
                            layoutBstForScreen(true);
                            layoutDirty = true;
                            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
                            int pd = act2Bst.predecessor(clicked.key);
                            int sc = act2Bst.successor(clicked.key);
                            act2Hl1 = clicked.key;
                            status = "Node " + std::to_string(clicked.key) + " | Pred=" +
                                     (pd != -1 ? std::to_string(pd) : "None") + " | Succ=" +
                                     (sc != -1 ? std::to_string(sc) : "None");
                        } else if (demoCam.checkDoubleClick(mc->position)) {
                            demoCam.reset();
                            pushToast("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            demoCam.panning = true;
                            demoCam.lastPanPixel = mc->position;
                        }
                    }
                    if (Ui::inside(mx, my, 54.f,   620.f, 155.f, 44.f)) triggerAct2Action(1);
                    if (Ui::inside(mx, my, 222.f,  620.f, 155.f, 44.f)) triggerAct2Action(2);
                    if (Ui::inside(mx, my, 390.f,  620.f, 155.f, 44.f)) triggerAct2Action(3);
                    if (Ui::inside(mx, my, 558.f,  620.f, 155.f, 44.f)) triggerAct2Action(4);
                    if (Ui::inside(mx, my, 726.f,  620.f, 155.f, 44.f)) triggerAct2Action(5);
                    if (Ui::inside(mx, my, 894.f,  620.f, 155.f, 44.f)) triggerAct2Action(6);
                    if (Ui::inside(mx, my, 1062.f, 620.f, 145.f, 44.f)) triggerAct2Action(7);
                    if (Ui::inside(mx, my, 1220.f, 620.f, 145.f, 44.f)) triggerAct2Action(8);
                    if (Ui::inside(mx, my, 1378.f, 620.f, 156.f, 44.f)) triggerAct2Action(9);
                }
            }
        }

        if (bgmLoaded) {
            bgm.setPitch(1.0f);
            bool inGameplay = (currentScreen == SCREEN_LEVEL_PLAY || currentScreen == SCREEN_SANDBOX || currentScreen == SCREEN_ENDLESS);
            float targetVol = !soundEnabled ? 0.f : (inGameplay ? 15.f : 35.f);
            bgm.setVolume(targetVol);
        }

        renderFrame(in);
    }

    return 0;
