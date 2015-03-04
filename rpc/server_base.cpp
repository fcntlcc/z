#include "./server_base.h"
#include "../low/low.h"
#include <new>

namespace z {
namespace rpc {
;

ServerBase::~ServerBase() {}

void ServerBase::prepareTask(TaskBase *task) {
    Z_RET_IF_ANY_ZERO_2(task, task->mempool,);
    Z_RET_IF(task->socket == -1,);

    void *mem_req = task->mempool->malloc(sizeof(z::low::mem::RWBuffer) );
    void *mem_res = task->mempool->malloc(sizeof(z::low::mem::RWBuffer) );
    Z_RET_IF_ANY_ZERO_2(mem_req, mem_res,);

    task->req_buf = new (mem_req) z::low::mem::RWBuffer(task->mempool, 1024 * 16u);
    task->res_buf = new (mem_res) z::low::mem::RWBuffer(task->mempool, 1024 * 16u);

    task->flag_keepalive = 1;
    task->flag_reserved  = 0;
}

void ServerBase::resetTask(TaskBase *task) {
    Z_RET_IF_ANY_ZERO_2(task, task->mempool,);
    
    Z_PTR_DO_7(task,
        req_buf->z::low::mem::RWBuffer::~RWBuffer(),
        res_buf->z::low::mem::RWBuffer::~RWBuffer(),
        mempool->free(task->req_buf),
        mempool->free(task->res_buf),
        req_buf = nullptr,
        res_buf = nullptr,
        mempool->reset()
        );
}

} // namespace rpc
} // namespace z

