#include "./multi_thread.h"
#include <string.h>
#include <errno.h>

namespace z {
namespace low {
namespace multi_thread {
;

ZMutexLock::ZMutexLock() {
    ZASSERT(0 == pthread_mutex_init(&_mutex, nullptr));
}

ZMutexLock::~ZMutexLock() {
    ZASSERT(0 == pthread_mutex_destroy(&_mutex));
}

bool ZMutexLock::try_lock() {
    return 0 == pthread_mutex_trylock(&_mutex);
}

void ZMutexLock::lock() {
    pthread_mutex_lock(&_mutex);
}

void ZMutexLock::unlock() {
    pthread_mutex_unlock(&_mutex);
}

ZSpinLock::ZSpinLock() {
    ZASSERT(0 == pthread_spin_init(&_spinlock, PTHREAD_PROCESS_SHARED));
}

ZSpinLock::~ZSpinLock() {
    ZASSERT(0 == pthread_spin_destroy(&_spinlock));
}

bool ZSpinLock::try_lock() {
    return 0 == pthread_spin_lock(&_spinlock);
}

void ZSpinLock::lock() {
    pthread_spin_lock(&_spinlock);
}

void ZSpinLock::unlock() {
    pthread_spin_unlock(&_spinlock);
}


// ------------------------------------------------------------------------ //

ZThreadTask::ZThreadTask()
: _status(TaskStatus::INIT) {
    ZASSERT(0 == pthread_mutex_init(&_mutex, nullptr));
    ZASSERT(0 == pthread_cond_init(&_cond, nullptr));
    reset();
}

ZThreadTask::~ZThreadTask() {
    _status = TaskStatus::DONE;
    ZASSERT(0 == pthread_mutex_destroy(&_mutex));
    ZASSERT(0 == pthread_cond_destroy(&_cond));
}

void ZThreadTask::reset() {
    _status = TaskStatus::INIT;
}

ZThreadTask::TaskStatus ZThreadTask::status() const {
    return TaskStatus(_status);
}

void ZThreadTask::next_status() {
    if (_status >= TaskStatus::DONE) {
        return ;
    }

    ++_status;
}

bool ZThreadTask::wait(int timeout_ms) {
    if (_status == TaskStatus::DONE) {
        return true;
    }

    ::timespec tm = {timeout_ms / 1000, (timeout_ms % 1000) * 1000l * 1000l};
    pthread_mutex_lock(&_mutex);
    while (_status != TaskStatus::DONE) {
        if (ETIMEDOUT == pthread_cond_timedwait(&_cond, &_mutex, &tm) ) {
            break;
        }
    }
    pthread_mutex_unlock(&_mutex);

    return _status == TaskStatus::DONE;
}

void ZThreadTask::signal_done() {
    pthread_mutex_lock(&_mutex);
    _status = TaskStatus::DONE;
    pthread_mutex_unlock(&_mutex);
    pthread_cond_broadcast(&_cond);   
}

int ZThreadTask::exec(void* ) {
    return 0;
}

ZThreadPool::ZThreadPool(uint32_t task_queue_size)
: _status(ThreadPoolStatus::INIT), _sched_epoll(-1), _task_queue(task_queue_size) {
    _sched_epoll = ::epoll_create1(EPOLL_CLOEXEC);
    ZASSERT(_sched_epoll != -1);
    _ev.events = EPOLLIN | EPOLLONESHOT;
    _ev.data.ptr = nullptr;

    ZASSERT(0 == epoll_ctl(_sched_epoll, EPOLL_CTL_ADD, _task_queue.dequeue_fd(), &_ev) );
}

ZThreadPool::~ZThreadPool() {
    stop();
    ::close(_sched_epoll);
}

bool ZThreadPool::start(uint32_t thread_num) {
    Z_RET_IF(_status != INIT || thread_num == 0, false);
    _threads.resize(thread_num);
    for (uint32_t i = 0; i < thread_num; ++i) {
        ThreadInfo & info = _threads[i];
        info.is_running = 1;
        info.args.this_ptr = this;
        info.args.info_offset = i;
        if (0 != pthread_create(&info.id, nullptr, &ZThreadPool::ThreadMain, &info.args) ) {
            info.is_running = 0;
        }
    }
    _status = ThreadPoolStatus::RUNNING;
    return true;
}

void ZThreadPool::stop() {
    Z_RET_IF(_status != ThreadPoolStatus::RUNNING, );
    _status = ThreadPoolStatus::STOP;
    for (ThreadInfoList::iterator i = _threads.begin(); i != _threads.end(); ++i) {
        if (i->is_running) {
            pthread_join(i->id, nullptr);
        }
    }

    _status = ThreadPoolStatus::STOPPED;
}

bool ZThreadPool::commit(ZThreadTask *task) {
    Z_RET_IF(task == nullptr 
            || _status != ThreadPoolStatus::RUNNING
            || task->status() != ZThreadTask::INIT, false);
    
    if (_task_queue.enqueue(task) ) {
        task->next_status();
        return true;
    } else {
        return false;
    }
}

uint32_t ZThreadPool::thread_count() const {
    return _threads.size();
}

uint32_t ZThreadPool::waiting_task() const {
    return _task_queue.count();
}

void *ZThreadPool::ThreadMain(void *a) {
    Z_RET_IF_ANY_ZERO_1(a, nullptr);
    ThreadArgs *args = (ThreadArgs*)(a);

    ZThreadPool *this_ptr = args->this_ptr;
    ThreadInfo &info = this_ptr->_threads[args->info_offset];
    info.is_running = true;
    while (this_ptr->_status != ThreadPoolStatus::STOP) {
        epoll_event ev;
        int ret = epoll_wait(this_ptr->_sched_epoll, &ev, 1, 100);
        if (ret < 0) {
            ZLOG(LOG_WARN, "epoll wait error. %d:%d(%s) event:0x%lx", 
                ret, errno, ZSTRERR(errno).c_str(), uint64_t(ev.events) );
            usleep(1000 * 1000);
            continue;
        } else if (ret == 0) {
            continue;
        } 

        if (ev.events | EPOLLIN) {
            ZThreadTask *task = nullptr;
            this_ptr->_task_queue.dequeue(&task);
            ZASSERT(0 == epoll_ctl(this_ptr->_sched_epoll, EPOLL_CTL_MOD, 
                            this_ptr->_task_queue.dequeue_fd(), &this_ptr->_ev) );
            if (task) {
                task->next_status();
                task->exec(nullptr);
                task->next_status();
                task->signal_done();
            } else {
                ZLOG(LOG_WARN, "NO TASK: count:%lu", this_ptr->_task_queue.count() );
            }
        } else {
            ZLOG(LOG_INFO, "FDERROR: 0x%lx, ret=%d, fd=%d", 
                uint64_t(ev.events), ret, this_ptr->_task_queue.dequeue_fd() );
            ZASSERT(0 == epoll_ctl(this_ptr->_sched_epoll, EPOLL_CTL_MOD, 
                            this_ptr->_task_queue.dequeue_fd(), &this_ptr->_ev) );
        }
    }

    info.is_running = false;
    return nullptr;
}



} // namespace multi_thread
} // namespace low
} // namespace z

