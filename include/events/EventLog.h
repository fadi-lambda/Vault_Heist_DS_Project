#pragma once

#include "events/Event.h"

namespace VaultHeist {

/**
 * @brief Minimal store of completed EventBatches (PLAN.md §6.6).
 *
 * Manual dynamic array of EventBatch (no std::vector). No playback cursor,
 * no SFML — nothing consumes it yet besides tests / a future GUI panel.
 * Each appended batch is stored as a deep copy and assigned an operation id
 * equal to its index in the log.
 */
class EventLog {
private:
    EventBatch* batches;
    int capacity;
    int count;
    EventBatch emptyBatch; // returned for out-of-range getBatch()

    void resize(int newCapacity);

public:
    EventLog();
    ~EventLog();

    // Owning raw array — disallow copies to avoid double free.
    EventLog(const EventLog&) = delete;
    EventLog& operator=(const EventLog&) = delete;

    void appendBatch(const EventBatch& batch);
    int getBatchCount() const { return count; }
    const EventBatch& getBatch(int i) const;
    int getTotalEventCount() const;
    void clear();

    // Debug print (matches printInorder()/printHeap() elsewhere)
    void printLog() const;
};

} // namespace VaultHeist
