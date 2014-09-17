#include "./ds.h"
#include "./multi_thread.h"

namespace z {
namespace low {
namespace ds {
;


} // namespace ds
} // namespace low
} // namespace z

#if Z_COMPILE_FLAG_ENABLE_UT 
#include <gtest/gtest.h>
TEST(ut_low_ds, static_linked_list) {
    using namespace z::low::ds;

    StaticLinkedList<uint32_t> sl(8);
    uint32_t * ptrs[8];

    ASSERT_FALSE(sl.isEmpty() );
    for (uint32_t i = 0; i < 8; ++i) {
        ptrs[i] = sl.allocate();
        ASSERT_NE(nullptr, ptrs[i]);
    }
    ASSERT_TRUE(sl.isEmpty() );

    for (uint32_t i = 0; i < 8; i += 2) {
        sl.release(ptrs[i]);
        ASSERT_FALSE(sl.isEmpty() );
    }

    for (uint32_t i = 1; i < 8; i += 2) {
        sl.release(ptrs[i]);
        ASSERT_FALSE(sl.isEmpty() );
    }

    for (uint32_t i = 0; i < 8; ++i) {
        ptrs[i] = sl.allocate();
        ASSERT_NE(nullptr, ptrs[i]);
    }
    ASSERT_TRUE(sl.isEmpty() );
}


TEST(ut_low_ds, fixed_length_queue) {
    using namespace z::low::ds;

    FixedLengthQueue<uint32_t>   q(8);
    uint32_t v = 0;

    ASSERT_EQ(0, q.count());
    ASSERT_FALSE(q.isFull());
    ASSERT_TRUE(q.isEmpty());
    ASSERT_FALSE(q.dequeue(&v));

    for (uint32_t i = 0; i < 8; ++i) {
        ASSERT_TRUE(q.enqueue(i));
        ASSERT_FALSE(q.isEmpty());
    }
    ASSERT_TRUE(q.isFull());
    ASSERT_EQ(8, q.count());
    ASSERT_FALSE(q.enqueue(0));

    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.dequeue(&v));
        ASSERT_EQ(i, v);
    }
    ASSERT_EQ(4, q.count());
    ASSERT_FALSE(q.isFull());
    ASSERT_FALSE(q.isEmpty());

    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.enqueue(i));
    }
    ASSERT_TRUE(q.isFull());
    ASSERT_EQ(8, q.count());
    ASSERT_FALSE(q.enqueue(0));

    for (uint32_t i = 4; i < 8; ++i) {
        ASSERT_TRUE(q.dequeue(&v));
        ASSERT_EQ(i, v);
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.dequeue(&v));
        ASSERT_EQ(i, v);
    }
    ASSERT_EQ(0, q.count());
    ASSERT_FALSE(q.isFull());
    ASSERT_TRUE(q.isEmpty());
    
    ASSERT_FALSE(q.dequeue(&v));
}

TEST(ut_low_ds, fixed_length_queue_with_fd) {
    using namespace z::low::ds;

    FixedLengthQueueWithFd<uint32_t>   q(8);
    uint32_t v = 0;

    ASSERT_EQ(0, q.count());
    ASSERT_FALSE(q.isFull());
    ASSERT_TRUE(q.isEmpty());
    ASSERT_FALSE(q.dequeue(&v));

    for (uint32_t i = 0; i < 8; ++i) {
        ASSERT_TRUE(q.enqueue(i));
        ASSERT_FALSE(q.isEmpty());
    }
    ASSERT_TRUE(q.isFull());
    ASSERT_EQ(8, q.count());
    ASSERT_FALSE(q.enqueue(0));

    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.dequeue(&v));
        ASSERT_EQ(i, v);
    }
    ASSERT_EQ(4, q.count());
    ASSERT_FALSE(q.isFull());
    ASSERT_FALSE(q.isEmpty());

    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.enqueue(i));
    }
    ASSERT_TRUE(q.isFull());
    ASSERT_EQ(8, q.count());
    ASSERT_FALSE(q.enqueue(0));

    for (uint32_t i = 4; i < 8; ++i) {
        ASSERT_TRUE(q.dequeue(&v));
        ASSERT_EQ(i, v);
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.dequeue(&v));
        ASSERT_EQ(i, v);
    }
    ASSERT_EQ(0, q.count());
    ASSERT_FALSE(q.isFull());
    ASSERT_TRUE(q.isEmpty());

    ASSERT_FALSE(q.dequeue(&v));
}
#endif // Z_COMPILE_FLAG_ENABLE_UT

