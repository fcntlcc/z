#include "./ztime.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>

namespace z {
namespace low {
namespace time {
;

const char * now_local(char * buf, uint32_t bufsize)
{
    if (NULL == buf || 0 == bufsize) {
        return buf;
    }

    struct timespec ts;
    struct tm       now;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &now);
    
    snprintf(buf, bufsize, "%.4d%.2d%.2d %.2d:%.2d:%.2d %.6ld", 
        now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
        now.tm_hour, now.tm_min, now.tm_sec, ts.tv_nsec/1000);

    return buf;
}

const char * now_utc(char * buf, uint32_t bufsize)
{
    if (NULL == buf || 0 == bufsize) {
        return buf;
    }

    struct timespec ts;
    struct tm       now;
    clock_gettime(CLOCK_REALTIME, &ts);
    gmtime_r(&ts.tv_sec, &now);
    
    snprintf(buf, bufsize, "%.4d%.2d%.2d %.2d:%.2d:%.2d %.6ld", 
        now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
        now.tm_hour, now.tm_min, now.tm_sec, ts.tv_nsec/1000);
    
    return buf;
}

ztime_t ztime_now()
{
    ztime_t     t;
    ztime_now(&t);

    return t;
}

void ztime_now(ztime_t * t)
{
    clock_gettime(CLOCK_REALTIME, t);
}

long ztime_length_us(const ztime_t & begin, const ztime_t & end)
{
    return (end.tv_sec - begin.tv_sec) * 1000L * 1000L
         + (end.tv_nsec - begin.tv_nsec) / 1000L;
}

void zsleep_sec(uint32_t seconds) {
    timeval tm = {seconds, 0};
    select(0, NULL, NULL, NULL, &tm);
}

void zsleep_ms(uint32_t milliseconds) {
    timeval tm = {0, milliseconds * 1000};
    if (milliseconds >= 1000) {
        tm.tv_sec   = milliseconds / 1000;
        tm.tv_usec  = (milliseconds % 1000) * 1000;
    }
    select(0, NULL, NULL, NULL, &tm);
}

void zsleep_us(uint32_t microseconds) {
    const long US_PER_SEC = 1000L * 1000L;
    
    timeval tm = {0, microseconds};
    if (microseconds >= US_PER_SEC) {
        tm.tv_sec   = microseconds / US_PER_SEC;
        tm.tv_usec  = microseconds % US_PER_SEC;
    }
    select(0, NULL, NULL, NULL, &tm);
}

void zsleep_ns(uint32_t nanoseconds) {
    const long NS_PER_SEC = 1000L * 1000L * 1000L;

    timespec ts = {0, nanoseconds};
    if (nanoseconds > NS_PER_SEC) {
        ts.tv_sec   = nanoseconds / NS_PER_SEC;
        ts.tv_nsec  = nanoseconds % NS_PER_SEC;
    }
    pselect(0, NULL, NULL, NULL, &ts, NULL);
}


} // namespace time
} // namespace low
} // namespace z

#if Z_COMPILE_FLAG_ENABLE_UT 
#include <gtest/gtest.h>
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
#endif // Z_COMPILE_FLAG_ENABLE_UT

