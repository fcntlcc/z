#ifndef Z_RPC_INTERNAL_SERVER_H__
#define Z_RPC_INTERNAL_SERVER_H__

#include "./server_base.h"

namespace z {
namespace rpc {
;

struct InternalServerTask : public TaskBase {
};

class InternalServer : public ServerBase {
public:
    virtual ~InternalServer();

    virtual void prepareTask(TaskBase *task);
    virtual RequestCheckResult checkRequestBuffer(TaskBase *task);
    virtual void processTask(TaskBase *task);
    virtual void resetTask(TaskBase *task);
};

class InternalEchoServer : public InternalServer {
public:
    InternalEchoServer();
    ~InternalEchoServer();
public:
    void processTask(TaskBase *task);
};


} // namespace rpc
} // namespace z

#endif

