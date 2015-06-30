#include "./server_framework.h"
#include "../low/low.h"
#include <sys/epoll.h>
#include <unistd.h>

namespace z {
namespace rpc {
namespace server {
;

enum {
    ERR_RECHECK_COUNT = 2,
};


enum EPOLL_LOOP_CONF_ENUM {
    RPC_EL_EVPACK_SIZE              = 16,
    RPC_EL_IDLE_WAIT_MS             = 200,
    RPC_EL_EPOLL_ERR_TRIGGER        = 5,
    RPC_EL_EPOLL_ERR_IDLE_SEC       = 3,
};

enum RPC_WORKER_CONF_ENUM {
    RPC_WORKER_IDLE_WAIT_MS         = 20,
};

enum EPOLL_LOOP_FD_TYPE_ENUM {
    RPC_EL_FD_LISTEN        = 0,
    RPC_EL_FD_IO,
    RPC_EL_FD_END_FLAG,
    RPC_EL_FD_UNKNOWN,
};

enum RPC_OP_TYEP_ENUM {
    RPC_CPU_BIND    = 0,
    RPC_IO_BIND     = 1,
    RPC_NO_BIND     = 2,
};

static int g_rpc_op_type[RPC_OP_LIMIT] = {
    RPC_CPU_BIND, // RPC_OP_ERR,
    RPC_NO_BIND,  // RPC_OP_NOOP,
    RPC_CPU_BIND, // RPC_OP_BEGIN,
    RPC_IO_BIND,  // RPC_OP_READ,
    RPC_CPU_BIND, // RPC_OP_SCHED,
    RPC_CPU_BIND, // RPC_OP_CALC,
    RPC_IO_BIND,  // RPC_OP_WRITE,
    RPC_CPU_BIND, // RPC_OP_END,
    RPC_IO_BIND,  // RPC_OP_CLOSE,
};

struct rpc_worker_thread_arg_t {
    RPCServiceHandle    *service;
    uint32_t            thread_id;
};

static void rpc_init_service(RPCServiceHandle *service, int epoll);
static void rpc_deinit_service(RPCServiceHandle *service);

static RPCTask* rpc_build_task(RPCServiceHandle *service, int fd, int fd_type);
static void rpc_destroy_task(RPCServiceHandle *service, RPCTask *task);

static int rpc_poll_task(RPCServiceHandle *service, RPCTask *task, int events);
static int rpc_unpoll_task(RPCServiceHandle *service, RPCTask *task);

static RPCTask* rpc_get_task_from_event(epoll_event &ev);
static int rpc_get_fd_type(epoll_event &ev);

static void rpc_listen_event(int epoll, epoll_event &ev);
static void rpc_io_event(int epoll, epoll_event &ev);
static void rpc_unknown_event(int epoll, epoll_event &ev);

static int rpc_do_op(int op, RPCTask *task); 
static void rpc_do_op_rec(int op, RPCTask *task);
static int rpc_switch_op_polling_stat(int op_prev, int op_next, RPCTask *task);

static int rpc_decode_queue_id_for_op_sched(int sched_ret);

static void* rpc_worker_thread_main(void *arg);

int rpc_run_service(RPCServiceHandle *service) {
    if (!service || service->listen_socket < 0) {
        ZLOG(LOG_FATAL, "Fail to run service. [service: %p] [listen fd: %d]",
            service, service ? service->listen_socket : -123456789);
        return -1;
    }

    // create the epoll, then add the listening fd to it
    int epoll = epoll_create1(EPOLL_CLOEXEC);
    if (-1 == epoll) {
        ZLOG(LOG_FATAL, "Fail to create a epoll. errno: %d (%s)",
            errno, ZSTRERR(errno).c_str() );
        return -2;
    }
    rpc_init_service(service, epoll);
    RPCTask *listen_task = rpc_build_task(service, service->listen_socket, RPC_EL_FD_LISTEN);
    if (NULL == listen_task) {
        ZLOG(LOG_FATAL, "Fail to build the listen task.");
        return -3;
    }
    if (rpc_poll_task(service, listen_task, EPOLLIN) ) {
        ZLOG(LOG_FATAL, "Fail to poll the listen task.");
        return -4;
    }

    // the main loop
    int err_cnt = 0;
    while ( ! (   (service->flags & RPC_FLAG_FORCE_EXIT) 
               || (service->link_count <= 1 && (service->flags & RPC_FLAG_EXIT)) 
              ) 
          )
    {
        epoll_event e[RPC_EL_EVPACK_SIZE];
        int n = epoll_wait(epoll, e, RPC_EL_EVPACK_SIZE, RPC_EL_IDLE_WAIT_MS);
        if (n < 0) {
            if (n == -1 && errno == EINTR) {
                // interrupted by signal
                continue;
            }

            // some error occured.
            ZLOG(LOG_WARN, "IO Epoll error: ret = %d. errno: %d (%s)",
                n, errno, ZSTRERR(errno).c_str() );
            ++err_cnt;
            if (err_cnt > RPC_EL_EPOLL_ERR_TRIGGER) {
                ZLOG(LOG_FATAL, "Too many epoll error, sleep for %d seconds.", 
                    RPC_EL_EPOLL_ERR_IDLE_SEC);
                z::low::time::zsleep_sec(RPC_EL_EPOLL_ERR_IDLE_SEC);
                err_cnt = 0;
            }
        } else if (n == 0) {
            // No event now.
            continue;
        }

        for (int i = 0; i < n; ++i) {
            int type = rpc_get_fd_type(e[i]); 
            switch (type) { 
            case RPC_EL_FD_LISTEN:
                rpc_listen_event(epoll, e[i]);
                break;
            case RPC_EL_FD_IO:
                rpc_io_event(epoll, e[i]);
                break;
            default:
                rpc_unknown_event(epoll, e[i]);
            }
        }
    }

    // stop the epoll
    rpc_unpoll_task(service, listen_task);
    rpc_destroy_task(service, listen_task);
    close(epoll);

    // deinit the service
    rpc_deinit_service(service);

    return 0;
}

int rpc_set_service_flags(RPCServiceHandle *service, int flags) {
    if (service) {
        service->flags |= flags;
        return 0;
    }

    return -1;
}

int rpc_get_service_flags(RPCServiceHandle *service) {
    if (service) {
        return service->flags;
    }

    return 0;
}

static void rpc_init_service(RPCServiceHandle *service, int epoll) {
    if (service->listen_socket < 0) {
        ZLOG(LOG_WARN, "No listen socket! [listen_socket: %d]", service->listen_socket);
    }

    service->epoll_fd = epoll;

    for (int op = 0; op < RPC_OP_LIMIT; ++op) {
        if (NULL == service->service_op[op]) {
            switch (op) {
            case RPC_OP_ERR:
                ZLOG(LOG_INFO, "[service op: ERR] is not set, use the default.");
                service->service_op[op] = rpc_default_op_err;
                break;
            case RPC_OP_NOOP:
                ZLOG(LOG_INFO, "[service op: NOOP] is not set, use the default.");
                service->service_op[op] = rpc_default_op_noop;
                break;
            case RPC_OP_BEGIN:
                ZLOG(LOG_INFO, "[service op: BEGIN] is not set, use the default.");
                service->service_op[op] = rpc_default_op_begin;
                break;
            case RPC_OP_READ:
                ZLOG(LOG_INFO, "[service op: READ] is not set, use the default.");
                service->service_op[op] = rpc_default_op_read;
                break;
            case RPC_OP_SCHED:
                ZLOG(LOG_INFO, "[service op: SCHED] is not set, use the default.");
                service->service_op[op] = rpc_default_op_sched;
                break;
            case RPC_OP_CALC:
                ZLOG(LOG_INFO, "[service op: CALC] is not set, use the default.");
                service->service_op[op] = rpc_default_op_calc;
                break;
            case RPC_OP_WRITE:
                ZLOG(LOG_INFO, "[service op: WRITE] is not set, use the default.");
                service->service_op[op] = rpc_default_op_write;
                break;
            case RPC_OP_END:
                ZLOG(LOG_INFO, "[service op: END] is not set, use the default.");
                service->service_op[op] = rpc_default_op_end;
                break;
            case RPC_OP_CLOSE:
                ZLOG(LOG_INFO, "[service op: CLOSE] is not set, use the default.");
                service->service_op[op] = rpc_default_op_close;
                break;
            default:
                ZLOG(LOG_INFO, "[service op: %d] is not set, use the default op.", op);
                service->service_op[op] = rpc_default_op_noop;
            }
        }
    }

    service->link_count = 0;
    if (service->task_pool) {
        ZLOG(LOG_WARN, "task_pool is not NULL, ignore. create a new one.");
    }
    service->task_pool = new RPCServiceHandle::task_pool_t(1 + service->link_max);

    if (service->task_queue) {
        ZLOG(LOG_WARN, "task_queue is not NULL, ignore. create new queues.");
    }

    if (service->calc_thread_id) {
        ZLOG(LOG_WARN, "calc_thread_id is not NULL, ignore, create new ");
    }
    
    if (service->calc_thread_arg) {
        ZLOG(LOG_WARN, "calc_thread_arg is not NULL, ignore, create new ");
    }

    service->task_queue = NULL;
    service->calc_thread_id = NULL;
    service->calc_thread_arg = NULL;
    if (service->calc_thread > 0) {
        service->task_queue = new RPCServiceHandle::task_queue_t*[service->calc_thread];
        service->calc_thread_id = new pthread_t[service->calc_thread];
        rpc_worker_thread_arg_t *args = new rpc_worker_thread_arg_t[service->calc_thread];
        service->calc_thread_arg = args;
        for (uint32_t i = 0; i < service->calc_thread; ++i) {
            service->task_queue[i] = new RPCServiceHandle::task_queue_t(service->task_queue_size);
            args[i].service     = service;
            args[i].thread_id   = i;
            if (0 != pthread_create(&service->calc_thread_id[i], NULL, 
                        rpc_worker_thread_main, &args[i]) ) {
                ZLOG(LOG_FATAL, "Fail to create worker thread."); 
                assert(0);
            }
        }
    }

    service->flags = 0;
}

static void rpc_deinit_service(RPCServiceHandle *service) {
    if (service->calc_thread > 0) {
        for (uint32_t i = 0; i < service->calc_thread; ++i) {
            pthread_join(service->calc_thread_id[i], NULL);
            delete [] service->task_queue[i];
        }
        delete [] service->task_queue;
        delete [] service->calc_thread_id;
        delete [] ((rpc_worker_thread_arg_t*)(service->calc_thread_arg));

        service->task_queue = NULL;
        service->calc_thread_id = NULL;
        service->calc_thread_arg = NULL;
    }

    delete service->task_pool;
    service->task_pool = NULL;

    service->epoll_fd = -1;
}

static RPCTask* rpc_build_task(RPCServiceHandle *service, int fd, int fd_type) {
    RPCTask *t = service->task_pool->allocate();
    if (NULL == t) {
        return NULL;
    }

    t->fd           = fd;
    t->type         = fd_type;
    t->op_prev      = RPC_OP_BEGIN;
    t->op_next      = RPC_OP_BEGIN;
    t->reserved     = 0;
    t->events       = 0;
    t->buf[0]       = 0;
    t->service      = service;
    t->dn           = 0;
    t->dptr         = 0;

    return t;
}

static void rpc_destroy_task(RPCServiceHandle *service, RPCTask *task) {
    if (task->events) {
        rpc_unpoll_task(service, task);
    }

    close(task->fd);
    task->fd = -1;
    
    if (task->dn || task->dptr) {
        ZLOG(LOG_WARN, "Forget to release the user data in RPCTask? [dn: %lu] [dptr: %p]",
            task->dn, task->dptr);
    }

    task->dn = 0;
    task->dptr = 0;

    service->task_pool->release(task);
}

static int rpc_poll_task(RPCServiceHandle *service, RPCTask *task, int events) {
    epoll_event ev;
    ev.events = events;
    ev.data.ptr = task;

    int r = (task->events) ?
          epoll_ctl(service->epoll_fd, EPOLL_CTL_MOD, task->fd, &ev)
        : epoll_ctl(service->epoll_fd, EPOLL_CTL_ADD, task->fd, &ev);
        
    if (0 == r) {
        task->events |= events;
    } else {
        ZLOG(LOG_WARN, "Fail to poll task. [efd: %d], [fd: %d], [events: %d] [errno: %d, %s]",
            service->epoll_fd, task->fd, events, errno, ZSTRERR(errno).c_str() );
    }

    return r;
}

static int rpc_unpoll_task(RPCServiceHandle *service, RPCTask *task) {
    epoll_event ev;
    ev.events = task->events;
    ev.data.ptr = task;
    
    int r = epoll_ctl(service->epoll_fd, EPOLL_CTL_DEL, task->fd, &ev);

    task->events = 0;
    
    return r;
}

static RPCTask* rpc_get_task_from_event(epoll_event &ev) {
    return (RPCTask*) ev.data.ptr;
}

static int rpc_get_fd_type(epoll_event &ev) {
    RPCTask *t = rpc_get_task_from_event(ev);
    if (NULL == t) {
        return RPC_EL_FD_UNKNOWN;
    }

    return t->type;
}

static void rpc_listen_event(int /*epoll*/, epoll_event &ev) {
    RPCTask *t = rpc_get_task_from_event(ev);
    if (NULL == t) {
        ZLOG(LOG_WARN, "Fail to get task from event.");
        return ;
    }

    if (ev.events & (EPOLLIN | EPOLLPRI) ) {
        // accept a new link
        int fd = z::low::net::tcp_accept(t->fd, true/*async*/, NULL/*&client*/);
        if (fd >= 0) {
            RPCServiceHandle *s = t->service;
            RPCTask *new_task = rpc_build_task(s, fd, RPC_EL_FD_IO);
            if (NULL == new_task) {
                ZLOG(LOG_WARN, "Build task for I/O fd failed. [link: %u] [max: %u]",
                    s->link_count, s->link_max);
                close(fd);
                }

            rpc_do_op_rec(new_task->op_next, new_task);
        } else {
            ZLOG(LOG_WARN, "Accept return %d: errno = %d(%s)", fd, errno, ZSTRERR(errno).c_str() );
        }
    } else {
        ZLOG(LOG_FATAL, "Listen socket(%d) error. stop the service now. errno = %d (%s)", 
            t->fd, errno, ZSTRERR(errno).c_str() );
        rpc_set_service_flags(t->service, RPC_FLAG_EXIT);
    }
}

static void rpc_io_event(int /*epoll*/, epoll_event &ev) {
    RPCTask *t = rpc_get_task_from_event(ev);
    if (NULL == t) {
        ZLOG(LOG_WARN, "Fail to get task from event.");
        return ;
    }

    if (ev.events & (EPOLLERR | EPOLLHUP) ) {
        ZLOG(LOG_DEBUG, "Client %d exit.", t->fd);
        if (RPC_OP_CLOSE != t->op_next) {
            // ERR(CPU) --> END(CPU) --> CLOSE(IO)
            t->op_next = RPC_OP_ERR;
        }
    }

    if ( (RPC_OP_READ == t->op_next) && !(ev.events & (EPOLLIN | EPOLLPRI) ) ) {
        ZLOG(LOG_WARN, "EPOLLIN (%d) expected. but event [%d] occured.", EPOLLIN, ev.events);
        t->op_next = RPC_OP_ERR;
    }
    
    if ( (RPC_OP_WRITE == t->op_next) && !(ev.events & EPOLLOUT) ) {
        ZLOG(LOG_WARN, "EPOLLOUT (%d) expected. but event [%d] occured.", EPOLLOUT, ev.events);
        t->op_next = RPC_OP_ERR;
    }
    
    if (RPC_CPU_BIND == g_rpc_op_type[rpc_do_op(t->op_next, t)]) {
        rpc_do_op_rec(t->op_next, t);
    }
}

static void rpc_unknown_event(int /*epoll*/, epoll_event &ev) {
    RPCTask *t = rpc_get_task_from_event(ev);
    if (NULL == t) {
        ZLOG(LOG_WARN, "Fail to get task from event.");
        return ;
    } 

    ZLOG(LOG_WARN, "Unknown fd/task type. [fd: %d] [type: %d]",
        t->fd, int(t->type) );
}

static int rpc_do_op(int op, RPCTask *task) {
    if (op >= RPC_OP_LIMIT) {
        ZLOG(LOG_WARN, "rcp_op error. [op: %d] >= [limit: %d]",
            op, RPC_OP_LIMIT);
        task->op_next = RPC_OP_ERR;
        return RPC_OP_ERR;
    }

    task->op_prev = task->op_next;
    int op_next = (task->service->service_op[op])(task);

    if (task->op_prev == RPC_OP_SCHED && op_next >= RPC_OP_LIMIT) {
        // the return value of RPC_OP_SCHED is a encoded task queue id 
        int target_queue_id = rpc_decode_queue_id_for_op_sched(op_next);
        if (target_queue_id < 0 || (uint32_t)target_queue_id >= task->service->calc_thread) {
            ZLOG(LOG_FATAL, "Incorrect target queue id: [%d], should in [0, %u)",
                target_queue_id, task->service->calc_thread);
            op_next = RPC_OP_ERR;
            task->op_next = op_next;
        }

        op_next = RPC_OP_NOOP;
        task->op_next = RPC_OP_CALC;
        rpc_unpoll_task(task->service, task);
        task->service->task_queue[target_queue_id]->enqueue(task);
    } else {
        if (op_next >= RPC_OP_LIMIT || op_next < 0) {
            ZLOG(LOG_FATAL, "Incorrect op_next returned from op function. "
                "set op_next = OP_RPC_ERR, [op: %d] [ret_op: %d] [limit: %d]",
                op, op_next, RPC_OP_LIMIT);
            op_next = RPC_OP_ERR;
        }

        task->op_next = op_next;
        
        if (rpc_switch_op_polling_stat(task->op_prev, op_next, task) ) {
            ZLOG(LOG_FATAL, "Fail to switch polling stat. set op_next as RPC_OP_ERR. "
                "[fd: %d] [prev: %d] [next: %d]",
                task->fd, task->op_prev, op_next);

            rpc_unpoll_task(task->service, task);
            op_next = RPC_OP_ERR;
        }
        task->op_next = op_next;
    }

    return op_next;
}

static int rpc_switch_op_polling_stat(int op_prev, int op_next, RPCTask *task) {
    if (op_prev == op_next) {
        return 0;
    }

    /* Assume: RPC_CPU_BIND == 0, RPC_IO_BIND == 1
       
       cpu - cpu:   0b00   = 0    noop
       cpu - io:    0b01   = 1    to poll
       io  - cpu    0b10   = 2    to unpoll
       io  - io     0b11   = 3    unpoll then poll
    */ 
    int switch_flag = (g_rpc_op_type[op_prev] << 1) | g_rpc_op_type[op_next];
    switch (switch_flag) {
    case 0:
        return 0;
    case 1:
        {
            int events = (op_next == RPC_OP_READ) ? EPOLLIN : EPOLLOUT;
            return rpc_poll_task(task->service, task, events);
        }
    case 2:
        {
            if (rpc_unpoll_task(task->service, task) ) {
                // TODO
            }
            
            return 0;
        }
    case 3:
        {
            if (rpc_unpoll_task(task->service, task) ) {
                // TODO
            }
            int events = (op_next == RPC_OP_READ) ? EPOLLIN : EPOLLOUT;
            return rpc_poll_task(task->service, task, events);
        }
    default:
        ZLOG(LOG_FATAL, "impossible here!");
        assert(0);
        return -2;
    }
}

static void rpc_do_op_rec(int op, RPCTask *task) {
    int op_next = op;
    int err_cnt = 0;
    do {
        op_next = rpc_do_op(op_next, task);
        if (task->op_prev == RPC_OP_ERR && task->op_next == RPC_OP_ERR) {
            ++err_cnt;
        }

        if (err_cnt >= ERR_RECHECK_COUNT) {
            err_cnt = 0;
            ZLOG(LOG_WARN, "Recursively call RPC_OP_ERR for %d times now! sleep 3 seconds.", 
                ERR_RECHECK_COUNT);
            z::low::time::zsleep_sec(3);
        }
    } while (RPC_OP_NOOP != op_next && RPC_CPU_BIND == g_rpc_op_type[op_next]);
}

static int rpc_decode_queue_id_for_op_sched(int sched_ret) {
    return sched_ret - RPC_OP_LIMIT;
}

static void* rpc_worker_thread_main(void *arg_) {
    rpc_worker_thread_arg_t *arg = (rpc_worker_thread_arg_t*)(arg_);
    if (NULL == arg) {
        ZLOG(LOG_FATAL, "NULL thread argument.");
        assert(0);
    }

    RPCServiceHandle *service = arg->service;
    if (NULL == service) {
        ZLOG(LOG_FATAL, "NULL service in worker thread.");
        assert(0);
    }

    RPCServiceHandle::task_queue_t *task_queue = service->task_queue[arg->thread_id];
    while (! (service->flags & RPC_FLAG_FORCE_EXIT) ) {
        RPCTask* task = NULL;
        if (task_queue->dequeue(&task) ) {
            if (task->type == RPC_EL_FD_END_FLAG) {
                break;
            }

            rpc_do_op_rec(task->op_next, task);
        }

        if (task_queue->isEmpty() ) {
            z::low::time::zsleep_ms(RPC_WORKER_IDLE_WAIT_MS);
        }
    }

    return NULL;
}

int rpc_encode_queue_id_for_op_sched(int queue_id) {
    if (queue_id < 0) {
        ZLOG(LOG_WARN, "Incorrect queue id, should >=0. [id: %d]", queue_id);
        return RPC_OP_ERR;
    }

    return RPC_OP_LIMIT + queue_id;
}

int rpc_default_op_err(RPCTask *t) {
    rpc_unpoll_task(t->service, t);
    rpc_destroy_task(t->service, t);
    return RPC_OP_NOOP;
}

int rpc_default_op_noop(RPCTask *) {
    return RPC_OP_NOOP;
}

int rpc_default_op_begin(RPCTask *) {
    return RPC_OP_NOOP;
}

int rpc_default_op_read(RPCTask *) {
    return RPC_OP_ERR;
}

int rpc_default_op_sched(RPCTask *) {
    return RPC_OP_ERR;
}

int rpc_default_op_calc(RPCTask *) {
    return RPC_OP_ERR;
}

int rpc_default_op_write(RPCTask *) {
    return RPC_OP_ERR;
}

int rpc_default_op_end(RPCTask *) {
    return RPC_OP_NOOP;
}

int rpc_default_op_close(RPCTask *t) {
    rpc_unpoll_task(t->service, t);
    rpc_destroy_task(t->service, t);
    return RPC_OP_NOOP;
}

} // namespace server
} // namespace rpc
} // namespace z

