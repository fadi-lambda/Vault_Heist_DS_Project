#include <gtest/gtest.h>
#include "events/Event.h"
#include "events/EventLog.h"

using namespace VaultHeist;

// 1. Empty log
TEST(EventLogTest, EmptyLog) {
    EventLog log;
    EXPECT_EQ(log.getBatchCount(), 0);
    EXPECT_EQ(log.getTotalEventCount(), 0);
    // Out-of-range access returns an empty batch, never crashes
    EXPECT_EQ(log.getBatch(0).getCount(), 0);
    EXPECT_EQ(log.getBatch(-1).getCount(), 0);
}

// 2. Append and retrieve batches; operation ids and sequence numbers are stamped
TEST(EventLogTest, AppendAndRetrieve) {
    EventLog log;

    EventBatch a;
    a.append(Event(EVENT_OPERATION_STARTED, "start"));
    Event v(EVENT_VISIT, "visit");
    v.nodeId = 7;
    a.append(v);
    a.append(Event(EVENT_OPERATION_COMPLETED, "done"));

    EventBatch b;
    b.append(Event(EVENT_OPERATION_FAILED, "fail"));

    log.appendBatch(a);
    log.appendBatch(b);

    ASSERT_EQ(log.getBatchCount(), 2);
    EXPECT_EQ(log.getTotalEventCount(), 4);

    const EventBatch& first = log.getBatch(0);
    ASSERT_EQ(first.getCount(), 3);
    EXPECT_EQ(first.getOperationId(), 0);
    EXPECT_EQ(first.getEventAt(0).type, EVENT_OPERATION_STARTED);
    EXPECT_EQ(first.getEventAt(1).type, EVENT_VISIT);
    EXPECT_EQ(first.getEventAt(1).nodeId, 7);
    EXPECT_EQ(first.getEventAt(1).sequence, 1);
    EXPECT_EQ(first.getEventAt(1).operationId, 0);
    EXPECT_EQ(first.getEventAt(2).type, EVENT_OPERATION_COMPLETED);
    // Unused fields stay at the -1 sentinel
    EXPECT_EQ(first.getEventAt(1).otherNodeId, -1);
    EXPECT_EQ(first.getEventAt(1).rotationCase, ROTATION_NONE);

    const EventBatch& second = log.getBatch(1);
    ASSERT_EQ(second.getCount(), 1);
    EXPECT_EQ(second.getOperationId(), 1);
    EXPECT_EQ(second.getEventAt(0).operationId, 1);
    EXPECT_EQ(second.getEventAt(0).type, EVENT_OPERATION_FAILED);

    // Log stores deep copies: mutating the source batch doesn't affect it
    a.clear();
    EXPECT_EQ(log.getBatch(0).getCount(), 3);
}

// 3. Clear empties the log and it stays usable afterwards
TEST(EventLogTest, Clear) {
    EventLog log;
    EventBatch a;
    a.append(Event(EVENT_VISIT, "visit"));
    for (int i = 0; i < 10; ++i) log.appendBatch(a);  // forces internal resize
    EXPECT_EQ(log.getBatchCount(), 10);
    EXPECT_EQ(log.getBatch(9).getOperationId(), 9);

    log.clear();
    EXPECT_EQ(log.getBatchCount(), 0);
    EXPECT_EQ(log.getTotalEventCount(), 0);

    log.appendBatch(a);
    EXPECT_EQ(log.getBatchCount(), 1);
    EXPECT_EQ(log.getBatch(0).getOperationId(), 0);
}

// 4. EventBatch grows past its initial capacity and copies deeply
TEST(EventLogTest, EventBatchGrowthAndCopy) {
    EventBatch batch;
    for (int i = 0; i < 50; ++i) {
        Event e(EVENT_VISIT, "visit");
        e.nodeId = i;
        batch.append(e);
    }
    ASSERT_EQ(batch.getCount(), 50);
    EXPECT_EQ(batch.getEventAt(49).nodeId, 49);
    EXPECT_EQ(batch.getEventAt(49).sequence, 49);
    EXPECT_EQ(batch.getEventAt(50).type, -1);  // out of range -> default
    EXPECT_EQ(batch.countOfType(EVENT_VISIT), 50);

    EventBatch copy(batch);
    EventBatch assigned;
    assigned = batch;
    batch.clear();
    EXPECT_EQ(copy.getCount(), 50);
    EXPECT_EQ(assigned.getCount(), 50);
    EXPECT_EQ(copy.getEventAt(10).nodeId, 10);
}
