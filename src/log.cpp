#include "log.h"
#include "tm.h"
#include "mem.h"
#include "thread.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace z {
;

static const int MAX_LOG_SIZE = DEF_SIZE_PAGE;

static const char * g_zlog_level_str[LOG_LEVEL_LIMIT] = {
    "MSG", "FAT", "ERR", "WRN", "INF", "DBG"
};

// ------------------------------------------------------------------------ //

class LogController;
class LogDevice;

class LogController {
    Z_DECLARE_COPY_FUNCTIONS(LogController);
public:
    Z_DECLARE_DEFAULT_CONS_DES_FUNCTIONS(LogController);

    bool set_default_log_file(const std::string &path);
    bool set_log_file_for_level(zlog_level_t log_level, const std::string &path);
    bool commit(zlog_level_t log_level, const char *log_msg, size_t log_length);
private:
    bool init();
    void release();
    bool reopen_file(const std::string &path, int *fd);

    bool start_log_thread();
    bool stop_log_thread();

    uint32_t write_log_for_level(zlog_level_t level);
    static void* log_thread_main(void *controller);
private:
    typedef z::BytesQueue               queue_t;
    typedef z::ZSpinLock                lock_t;
    
    struct log_device_t {
        std::string file;
        int         fd;
        queue_t     msg;
        lock_t      lock;
        int         ref_count;
    };

    uint32_t                  _flag_exit:1;
    uint32_t                  _flag_reserved:31;
    
    log_device_t              _default_device;
    log_device_t              _devices[LOG_LEVEL_LIMIT];
    log_device_t              *_level2dev[LOG_LEVEL_LIMIT];

    pthread_t                 _log_thread;
};

LogController::
LogController() {
    init();
    start_log_thread();
}

LogController::
~LogController() {
    stop_log_thread();
    release();
}

bool LogController::
set_default_log_file(const std::string &path) {
    _default_device.file = path;
    return reopen_file(path, &_default_device.fd);
}

bool LogController::
set_log_file_for_level(zlog_level_t log_level, const std::string &path) {
    Z_RET_IF(log_level < LOG_LEVEL_BEGIN || log_level >= LOG_LEVEL_END, false);
   
    log_device_t *old_dev = _level2dev[log_level];
    assert(old_dev);

    log_device_t *new_dev = NULL;
    for (int i = LOG_LEVEL_BEGIN; i < LOG_LEVEL_END; ++i) {
        new_dev = &_devices[i];
        if (0 == path.compare(_devices[i].file) ) {
            break;
        }

        if (_devices[i].fd == -1) {
            break;
        }
    }

    if (new_dev->fd == -1) {
        new_dev->file = path;
        if (!reopen_file(path, &new_dev->fd) ) {
            return false;
        }
    }

    return true;
}

bool LogController::
commit(zlog_level_t log_level, const char *log_msg, size_t log_length) {
    Z_RET_IF(log_level < LOG_LEVEL_BEGIN || log_level >= LOG_LEVEL_END, false);
    log_device_t *pdev = _level2dev[log_level];
    for (;;) {
        pdev->lock.lock();
        if (pdev->msg.in_size() < log_length ) {
            pdev->msg.optimize();
        }

        if (pdev->msg.in_size() < log_length ) {
            pdev->lock.unlock();
            continue;
        }
        memcpy(pdev->msg.in_pos(), log_msg, log_length);
        pdev->msg.commit(log_length);
        pdev->lock.unlock();
        break;
    }

    return true;
}

bool LogController::
init() {
    _flag_exit      = 0;
    _flag_reserved  = 0;

    _default_device.ref_count   = 0;
    _default_device.file        = "stderr";
    _default_device.fd          = ::fileno(stderr);
    for (uint32_t i = LOG_LEVEL_BEGIN; i < LOG_LEVEL_END; ++i) {
        _devices[i].fd          = -1;
        _devices[i].ref_count   = 0;
        _level2dev[i]           = &_default_device;
        ++_default_device.ref_count;
    }

    return true;
}

void LogController::
release() {
    for (uint32_t i = LOG_LEVEL_BEGIN; i < LOG_LEVEL_END; ++i) {
        if (_devices[i].fd != -1) {
            ::close(_devices[i].fd);
        }
    }

    if (_default_device.fd > 2) {
        ::close(_default_device.fd);
    }

    init();
}

bool LogController::
reopen_file(const std::string &path, int *fd) {
    Z_RET_IF_ANY_ZERO_1(fd, false);

    if (*fd > 2) {
        ::close(*fd);
        *fd = -1;
    }

    if (0 == path.compare("stdout") ) {
        *fd = ::fileno(stdout);
        return *fd != -1;
    }
    
    if (0 == path.compare("stderr") ) {
        *fd = ::fileno(stderr);
        return *fd != -1;
    }

    *fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_DIRECT);
    return (*fd != -1);
}

bool LogController::
start_log_thread() {
    _flag_exit = 0;
    int err = ::pthread_create(&_log_thread, NULL, LogController::log_thread_main, this);
    if (err != 0) {
        assert(0); //TODO: report the error instead of coredump.
        return false;
    }

    return true;
}

bool LogController::
stop_log_thread() {
    _flag_exit = 1;
    // TODO: check _log_thread
    ::pthread_join(_log_thread, NULL);
    return false;
}

uint32_t LogController::
write_log_for_level(zlog_level_t level) {
    log_device_t *pdev = _level2dev[level];

    queue_t &msg_q = pdev->msg; 
    if (msg_q.out_size() <= 0) {
        return 0;
    }

    lock_t &lock = pdev->lock;

    lock.lock();
    int bytes = ::write(pdev->fd, msg_q.out_pos(), msg_q.out_size() );
    if (bytes == -1) {
        //TODO report log write error.
        bytes = 0;
    }
    msg_q.consume(bytes);
    lock.unlock();

    return bytes;
}

void *LogController::
log_thread_main(void *controller) {
    LogController *this_ptr = reinterpret_cast<LogController*>(controller);
    Z_RET_IF_ANY_ZERO_1(this_ptr, 0);

    uint32_t bytes = 0;
    while (!this_ptr->_flag_exit) {
        bytes = 0;
        for (int i = LOG_LEVEL_BEGIN; i <= LOG_LEVEL_END; ++i) {
            bytes += this_ptr->write_log_for_level(zlog_level_t(i) );
        }

        if (bytes == 0) {
            timeval tm = {0, 1000 * 20};
            ::select(0, NULL, NULL, NULL, &tm);
        }
    }

    do {
        bytes = 0;
        for (int i = LOG_LEVEL_BEGIN; i <= LOG_LEVEL_END; ++i) {
            bytes += this_ptr->write_log_for_level(zlog_level_t(i) );
        }
    } while (bytes);

    return 0;
}

// ------------------------------------------------------------------------ //

static const char * short_file_name(const char * file) {
    if (NULL == file || '/' != *file) {
        return file;
    }

    const char * fn = strrchr(file, '/');
    return (fn) ? (fn + 1) : file;
}

static LogController    g_global_log_controller;

int zlog(zlog_level_t log_level, const char * file, uint32_t line, const char * pattern, ...)
{
    using namespace ::z;


    if (log_level < LOG_LEVEL_BEGIN || log_level >= LOG_LEVEL_END) {
        log_level = LOG_DEBUG;
    }

    char tm[z::DEF_SIZE_WORD];
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
    g_global_log_controller.commit(log_level, final_content, out_end + 1);

    return 0;
}

std::string zstrerror(int err) {
    char buf[256];
    return strerror_r(err, buf, sizeof(buf) );
}

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

TEST(ut_zlow_zlog, log_controller) {
    using ::z::low::log::g_global_log_controller;

    ::usleep(1000 * 100);
    EXPECT_TRUE(g_global_log_controller.set_default_log_file("stdout") );
    ::usleep(1000 * 100);
    EXPECT_TRUE(g_global_log_controller.commit(LOG_DEBUG, "debug to stdout\n", ::strlen("debug to stdout\n") ) );
    ::usleep(1000 * 100);
    EXPECT_TRUE(g_global_log_controller.set_default_log_file("stderr") );
    ::usleep(1000 * 100);
    EXPECT_TRUE(g_global_log_controller.commit(LOG_DEBUG, "debug to stderr\n", ::strlen("debug to stderr\n") ) );
}

#endif // Z_COMPILE_FLAG_ENABLE_UT
