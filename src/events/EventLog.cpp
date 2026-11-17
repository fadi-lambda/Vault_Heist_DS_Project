#include "events/EventLog.h"
#include <iostream>

namespace VaultHeist {

EventLog::EventLog() : batches(new EventBatch[4]), capacity(4), count(0), emptyBatch() {}

EventLog::~EventLog() {
    delete[] batches;
    batches = nullptr;
    capacity = 0;
    count = 0;
}

void EventLog::resize(int newCapacity) {
    EventBatch* newBatches = new EventBatch[newCapacity];
    for (int i = 0; i < count; ++i) {
        newBatches[i] = batches[i];
    }
    delete[] batches;
    batches = newBatches;
    capacity = newCapacity;
}

void EventLog::appendBatch(const EventBatch& batch) {
    if (count >= capacity) {
        resize(capacity * 2);
    }
    batches[count] = batch;
    batches[count].setOperationId(count);
    count++;
}

const EventBatch& EventLog::getBatch(int i) const {
    if (i < 0 || i >= count) return emptyBatch;
    return batches[i];
}

int EventLog::getTotalEventCount() const {
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += batches[i].getCount();
    }
    return total;
}

void EventLog::clear() {
    delete[] batches;
    capacity = 4;
    count = 0;
    batches = new EventBatch[capacity];
}

void EventLog::printLog() const {
    std::cout << "EventLog [" << count << " batches]\n";
    for (int b = 0; b < count; ++b) {
        const EventBatch& batch = batches[b];
        std::cout << "  op " << batch.getOperationId() << " (" << batch.getCount() << " events)\n";
        for (int i = 0; i < batch.getCount(); ++i) {
            Event e = batch.getEventAt(i);
            std::cout << "    #" << e.sequence << " type=" << e.type << " " << e.label
                      << " node=" << e.nodeId << " other=" << e.otherNodeId
                      << " old=" << e.oldValue << " new=" << e.newValue
                      << " idx=" << e.indexA << "," << e.indexB
                      << " rot=" << e.rotationCase << "\n";
        }
    }
}

} // namespace VaultHeist
