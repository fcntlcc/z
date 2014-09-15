#ifndef Z_RPC_SERVER_BASE_H__
#define Z_RPC_SERVER_BASE_H__

#include "../low/znet.h"
#include "../low/zmem.h"

namespace z {
namespace rpc {
;

struct TaskBase {
    z::low::net::socket_fd_t        socket;
    z::low::mem::CacheAppendMempool *mempool;
    z::low::mem::RWBuffer           *req_buf;
    z::low::mem::RWBuffer           *res_buf;
};

class ServerBase {
public:
    enum RequestCheckResult {
        REQ_CK_DONE         = 0,
        REQ_CK_INPROGRESS,
        REQ_CK_ERROR
    };

    enum ProcessResult {
        PROC_OK             = 0,
        PROC_ERR,
    };

    virtual ~ServerBase() = 0;
    virtual void prepareTask(TaskBase *task);
    virtual RequestCheckResult checkRequestBuffer(TaskBase *task) = 0;
    virtual void processTask(TaskBase *task) = 0;
    virtual void resetTask(TaskBase *task);
}; // ServerBase

} // namespace rpc
} // namespace z

#endif // Z_RPC_SERVER_BASE_H__

