#ifndef Z_NET_RPC_SERVER_H__
#define Z_NET_RPC_SERVER_H__

#include "net.h"
#include "mem.h"
#include "algo_ds.h"
#include "thread.h"

namespace z {
;

struct RPCServiceHandle;

struct RPCTask {
    enum { RPC_TMP_BUF_SIZE = sizeof(uint64_t) };
    z::socket_fd_t              fd;
    uint32_t                    type:2;
    uint32_t                    op_prev:4;
    uint32_t                    op_next:4;
    uint32_t                    reserved:22;
    uint32_t                    events;
    RPCServiceHandle            *service;       // the global service
    
    char                        buf[RPC_TMP_BUF_SIZE];
    uint64_t                    dn;             // user data of type int
    void                        *dptr;          // user data of type pointer
};

enum RPC_SERVICE_NEXT_OP_ENUM {
    RPC_OP_ERR          = 0,
    RPC_OP_NOOP,
    RPC_OP_BEGIN,
    RPC_OP_READ,
    RPC_OP_SCHED,
    RPC_OP_CALC,
    RPC_OP_WRITE,
    RPC_OP_END,
    RPC_OP_CLOSE,

    RPC_OP_LIMIT
};

int rpc_encode_queue_id_for_op_sched(int queue_id);

typedef int (*rpc_service_func_t) (RPCTask *ptask);

int rpc_default_op_err(RPCTask *t);
int rpc_default_op_noop(RPCTask *t);
int rpc_default_op_begin(RPCTask *t);
int rpc_default_op_read(RPCTask *t);
int rpc_default_op_sched(RPCTask *t);
int rpc_default_op_calc(RPCTask *t);
int rpc_default_op_write(RPCTask *t);
int rpc_default_op_end(RPCTask *t);
int rpc_default_op_close(RPCTask *t);

enum RPC_GLOBAL_FLAGS_ENUM {
    RPC_FLAG_EXIT               = 0x0001,
    RPC_FLAG_FORCE_EXIT         = 0x0002,
};

struct RPCServiceHandle {
    typedef z::socket_fd_t                                  socket_fd_t;
    typedef z::StaticLinkedList<RPCTask>                    task_pool_t;
    typedef z::FixedLengthQueue<RPCTask*, z::ZSpinLock>     task_queue_t;

    socket_fd_t                 listen_socket;
    int                         epoll_fd;
    rpc_service_func_t          service_op[RPC_OP_LIMIT];
    uint32_t                    link_count;
    uint32_t                    link_max;
    uint32_t                    calc_thread;
    uint32_t                    task_queue_size;
    task_pool_t                 *task_pool;
    task_queue_t                **task_queue;
    pthread_t                   *calc_thread_id;
    void                        *calc_thread_arg;

    int                         flags;          // global flags
    
    void                        *d;             // global user data
};

/**
 * @retval 0    OK
 * @retval < 0  ERROR
 */
int rpc_run_service(RPCServiceHandle *service);
int rpc_set_service_flags(RPCServiceHandle *service, int flags);
int rpc_get_service_flags(RPCServiceHandle *service);

} // namespace z

#endif

