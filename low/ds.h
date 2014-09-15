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
        ItemWrapper *item = Z_FIND_OBJ_BY_MEMBER(T, item, t);
        uint32_t item_id = (item - &_item_pool[0]) / sizeof(_item_pool[0]);
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

template <typename T, typename LOCK>
    FixedLengthQueue<T, LOCK>::FixedLengthQueue(uint32_t max_length)
    : _head(nullptr), _tail(nullptr), _count(0), _free_list(max_length + 1) {
        _head = _tail = _free_list.allocate();
        _head->next = nullptr;
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
            return false;
        }
    }

template <typename T, typename LOCK>
    bool FixedLengthQueue<T, LOCK>::dequeue(T *t) {
        Z_RET_IF_ANY_ZERO_1(t, false);
        _lock.lock();
        if (_head->next) {
            *t = _head->next->item;
            _head = _head->next;
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
        int ret = ::pipe2(_fds, O_CLOEXEC);
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
            ::write(enqueue_fd(), "\0", 1);
            return true;
        }

        return false;
    }

template <typename T, typename LOCK>
    bool FixedLengthQueueWithFd<T, LOCK>::dequeue(T *t) {
        if (_queue.dequeue(t) ) {
            char buf = 0;
            ::read(dequeue_fd(), &buf, 1);
            return true;
        }

        return false;
    }


} // namespace ds
} // namespace low
} // namespace z



#endif

