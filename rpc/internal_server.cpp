#include "./internal_server.h"
#include "../low/low.h"
#include <arpa/inet.h>

namespace z {
namespace rpc {
;

InternalServer::~InternalServer() {}

void InternalServer::prepareTask(TaskBase *task) {
    this->ServerBase::prepareTask(task);
}

InternalServer::RequestCheckResult 
    InternalServer::checkRequestBuffer(TaskBase *task) {
    Z_RET_IF_ANY_ZERO_2(task, task->req_buf, ServerBase::REQ_CK_ERROR);

    int32_t pack_len = 0;
    int32_t r = task->req_buf->read(&pack_len, sizeof(pack_len), false/*do not inc pos*/);
    Z_RET_IF(uint32_t(r) < sizeof(pack_len), ServerBase::REQ_CK_INPROGRESS);

    pack_len= ntohl(pack_len);
    Z_RET_IF(pack_len< 0, ServerBase::REQ_CK_ERROR);

    uint32_t pack_size = task->req_buf->data_size();
    Z_RET_IF(pack_size < pack_len + sizeof(pack_len), ServerBase::REQ_CK_INPROGRESS);

    return ServerBase::REQ_CK_DONE;
}

void InternalServer::processTask(TaskBase *task) {
    Z_RET_IF_ANY_ZERO_2(task, task->res_buf,);
}

void InternalServer::resetTask(TaskBase *task) {
    this->ServerBase::resetTask(task);
}

InternalEchoServer::InternalEchoServer() {}

InternalEchoServer::~InternalEchoServer() {}

void InternalEchoServer::processTask(TaskBase *task) {
    Z_RET_IF_ANY_ZERO_3(task, task->req_buf, task->res_buf, );

    void *req = nullptr;
    uint32_t req_len = 0;
/*    
    task->req_buf->block_ref(&req, &req_len);
    ZLOG(LOG_INFO, "REQ[size: %u] [HEAD 8][%lx]", 
        task->req_buf->data_size(), *((uint64_t*)(req)));
*/
    for (;;) {
        task->req_buf->block_read(&req, &req_len);
        if (req_len == 0) {
            break;
        }

        task->res_buf->write(req, req_len);
    }
/*
    task->res_buf->block_ref(&req, &req_len);
    ZLOG(LOG_INFO, "RES[size: %u] [HEAD 8][%lx]", 
        task->res_buf->data_size(), *((uint64_t*)(req)));
*/
}


} // namespace rpc
} // namespace z

