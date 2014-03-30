#include <gtest/gtest.h>
#include <gtest/gtest.h>
#include <string.h>
#include "../low/low.h"

TEST(ut_zlow_ztime, time_string) {
    char buf[z::low::time::ZTIME_STRING_MIN_BUFSIZE];
    for (uint32_t i = 0; i < 32; ++i) {
        z::low::time::now_local(buf, sizeof(buf) );
        EXPECT_EQ(24, strlen(buf) ) << buf << std::endl;
    }

    for (uint32_t i = 0; i < 32; ++i) {
        z::low::time::now_utc(buf, sizeof(buf) );
        EXPECT_EQ(24, strlen(buf) ) << buf << std::endl;
    }
}

TEST(ut_zlow_zlog, print_log) {
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_EQ(0, ZLOG(LOG_MSG,   "mylog [#%u] [%s]", i, "content") );
        EXPECT_EQ(0, ZLOG(LOG_FATAL, "mylog [#%u] [%s]", i, "content") );
        EXPECT_EQ(0, ZLOG(LOG_ERROR, "mylog [#%u] [%s]", i, "content") );
        EXPECT_EQ(0, ZLOG(LOG_WARN,  "mylog [#%u] [%s]", i, "content") );
        EXPECT_EQ(0, ZLOG(LOG_INFO,  "mylog [#%u] [%s]", i, "content") );
        EXPECT_EQ(0, ZLOG(LOG_DEBUG, "mylog [#%u] [%s]", i, "content") );
    }
}

