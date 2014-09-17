#ifndef Z_LOW_MULTI_THREAD_H__
#define Z_LOW_MULTI_THREAD_H__

#include "./macro_utils.h"
#include "./ztime.h"
#include "./ds.h"
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <sys/epoll.h>

namespace z {
namespace low {
namespace multi_thread {
;

using ::z::low::time::ztime_t;

class ZNoLock {
public:
    bool try_lock() {return true; }
    void lock() {}
    int timed_lock(const ztime_t *) {return 0;};
    void unlock() {}
};

class ZMutexLock {
public:
    Z_DECLARE_DEFAULT_CONS_DES_FUNCTIONS(ZMutexLock);

    bool try_lock();
    void lock();
    int timed_lock(const ztime_t *t); 
    void unlock();
private:
    pthread_mutex_t _mutex;
};

class ZSpinLock {
public:
    Z_DECLARE_DEFAULT_CONS_DES_FUNCTIONS(ZSpinLock);

    bool try_lock();
    void lock();
    int timed_lock(const ztime_t *t); 
    void unlock();
private:
    pthread_spinlock_t  _spinlock;
};

template <typename LOCK>
class ZAutoLocker {
public:
    ZAutoLocker(LOCK *lock) : _lock(lock) {this->lock();}
    ~ZAutoLocker() {this->unlock(); }

    void lock() {if (_lock && !_locked) {_lock->lock(); _locked = true;} }
    void unlock() {if (_lock && _locked) {_lock->unlock(); _locked = false;} }
private:
    LOCK    *_lock;
    bool    _locked;
};


// ------------------------------------------------------------------------ //

class ZThreadTask {
    Z_DECLARE_COPY_FUNCTIONS(ZThreadTask)
public:
    enum TaskStatus {
        INIT        = 0,
        WAITING,
        RUNNING,
        DONE,
    };

    ZThreadTask();
    virtual ~ZThreadTask();

    void reset();
    void next_status();
    TaskStatus status() const;
    bool wait(int timeout_ms = -1);
    void signal_done();

    virtual int exec(void*);
protected:
    int                 _status;
    pthread_mutex_t     _mutex;
    pthread_cond_t      _cond;
};

class ZThreadPool {
    Z_DECLARE_COPY_FUNCTIONS(ZThreadPool)
public:
    ZThreadPool(uint32_t task_queue_size = 1024);
    ~ZThreadPool();
public:
    bool     start(uint32_t thread_num = 2);
    void     stop();
    bool     commit(ZThreadTask *task);
    uint32_t thread_count() const;
    uint32_t waiting_task() const;
private:
    struct ThreadArgs {
        ZThreadPool     *this_ptr;
        uint32_t        info_offset;
    };
    static void* ThreadMain(void *arg);
private:
    struct ThreadInfo {
        pthread_t       id;
        ThreadArgs      args;
        uint32_t        is_running:1;
    };
    typedef ZThreadTask*                                    TaskPtr;
    typedef std::vector<ThreadInfo>                         ThreadInfoList;
    typedef z::low::ds::FixedLengthQueueWithFd<TaskPtr, 
                    z::low::multi_thread::ZSpinLock>        TaskQueue;
    enum ThreadPoolStatus {
        INIT        = 0,
        RUNNING,
        STOP,
        STOPPED,
    };

    int             _status;
    int             _sched_epoll;
    ::epoll_event   _ev;
    ThreadInfoList  _threads;
    TaskQueue       _task_queue;
};

} // namespace multi_thread
} // namespace low
} // namespace z



#endif

