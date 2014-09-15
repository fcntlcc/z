#include "./zlog.h"
#include "./ztime.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace z {
namespace low {
namespace log {
;

static const int MAX_LOG_SIZE = 1024 * 4;

static const char * g_zlog_level_str[LOG_LEVEL_END - LOG_LEVEL_BEGIN] = {
    "MSG", "FAT", "ERR", "WRN", "INF", "DBG"
};

static const char * short_file_name(const char * file) {
    if (NULL == file || '/' != *file) {
        return file;
    }

    const char * fn = strrchr(file, '/');
    return (fn) ? (fn + 1) : file;
}

int zlog(zlog_level_t log_level, const char * file, uint32_t line, const char * pattern, ...)
{
    using namespace ::z::low::time;

    static pthread_mutex_t  log_print_lock = PTHREAD_MUTEX_INITIALIZER;

    if (log_level < LOG_LEVEL_BEGIN || log_level >= LOG_LEVEL_END) {
        log_level = LOG_DEBUG;
    }

    char tm[ZTIME_STRING_MIN_BUFSIZE];
    char final_content[MAX_LOG_SIZE];
    va_list ap;
    va_start(ap, pattern);
    uint32_t out_end = snprintf(final_content, sizeof(final_content), "[%s] [%s] %5ld [%s:%u] ", 
        g_zlog_level_str[log_level], now_local(tm, sizeof(tm) ), syscall(SYS_gettid), short_file_name(file), line);
    if (out_end < sizeof(final_content) ) {
        out_end += vsnprintf(final_content + out_end, sizeof(final_content) - out_end -1, pattern, ap);
        if (out_end < sizeof(final_content) - 1) {
            final_content[out_end] = '\n';
            final_content[out_end + 1] = 0;
        }
    }
    pthread_mutex_lock(&log_print_lock);
    fputs(final_content, stderr);
    pthread_mutex_unlock(&log_print_lock);

    return 0;
}


std::string zstrerror(int err) {
    char buf[256];
    return strerror_r(err, buf, sizeof(buf) );
}


} // namespace log
} // namespace low
} // namespace z

#if Z_COMPILE_FLAG_ENABLE_UT 
#include <gtest/gtest.h>
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
#endif // Z_COMPILE_FLAG_ENABLE_UT

