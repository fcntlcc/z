#ifndef Z_ZLOW_ZLOG_H__
#define Z_ZLOW_ZLOG_H__

#include <stdint.h>

namespace z {
namespace low {
namespace log {
;

enum zlog_level_t {
    LOG_LEVEL_BEGIN = 0,

    LOG_MSG    = 0,
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,

    LOG_LEVEL_END
};

int zlog(zlog_level_t log_level, const char * file, uint32_t line, const char * pattern, ...);

} // namespace log
} // namespace low
} // namespace z

using z::low::log::LOG_MSG;
using z::low::log::LOG_FATAL;
using z::low::log::LOG_ERROR;
using z::low::log::LOG_WARN;
using z::low::log::LOG_INFO;
using z::low::log::LOG_DEBUG;
#define ZLOG(level, msg, ...) ::z::low::log::zlog(level, __BASE_FILE__, __LINE__, msg, ##__VA_ARGS__)

#endif

