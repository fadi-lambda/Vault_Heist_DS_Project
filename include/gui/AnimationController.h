#pragma once

#include "events/Event.h"
#include "events/EventLog.h"
#include "gui/LayoutEngine.h"
#include "gui/Renderer.h"
#include <string>

namespace VaultHeist {

class AnimationController {
private:
    EventLog log;
    EventBatch currentBatch;
    int cursor;
    bool playing;
    float speedMultiplier;
    float elapsedInEvent;
    float eventDuration;
    bool tweenActive;
    float tweenT;

    LayoutResult prevLayout;
    LayoutResult targetLayout;
    LayoutResult renderLayout;

    static float clamp01(float v) {
        if (v < 0.f) return 0.f;
        if (v > 1.f) return 1.f;
        return v;
    }

    static float lerpFloat(float a, float b, float t) {
        return a + (b - a) * t;
    }

    bool isTweenEvent(const Event& e) const;
    bool hasPassedStructuralEvent() const;
    void rebuildRenderLayout();

public:
    AnimationController();

    void loadOperation(const EventBatch& batch, const LayoutResult& before,
                       const LayoutResult& after, bool appendToLog = true,
                       bool autoPlay = false);

    // Advances to next event (starts linear tween for structural/swap events).
    bool stepForward();

    // Jumps immediately to the final state of the batch.
    void finishInstant();

    // Clears active batch (keeps history in EventLog unless clearAll is called).
    void clearCurrent();
    void clearAll();

    // Playback / tween update (advances active linear tween even when paused)
    void update(float dt);
    void setPlaying(bool p);
    void togglePlayPause();
    bool isPlaying() const { return playing; }
    void setSpeed(float s);
    float getSpeed() const { return speedMultiplier; }

    // Scrubber / replay (Step 8)
    void seekToEvent(int idx);
    void replayLastBatch();

    bool hasEvents() const { return !currentBatch.isEmpty(); }
    bool isAtEnd() const {
        return currentBatch.isEmpty() || (cursor >= currentBatch.getCount() - 1 && !tweenActive);
    }
    int getCursor() const { return cursor; }
    int getEventCount() const { return currentBatch.getCount(); }
    Event getCurrentEvent() const;
    const EventBatch& getBatch() const { return currentBatch; }
    const EventLog& getLog() const { return log; }

    const LayoutResult& getRenderLayout() const { return renderLayout; }

    void fillHighlightState(RenderHighlightState& hl) const;
    static std::string formatEvent(const Event& e);
};

} // namespace VaultHeist
