#ifndef Z_ZLOG_H__
#define Z_ZLOG_H__

#include <stdint.h>
#include <string>

namespace z {
;

enum zlog_level_t {
    LOG_LEVEL_BEGIN = 0,

    LOG_MSG    = 0,
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,

    LOG_LEVEL_END,
    LOG_LEVEL_LIMIT = LOG_LEVEL_END
};

int zlog(zlog_level_t log_level, const char * file, uint32_t line, const char * pattern, ...);

std::string zstrerror(int err);

} // namespace z

using z::LOG_MSG;
using z::LOG_FATAL;
using z::LOG_ERROR;
using z::LOG_WARN;
using z::LOG_INFO;
using z::LOG_DEBUG;

#define ZLOG(level, msg, ...) ::z::zlog(level, __BASE_FILE__, __LINE__, msg, ##__VA_ARGS__)

#define ZSTRERR(x) ::z::zstrerror(x)

#define ZLOGPOS ZLOG(LOG_INFO, "~")

#endif
