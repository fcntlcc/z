#include "./ztime.h"
#include <stdio.h>

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

} // namespace time
} // namespace low
} // namespace z
