#include "./sample_servers.h"
#include <string.h>

namespace z {
namespace rpc {
namespace sample {
namespace http {
;

struct HttpRR {
    enum {
        BUF_SIZE = 2048,
    };
    char        buf[BUF_SIZE];
    uint32_t    pos;
    // request
    uint32_t    uri;
    uint32_t    host;
    // response
    uint32_t    resp;
    uint32_t    resp_length;
    uint32_t    wpos;
};

int sample_http_server_op_err(RPCTask *t) {
    ZLOG(LOG_INFO, "err");
    ZLOG(LOG_WARN, "http server error. [fd: %d]",
        t->fd);
    
    return RPC_OP_END;
}

int sample_http_server_op_begin(RPCTask *t) {
    ZLOG(LOG_INFO, "begin");
    if (t->dn || t->dptr) {
        ZLOG(LOG_WARN, "Not null task. [dn: %lu] [dptr: %p]", t->dn, t->dptr);
    }

    HttpRR *h = new HttpRR;
    h->pos = 0;
    h->uri = 0;
    h->host = 0;
    h->resp = 0;
    h->resp_length = 0;
    h->wpos = 0;
    h->buf[0] = 0;
    h->buf[sizeof(h->buf) - 1] = 0;
    
    t->dn = sizeof(HttpRR);
    t->dptr = h;

    return RPC_OP_READ;
}

int sample_http_server_op_read(RPCTask *t) {
    ZLOG(LOG_INFO, "read");
    HttpRR *h = (HttpRR*)t->dptr;

    uint32_t bufsize = h->BUF_SIZE - h->pos - 1;
    ssize_t r = read(t->fd, h->buf + h->pos, bufsize);
    if (r == 0) {
        return RPC_OP_END;
    } else if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return RPC_OP_READ;
        }

        return RPC_OP_END;
    } else { // r > 0
        h->pos += r;

        if (h->pos > 16) {
            if (strncmp("GET ", h->buf, 4) ) {
                return RPC_OP_END;
            }
            h->uri = 5;

            char *p = strchr(h->buf + h->uri, ' ');
            if (!p) {
                return RPC_OP_READ;
            }    
            *p = '\0';

            p = strstr(p + 1, "Host: ");
            if (p) {
                h->host = p - h->buf + 6;
                p = strchr(p + 6, '\r');
                *p = 0;
                return RPC_OP_SCHED;
            }
        }

        return RPC_OP_READ;
    }
}

int sample_http_server_op_sched(RPCTask* /*t*/) {
    ZLOG(LOG_INFO, "sched");
    return rpc_encode_queue_id_for_op_sched(0);
}

int sample_http_server_op_calc(RPCTask *t) {
    ZLOG(LOG_INFO, "calc");
    HttpRR *h = (HttpRR*)t->dptr;
    h->resp = h->uri;
    h->resp_length = strlen(h->buf + h->uri);

    return RPC_OP_WRITE;
}

int sample_http_server_op_write(RPCTask *t) {
    ZLOG(LOG_INFO, "write");
    HttpRR *h = (HttpRR*)t->dptr;

    char out[8192];
    uint32_t len = snprintf(out, sizeof(out),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %u\r\n"
        "\r\n"
        "URI: %s",
        5 + h->resp_length, h->buf + h->resp);
    
    ssize_t w = write(t->fd, out + h->wpos, len - h->wpos);
    if (w == 0) {
        return RPC_OP_WRITE;
    } else if (w < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return RPC_OP_WRITE;
        }
        return RPC_OP_END;
    } else {
        h->wpos += w;
        if (h->wpos >= len) {
            return RPC_OP_END;
        }
        return RPC_OP_WRITE;
    }
}

int sample_http_server_op_end(RPCTask *t) {
    ZLOG(LOG_INFO, "end");
    HttpRR *h = (HttpRR*)t->dptr;
    delete h;

    t->dn = 0;
    t->dptr = NULL;

    return RPC_OP_CLOSE;
}

int sample_http_server_op_close(RPCTask *t) {
    ZLOG(LOG_INFO, "close");
    return rpc_default_op_close(t);
}

RPCServiceHandle * create_sample_http_service(int listen_fd, int thread_num) {
    ZLOG(LOG_INFO, "create");
    RPCServiceHandle *s = new RPCServiceHandle;
    s->listen_socket    = listen_fd;
    s->epoll_fd         = -1;
    memset(s->service_op, 0, sizeof(s->service_op) );
    s->link_count       = 0;
    s->link_max         = 1024;
    s->calc_thread      = thread_num;
    s->task_queue_size  = 1024;
        
    s->task_pool        = NULL;
    s->task_queue       = NULL;
    s->calc_thread_id   = NULL;
    s->calc_thread_arg  = NULL;
    s->flags            = 0;
    s->d                = NULL;

    s->service_op[RPC_OP_ERR] = sample_http_server_op_err;
    s->service_op[RPC_OP_BEGIN] = sample_http_server_op_begin;
    s->service_op[RPC_OP_READ] = sample_http_server_op_read;
    s->service_op[RPC_OP_SCHED] = sample_http_server_op_sched;
    s->service_op[RPC_OP_CALC] = sample_http_server_op_calc;
    s->service_op[RPC_OP_WRITE] = sample_http_server_op_write;
    s->service_op[RPC_OP_END] = sample_http_server_op_end;
    s->service_op[RPC_OP_CLOSE] = sample_http_server_op_close;

    return s;
}

void destroy_sample_http_service(RPCServiceHandle *service) {
    ZLOG(LOG_INFO, "destroy");
    delete service;
}


} // namespace http
} // namespace sample
} // namespace rpc
} // namespace z


