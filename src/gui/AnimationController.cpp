#include "gui/AnimationController.h"
#include <cstring>

namespace VaultHeist {

AnimationController::AnimationController()
    : cursor(-1), playing(false), speedMultiplier(1.0f),
      elapsedInEvent(0.f), eventDuration(0.45f), tweenActive(false), tweenT(1.0f) {}

bool AnimationController::isTweenEvent(const Event& e) const {
    if (e.type == EVENT_ROTATE || e.type == EVENT_SWAP_VALUE ||
        e.type == EVENT_HEAP_SWAP || e.type == EVENT_INSERT ||
        e.type == EVENT_UNLINK || e.type == EVENT_EXTRACT_MAX) {
        return true;
    }
    if (e.type == EVENT_VISIT && e.label &&
        (std::strcmp(e.label, "mirror") == 0 || std::strcmp(e.label, "mirror flip") == 0)) {
        return true;
    }
    return false;
}

bool AnimationController::hasPassedStructuralEvent() const {
    if (cursor < 0) return false;
    for (int i = 0; i <= cursor && i < currentBatch.getCount(); ++i) {
        if (isTweenEvent(currentBatch.getEventAt(i))) return true;
    }
    return false;
}

void AnimationController::loadOperation(const EventBatch& batch, const LayoutResult& before,
                                        const LayoutResult& after, bool appendToLog,
                                        bool autoPlay) {
    currentBatch = batch;
    if (appendToLog && !batch.isEmpty()) {
        log.appendBatch(batch);
        currentBatch.setOperationId(log.getBatchCount() - 1);
    }
    prevLayout = before;
    targetLayout = after;
    cursor = currentBatch.isEmpty() ? -1 : 0;
    playing = autoPlay && !currentBatch.isEmpty();
    elapsedInEvent = 0.f;
    tweenActive = (cursor >= 0 && isTweenEvent(getCurrentEvent()));
    tweenT = tweenActive ? 0.0f : 1.0f;
    rebuildRenderLayout();
}

bool AnimationController::stepForward() {
    if (currentBatch.isEmpty()) return false;
    if (tweenActive && tweenT < 1.0f) {
        // Snap current tween to end so step clicks feel responsive
        tweenT = 1.0f;
        tweenActive = false;
        rebuildRenderLayout();
        return true;
    }
    if (cursor + 1 < currentBatch.getCount()) {
        cursor++;
        elapsedInEvent = 0.f;
        tweenActive = isTweenEvent(getCurrentEvent());
        tweenT = tweenActive ? 0.0f : 1.0f;
        rebuildRenderLayout();
        return true;
    }
    playing = false;
    tweenActive = false;
    tweenT = 1.0f;
    elapsedInEvent = eventDuration;
    rebuildRenderLayout();
    return false;
}

void AnimationController::finishInstant() {
    if (currentBatch.isEmpty()) return;
    cursor = currentBatch.getCount() - 1;
    playing = false;
    tweenActive = false;
    tweenT = 1.0f;
    elapsedInEvent = eventDuration;
    rebuildRenderLayout();
}

void AnimationController::clearCurrent() {
    currentBatch.clear();
    cursor = -1;
    playing = false;
    tweenActive = false;
    tweenT = 1.0f;
    elapsedInEvent = 0.f;
    prevLayout.clear();
    targetLayout.clear();
    renderLayout.clear();
}

void AnimationController::clearAll() {
    clearCurrent();
    log.clear();
}

void AnimationController::update(float dt) {
    if (currentBatch.isEmpty()) return;

    if (tweenActive) {
        elapsedInEvent += dt * speedMultiplier;
        tweenT = clamp01(elapsedInEvent / eventDuration);
        if (tweenT >= 1.0f) {
            tweenT = 1.0f;
            tweenActive = false;
        }
        rebuildRenderLayout();
        if (tweenActive) return;
    }

    if (!playing) return;

    elapsedInEvent += dt * speedMultiplier;
    if (elapsedInEvent >= eventDuration) {
        if (cursor + 1 < currentBatch.getCount()) {
            cursor++;
            elapsedInEvent = 0.f;
            tweenActive = isTweenEvent(getCurrentEvent());
            tweenT = tweenActive ? 0.0f : 1.0f;
            rebuildRenderLayout();
        } else {
            elapsedInEvent = eventDuration;
            playing = false;
        }
    }
}

void AnimationController::setPlaying(bool p) {
    if (currentBatch.isEmpty()) {
        playing = false;
        return;
    }
    if (p && isAtEnd()) {
        cursor = 0;
        elapsedInEvent = 0.f;
        tweenActive = isTweenEvent(getCurrentEvent());
        tweenT = tweenActive ? 0.0f : 1.0f;
    }
    playing = p;
    rebuildRenderLayout();
}

void AnimationController::togglePlayPause() {
    setPlaying(!playing);
}

void AnimationController::setSpeed(float s) {
    if (s < 0.25f) s = 0.25f;
    if (s > 3.0f) s = 3.0f;
    speedMultiplier = s;
}

void AnimationController::seekToEvent(int idx) {
    if (currentBatch.isEmpty()) return;
    if (idx < 0) idx = 0;
    if (idx >= currentBatch.getCount()) idx = currentBatch.getCount() - 1;
    cursor = idx;
    elapsedInEvent = eventDuration;
    tweenActive = false;
    tweenT = 1.0f;
    playing = false;
    rebuildRenderLayout();
}

void AnimationController::replayLastBatch() {
    if (currentBatch.isEmpty()) return;
    cursor = 0;
    elapsedInEvent = 0.f;
    tweenActive = isTweenEvent(getCurrentEvent());
    tweenT = tweenActive ? 0.0f : 1.0f;
    playing = true;
    rebuildRenderLayout();
}

Event AnimationController::getCurrentEvent() const {
    if (cursor < 0 || cursor >= currentBatch.getCount()) return Event();
    return currentBatch.getEventAt(cursor);
}

void AnimationController::rebuildRenderLayout() {
    if (currentBatch.isEmpty() || cursor < 0) {
        renderLayout = targetLayout;
        return;
    }

    // Special case: synchronised MaxHeap swap animation (tree + array cells)
    if (targetLayout.structureType == STRUCT_MAX_HEAP && prevLayout.structureType == STRUCT_MAX_HEAP) {
        renderLayout = prevLayout;

        // Apply swaps in order up to `cursor` on a visual copy of nodes & cells
        int activeN = prevLayout.nodeCount;
        int curVals[MAX_LAYOUT_NODES];
        for (int i = 0; i < activeN && i < MAX_LAYOUT_NODES; ++i) {
            curVals[i] = prevLayout.nodes[i].key;
        }

        // Apply all completed swaps strictly before `cursor` (or at `cursor` if tweenT >= 1)
        int limit = tweenActive ? (cursor - 1) : cursor;
        for (int s = 0; s <= limit && s < currentBatch.getCount(); ++s) {
            Event ev = currentBatch.getEventAt(s);
            if (ev.type == EVENT_HEAP_SWAP) {
                int a = ev.indexA, b = ev.indexB;
                if (a >= 0 && a < activeN && b >= 0 && b < activeN) {
                    int tmp = curVals[a];
                    curVals[a] = curVals[b];
                    curVals[b] = tmp;
                }
            }
        }

        // If we already passed EXTRACT_MAX or HEAP_SIZE_CHANGE and finished tweening, show targetLayout
        bool sizeChanged = false;
        for (int s = 0; s <= limit && s < currentBatch.getCount(); ++s) {
            Event ev = currentBatch.getEventAt(s);
            if (ev.type == EVENT_HEAP_SIZE_CHANGE || ev.type == EVENT_EXTRACT_MAX) {
                sizeChanged = true;
                break;
            }
        }
        if (sizeChanged && !tweenActive) {
            renderLayout = targetLayout;
            return;
        }

        // Sync curVals into renderLayout nodes and active cells
        for (int i = 0; i < activeN; ++i) {
            renderLayout.nodes[i].key = curVals[i];
            if (i < renderLayout.cellCount && !renderLayout.cells[i].isSorted) {
                renderLayout.cells[i].value = curVals[i];
            }
        }
        // Recompute violation flags on visual heap
        for (int i = 1; i < activeN; ++i) {
            int p = (i - 1) / 2;
            bool viol = (renderLayout.nodes[i].key > renderLayout.nodes[p].key);
            renderLayout.nodes[i].isViolation = viol;
        }
        for (int i = 0; i < renderLayout.edgeCount; ++i) {
            int p = renderLayout.edges[i].parentId;
            int c = renderLayout.edges[i].childId;
            if (p >= 0 && p < activeN && c >= 0 && c < activeN) {
                renderLayout.edges[i].isViolation = (renderLayout.nodes[p].key < renderLayout.nodes[c].key);
            }
        }

        // Linear tween on active EVENT_HEAP_SWAP(indexA, indexB)
        Event curEv = getCurrentEvent();
        if (curEv.type == EVENT_HEAP_SWAP && tweenActive) {
            int a = curEv.indexA, b = curEv.indexB;
            if (a >= 0 && a < activeN && b >= 0 && b < activeN) {
                float ax = prevLayout.nodes[a].x, ay = prevLayout.nodes[a].y;
                float bx = prevLayout.nodes[b].x, by = prevLayout.nodes[b].y;
                renderLayout.nodes[a].x = lerpFloat(ax, bx, tweenT);
                renderLayout.nodes[a].y = lerpFloat(ay, by, tweenT);
                renderLayout.nodes[b].x = lerpFloat(bx, ax, tweenT);
                renderLayout.nodes[b].y = lerpFloat(by, ay, tweenT);

                if (a < renderLayout.cellCount && b < renderLayout.cellCount &&
                    !renderLayout.cells[a].isSorted && !renderLayout.cells[b].isSorted) {
                    float cax = prevLayout.cells[a].x;
                    float cbx = prevLayout.cells[b].x;
                    renderLayout.cells[a].x = lerpFloat(cax, cbx, tweenT);
                    renderLayout.cells[b].x = lerpFloat(cbx, cax, tweenT);
                }
            }
        }
        return;
    }

    // Tree structures (BinaryTree, BST, AVLTree):
    // Before reaching any structural event, render prevLayout so visits/compares match the pre-op tree.
    if (!hasPassedStructuralEvent() && prevLayout.nodeCount > 0) {
        renderLayout = prevLayout;
        return;
    }

    renderLayout = targetLayout;
    Event cur = getCurrentEvent();
    if (tweenActive && prevLayout.nodeCount > 0 && targetLayout.nodeCount > 0) {
        float t = clamp01(tweenT);
        for (int i = 0; i < renderLayout.nodeCount; ++i) {
            int prevIdx = prevLayout.findNodeIndexById(renderLayout.nodes[i].id);
            if (prevIdx >= 0) {
                renderLayout.nodes[i].x = lerpFloat(prevLayout.nodes[prevIdx].x, targetLayout.nodes[i].x, t);
                renderLayout.nodes[i].y = lerpFloat(prevLayout.nodes[prevIdx].y, targetLayout.nodes[i].y, t);
            } else if (cur.type == EVENT_INSERT && renderLayout.nodes[i].id == cur.nodeId) {
                renderLayout.nodes[i].scale = lerpFloat(0.25f, 1.0f, t);
                renderLayout.nodes[i].alpha = lerpFloat(0.15f, 1.0f, t);
            }
        }
        for (int i = 0; i < renderLayout.edgeCount; ++i) {
            int pIdx = renderLayout.findNodeIndexById(renderLayout.edges[i].parentId);
            int cIdx = renderLayout.findNodeIndexById(renderLayout.edges[i].childId);
            if (pIdx >= 0 && cIdx >= 0) {
                renderLayout.edges[i].x1 = renderLayout.nodes[pIdx].x;
                renderLayout.edges[i].y1 = renderLayout.nodes[pIdx].y;
                renderLayout.edges[i].x2 = renderLayout.nodes[cIdx].x;
                renderLayout.edges[i].y2 = renderLayout.nodes[cIdx].y;
            }
        }
    }
}

void AnimationController::fillHighlightState(RenderHighlightState& hl) const {
    if (currentBatch.isEmpty() || cursor < 0) return;
    Event e = getCurrentEvent();

    if (e.type == EVENT_VISIT) {
        hl.primaryNodeId = e.nodeId;
        hl.primaryStyle = HL_VISIT;
    } else if (e.type == EVENT_COMPARE) {
        hl.primaryNodeId = e.nodeId;
        hl.primaryStyle = HL_COMPARE;
    } else if (e.type == EVENT_INSERT) {
        hl.primaryNodeId = e.nodeId;
        hl.secondaryNodeId = e.otherNodeId;
        hl.primaryStyle = HL_INSERT;
    } else if (e.type == EVENT_UNLINK || e.type == EVENT_EXTRACT_MAX) {
        hl.primaryNodeId = e.nodeId;
        hl.secondaryNodeId = e.otherNodeId;
        hl.primaryStyle = HL_DELETE;
        hl.arrayHl1 = e.indexA;
        hl.arrayHl2 = e.indexB;
    } else if (e.type == EVENT_SWAP_VALUE) {
        hl.primaryNodeId = e.nodeId;
        hl.secondaryNodeId = e.otherNodeId;
        hl.primaryStyle = HL_SWAP;
    } else if (e.type == EVENT_ROTATE || e.type == EVENT_BALANCE_ALERT) {
        hl.primaryNodeId = e.nodeId;
        hl.secondaryNodeId = e.otherNodeId;
        hl.primaryStyle = (e.type == EVENT_BALANCE_ALERT) ? HL_DELETE : HL_SWAP;
    } else if (e.type == EVENT_HIGHLIGHT_SUBTREE) {
        hl.prunedSubtreeId = e.nodeId;
        hl.primaryNodeId = e.nodeId;
        hl.primaryStyle = HL_VISIT;
    } else if (e.type == EVENT_HEAP_SWAP) {
        hl.primaryNodeId = e.indexA;
        hl.secondaryNodeId = e.indexB;
        hl.primaryStyle = HL_SWAP;
        hl.arrayHl1 = e.indexA;
        hl.arrayHl2 = e.indexB;
    } else if (e.type == EVENT_HEAPIFY_START || e.type == EVENT_HEAPIFY_DONE) {
        hl.primaryNodeId = e.indexA;
        hl.primaryStyle = (e.type == EVENT_HEAPIFY_DONE) ? HL_SUCCESS : HL_VISIT;
        hl.arrayHl1 = e.indexA;
    } else if (e.type == EVENT_OPERATION_COMPLETED) {
        hl.primaryStyle = HL_SUCCESS;
    } else if (e.type == EVENT_OPERATION_FAILED) {
        hl.primaryStyle = HL_DELETE;
    }
}

std::string AnimationController::formatEvent(const Event& e) {
    if (e.type < 0) return "";
    std::string s = "#" + std::to_string(e.sequence) + " ";
    switch (e.type) {
        case EVENT_OPERATION_STARTED:   return s + "START: " + e.label;
        case EVENT_VISIT:               return s + "VISIT node #" + std::to_string(e.nodeId) + " (val=" + std::to_string(e.newValue) + ")";
        case EVENT_COMPARE:             return s + "CMP " + std::to_string(e.oldValue) + " vs " + std::to_string(e.newValue);
        case EVENT_INSERT:              return s + "INSERT key=" + std::to_string(e.newValue) + " (node #" + std::to_string(e.nodeId) + ")";
        case EVENT_UNLINK:              return s + "UNLINK #" + std::to_string(e.nodeId) + " (val=" + std::to_string(e.oldValue) + ")";
        case EVENT_SWAP_VALUE:          return s + "SWAP val " + std::to_string(e.oldValue) + " <- " + std::to_string(e.newValue);
        case EVENT_ROTATE: {
            const char* rc = (e.rotationCase == ROTATION_LL) ? "LL" :
                             (e.rotationCase == ROTATION_RR) ? "RR" :
                             (e.rotationCase == ROTATION_LR) ? "LR" :
                             (e.rotationCase == ROTATION_RL) ? "RL" : "ROT";
            return s + "ROTATE [" + rc + "] pivot=" + std::to_string(e.nodeId);
        }
        case EVENT_UPDATE_HEIGHT:       return s + "HEIGHT #" + std::to_string(e.nodeId) + ": " + std::to_string(e.oldValue) + "->" + std::to_string(e.newValue);
        case EVENT_UPDATE_SIZE:         return s + "SIZE #" + std::to_string(e.nodeId) + ": " + std::to_string(e.oldValue) + "->" + std::to_string(e.newValue);
        case EVENT_BALANCE_ALERT:       return s + "BF ALERT node " + std::to_string(e.nodeId) + " BF=" + std::to_string(e.newValue);
        case EVENT_HIGHLIGHT_SUBTREE:   return s + "PRUNE subtree #" + std::to_string(e.nodeId) + " (" + std::to_string(e.newValue) + ")";
        case EVENT_HEAP_BUILD:          return s + "BUILD HEAP n=" + std::to_string(e.newValue);
        case EVENT_HEAP_SWAP:           return s + "SWAP [" + std::to_string(e.indexA) + "](" + std::to_string(e.oldValue) + ") <-> [" + std::to_string(e.indexB) + "](" + std::to_string(e.newValue) + ")";
        case EVENT_HEAPIFY_START:       return s + "HEAPIFY start [" + std::to_string(e.indexA) + "]";
        case EVENT_HEAPIFY_DONE:        return s + "HEAPIFY done [" + std::to_string(e.indexA) + "]";
        case EVENT_EXTRACT_MAX:         return s + "EXTRACT max=" + std::to_string(e.oldValue);
        case EVENT_MARK_SORTED:         return s + "SORTED [" + std::to_string(e.indexA) + "]=" + std::to_string(e.newValue);
        case EVENT_HEAP_SIZE_CHANGE:    return s + "HEAP SIZE " + std::to_string(e.oldValue) + "->" + std::to_string(e.newValue);
        case EVENT_HEAP_SORT_COMPLETE:  return s + "HEAP SORT DONE";
        case EVENT_OPERATION_COMPLETED: return s + "DONE: " + e.label;
        case EVENT_OPERATION_FAILED:    return s + "FAILED: " + e.label;
        default:                        return s + e.label;
    }
}

} // namespace VaultHeist
