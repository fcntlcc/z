#ifndef Z_RPC_HTTP_SERVER_H__
#define Z_RPC_HTTP_SERVER_H__

#include "./internal_server.h"

namespace z {
namespace rpc {
;

class HttpParser;
struct HttpRequest;
struct HttpResponse;

struct HttpServerTask : public InternalServerTask {
    /**
     * From base structures:
     *
     *  z::low::net::socket_fd_t        socket;
     *  z::low::mem::CacheAppendMempool *mempool;
     *  z::low::mem::RWBuffer           *req_buf;
     *  z::low::mem::RWBuffer           *res_buf;
     *
     */
    HttpParser      *http_parser;
    HttpRequest     *http_request;
    HttpResponse    *http_response;
};

class HttpServer : public InternalServer {
public:
    HttpServer();
    ~HttpServer();

    void prepareTask(TaskBase *task);
    RequestCheckResult checkRequestBuffer(TaskBase *task);
    void processTask(TaskBase *task);
    void resetTask(TaskBase *task);
};

} // namespace rpc
} // namespace z

#endif

