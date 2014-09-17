#ifndef Z_LOW_DS_H__
#define Z_LOW_DS_H__


/**
 * @brief The Basic Data Structures.
 */

#include "./macro_utils.h"
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

namespace z {
namespace low {
namespace multi_thread {
;
class ZNoLock;
} // namespace multi_thread;

namespace ds {
;

template <typename T, typename LOCK = z::low::multi_thread::ZNoLock>
    class StaticLinkedList {
    private:
        Z_DECLARE_COPY_FUNCTIONS(StaticLinkedList);
        struct ItemWrapper {
            uint32_t    next_ptr;
            T           item;
        };
    public:
        StaticLinkedList(uint32_t max_item_num);
        ~StaticLinkedList();

        T *allocate();
        void release(T *t);

        bool isEmpty() const;
    private:
        void init(uint32_t max_item_num);
        void destroy();
    private:
        ItemWrapper     *_item_pool;
        LOCK            _lock;
    };

template <typename LOCK = z::low::multi_thread::ZNoLock>
class IDPool {
    Z_DECLARE_COPY_FUNCTIONS(IDPool)
public:
    IDPool(uint32_t max_id);
    ~IDPool();

    uint32_t allocate();
    void release(uint32_t id);
private:
    uint32_t    _size;
    uint32_t    *_pool;
    LOCK         _lock;
};

template <typename T, typename LOCK = z::low::multi_thread::ZNoLock>
    class FixedLengthQueue {
    private:
        Z_DECLARE_COPY_FUNCTIONS(FixedLengthQueue);
        struct ItemWrapper {
            T               item;
            ItemWrapper     *next;
        };
    public:
        FixedLengthQueue(uint32_t max_length);
        ~FixedLengthQueue();

        bool enqueue(const T &t);
        bool dequeue(T *t);

        uint32_t count() const;
        bool isFull() const;
        bool isEmpty() const;
    private:
        LOCK                _lock;
        ItemWrapper         *_head;
        ItemWrapper         *_tail;
        uint32_t            _count;
        StaticLinkedList<ItemWrapper> _free_list;
    };

template <typename T, typename LOCK = z::low::multi_thread::ZNoLock>
    class FixedLengthQueueWithFd {
    private:
        Z_DECLARE_COPY_FUNCTIONS(FixedLengthQueueWithFd);
    public:
        FixedLengthQueueWithFd(uint32_t max_length);
        ~FixedLengthQueueWithFd();

        bool enqueue(const T &t);
        bool dequeue(T *t);

        uint32_t count() const {return _queue.count(); }
        bool isFull() const {return _queue.isFull(); }
        bool isEmpty() const {return _queue.isEmpty(); }

        int enqueue_fd() const {return _fds[1];}
        int dequeue_fd() const {return _fds[0];}
    private:
        int                             _fds[2];
        FixedLengthQueue<T, LOCK>       _queue;
    };


// ------------------------------------------------------------------------ //


template <typename T, typename LOCK>
    StaticLinkedList<T, LOCK>::StaticLinkedList(uint32_t max_item_num) : _item_pool(nullptr) {
        init(max_item_num);
    }

template <typename T, typename LOCK>
    StaticLinkedList<T, LOCK>::~StaticLinkedList() {
        destroy();
    }

template <typename T, typename LOCK>
    T *StaticLinkedList<T, LOCK>::allocate() {
        Z_RET_IF_ANY_ZERO_2(_item_pool, _item_pool[0].next_ptr, nullptr);
        _lock.lock();
        ItemWrapper *free_item = &_item_pool[_item_pool[0].next_ptr];
        _item_pool[0].next_ptr = free_item->next_ptr;
        _lock.unlock();
        if (free_item == &_item_pool[0]) {
            return nullptr;
        } else {
            free_item->next_ptr = 0;
            return &free_item->item;
        }
    }

template <typename T, typename LOCK>
    void StaticLinkedList<T, LOCK>::release(T *t) {
        Z_RET_IF_ANY_ZERO_2(t, _item_pool, );
        ItemWrapper *item = Z_FIND_OBJ_BY_MEMBER(ItemWrapper, item, t);
        uint32_t item_id = ((char*)(item) - (char*)(&_item_pool[0])) / sizeof(_item_pool[0]);
        _lock.lock();
        item->next_ptr = _item_pool[0].next_ptr;
        _item_pool[0].next_ptr = item_id;
        _lock.unlock();
    }

template <typename T, typename LOCK>
    bool StaticLinkedList<T, LOCK>::isEmpty() const {
        return (_item_pool[0].next_ptr == 0);
    }

template <typename T, typename LOCK>
    void StaticLinkedList<T, LOCK>::init(uint32_t max_item_num) {
        destroy();
        _item_pool = new ItemWrapper[max_item_num + 1];
        for (uint32_t i = 1; i <= max_item_num; ++i) {
            _item_pool[i].next_ptr = i - 1;
        }
        _item_pool[0].next_ptr = max_item_num;
    }

template <typename T, typename LOCK>
    void StaticLinkedList<T, LOCK>::destroy() {
        delete [] _item_pool;
        _item_pool = nullptr;
    }

template <typename LOCK>
IDPool<LOCK>::IDPool(uint32_t max_id) 
: _size(max_id + 1) {
    _pool = new uint32_t[_size];

    for (uint32_t i = 0; i < _size; ++i) {
        _pool[i] = i + 1;
    }

    _pool[_size - 1] = 0;
}

template <typename LOCK>
IDPool<LOCK>::~IDPool() {
    delete _pool;
    _size = 0;
    _pool = nullptr;
}

template <typename LOCK>
uint32_t IDPool<LOCK>::allocate() {
    if (0 == _pool[0]) {
        return uint32_t(-1);
    }

    _lock.lock();
    uint32_t offset = _pool[0];
    _pool[0] = _pool[offset];
    _pool[offset] = uint32_t(-1);
    _lock.unlock();

    return (offset - 1);
}

template <typename LOCK>
void IDPool<LOCK>::release(uint32_t id) {
    uint32_t offset = id + 1;
    if (offset >= _size || _pool[offset] != uint32_t(-1) ) {
        return ;
    }

    _lock.lock();
    _pool[offset] = _pool[0];
    _pool[0] = offset;
    _lock.unlock();
}



template <typename T, typename LOCK>
    FixedLengthQueue<T, LOCK>::FixedLengthQueue(uint32_t max_length)
    : _head(nullptr), _tail(nullptr), _count(0), _free_list(max_length + 1) {
        _head = _tail = _free_list.allocate();
        _tail->next = nullptr;
    }

template <typename T, typename LOCK>
    FixedLengthQueue<T, LOCK>::~FixedLengthQueue() {}

template <typename T, typename LOCK>
    bool FixedLengthQueue<T, LOCK>::enqueue(const T &t) {
        _lock.lock();
        ItemWrapper * new_item = _free_list.allocate();
        if (new_item) {
            new_item->item = t;
            new_item->next = nullptr;
            _tail->next = new_item;
            _tail = new_item;
            ++_count;
            _lock.unlock();
            return true;
        } else {
            _lock.unlock();
            ZLOG(LOG_DEBUG, "call enqueue on a full queue. count: %u",
                count() );
            return false;
        }
    }

template <typename T, typename LOCK>
    bool FixedLengthQueue<T, LOCK>::dequeue(T *t) {
        Z_RET_IF_ANY_ZERO_1(t, false);
        _lock.lock();
        if (_head->next) {
            ItemWrapper *to_del = _head->next;
            *t = to_del->item;
            _head->next = to_del->next;
            if (to_del == _tail) {
                _tail = _head;
            }
            _free_list.release(to_del);
            --_count;
            _lock.unlock();
            return true;
        } else {
            _lock.unlock();
            return false;
        }
    }

template <typename T, typename LOCK>
    uint32_t FixedLengthQueue<T, LOCK>::count() const {
        return _count;
    }

template <typename T, typename LOCK>
    bool FixedLengthQueue<T, LOCK>::isFull() const {
        return _free_list.isEmpty();
    }

template <typename T, typename LOCK>
    bool FixedLengthQueue<T, LOCK>::isEmpty() const {
        return (count() == 0);
    }

template <typename T, typename LOCK>
    FixedLengthQueueWithFd<T, LOCK>::FixedLengthQueueWithFd(uint32_t max_length)
    : _queue(max_length) {
        int ret = ::pipe2(_fds, O_CLOEXEC | O_NONBLOCK);
        ZASSERT(0 == ret);
    }

template <typename T, typename LOCK>
    FixedLengthQueueWithFd<T, LOCK>::~FixedLengthQueueWithFd() {
        ::close(_fds[0]);
        ::close(_fds[1]);
    }

template <typename T, typename LOCK>
    bool FixedLengthQueueWithFd<T, LOCK>::enqueue(const T &t) {
        if (_queue.enqueue(t) ) {
            for (;;) {
                int ret = ::write(enqueue_fd(), "\1", 1);
                if (ret > 0) {
                    break;
                } else if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                    ZASSERT(0);
                }
                ZLOG(LOG_WARN, "blocking enqueue(). sleep 1ms, ret = %d, errno = %d (%s)",
                    ret, errno, ZSTRERR(errno).c_str());
                ZLOG(LOG_WARN, "AG=%d, WB=%d, INT=%d", EAGAIN, EWOULDBLOCK, EINTR);
                usleep(1000);
            }
            return true;
        }

        return false;
    }

template <typename T, typename LOCK>
    bool FixedLengthQueueWithFd<T, LOCK>::dequeue(T *t) {
        char buf = 0;
        for (;;) {
            int ret = ::read(dequeue_fd(), &buf, 1);
            if (    (ret == 0)
                ||  (ret == -1 && errno != EINTR) )
            {
                /*
                ZLOG(LOG_DEBUG, "pipe not ready now, maybe too busy, ret = %d, error = %d (%s)",
                    ret, errno, ZSTRERR(errno).c_str() );
                */
                return false;
            } else if (ret > 0) {
                break;
            }
            ZLOG(LOG_DEBUG, "blocking dequeue(). ret = %d, errno = %d (%s)",
                ret, errno, ZSTRERR(errno).c_str());
        }

        if (_queue.dequeue(t) ) {
            return true;
        }

        ZLOG(LOG_WARN, "Logic error. some data lost...");
        return false;
    }


} // namespace ds
} // namespace low
} // namespace z



#endif

