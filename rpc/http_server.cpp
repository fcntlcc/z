#include "./http_server.h"
#include "../low/low.h"
#include <string.h>

namespace z {
namespace rpc {
;

enum HttpMethod {
    HTTP_METHOD_GET     = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_LIMIT,
};

struct HttpRequest {
    int         method;
    char        *host;
    char        *path;
    char        *content;
    uint64_t    content_length;
};

struct HttpResponse {
    int         status;
    char*       content;
    uint64_t    content_length;
};


// ------------------------------------------------------------------------ //

class HttpParser {
};

// ------------------------------------------------------------------------ //

HttpServer::
HttpServer() {}

HttpServer::
~HttpServer() {}

void HttpServer::
prepareTask(TaskBase *task) {
    HttpServerTask *t = static_cast<HttpServerTask*>(task);
    Z_RET_IF_ANY_ZERO_1(t,);

    InternalServer::prepareTask(task);

    char *parser_buf = (char*) t->mempool->malloc(sizeof(HttpParser) );
    t->http_request = (HttpRequest*) t->mempool->malloc(sizeof(HttpRequest) );
    t->http_response = (HttpResponse*) t->mempool->malloc(sizeof(HttpResponse) );
    Z_RET_IF_ANY_ZERO_3(parser_buf, t->http_request, t->http_response,);

    t->http_parser = new(parser_buf) HttpParser;
    t->flag_keepalive = 0;
}

ServerBase::RequestCheckResult HttpServer::
checkRequestBuffer(TaskBase *task) {
    HttpServerTask *t = static_cast<HttpServerTask*>(task);
    Z_RET_IF_ANY_ZERO_1(t, REQ_CK_ERROR);

    if (t->req_buf && t->req_buf->data_size() > 0) {
        char buf[1024 * 64];
        uint32_t bytes = t->req_buf->read(buf, sizeof(buf) - 1, false);
        buf[bytes] = 0;
        if (nullptr == ::strstr(buf, "\r\n\r\n") ) {
            ZLOG(LOG_INFO, "Continue, Now:\n%s\n", buf);
            return REQ_CK_INPROGRESS;
        } else {
            ZLOG(LOG_INFO, "Done, Now:\n%s\n", buf);
            return REQ_CK_DONE;
        }
    }

    return REQ_CK_INPROGRESS;
}

void HttpServer::
processTask(TaskBase *task) {
    HttpServerTask *t = static_cast<HttpServerTask*>(task);
    Z_RET_IF_ANY_ZERO_1(t,);

    char res[] = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=UTF-8\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "{\"ret\": \"OK\"}";


    t->res_buf->write(res, sizeof(res) );
}

void HttpServer::
resetTask(TaskBase *task) {
    HttpServerTask *t = static_cast<HttpServerTask*>(task);
    Z_RET_IF_ANY_ZERO_1(t, );

    if (t->http_parser) {
        t->http_parser->~HttpParser();
    }

    Z_PTR_DO_4(t,
        mempool->free(t->http_response),
        mempool->free(t->http_request),
        http_response = nullptr,
        http_request = nullptr
        );

    InternalServer::resetTask(task);
}


} // namespace rpc
} // namespace z


