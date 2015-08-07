#include "./tcp_client.h"
namespace z {
namespace low {
namespace net {
;

TcpInstance* tcp_connect_to_instance(const char *host, short int port,
            uint32_t init_link, uint32_t max_link) {
    if (max_link < init_link) {
        ZLOG(LOG_WARN, "Incorrect link number: [init_link: %u] [max_link: %u]",
            init_link, max_link);
        return NULL;
    }

    TcpInstance * instance = new TcpInstance;
    instance->link_pool = new TcpInstance::tcp_link_pool_t(max_link);
    for (uint32_t i = 0; i < init_link; ++i) {
        socket_fd_t fd = tcp_create_socket_to(host, port);
        if (fd < 0) {
            ZLOG(LOG_WARN, "Can not create more links to [%s:%hd]. [created: %u] "
                "[init_link: %u] [max_link: %u]", host, port, i, init_link, max_link);
            break;
        }
        tcp_socket_set_async(fd);
        TcpLink *link = new TcpLink;
        link->fd        = fd;
        link->flags     = 0;
        link->d         = NULL;
        instance->link_pool->enqueue(link);
    }

    return instance;
}


TcpLink* tcp_borrow_link_from_instance(TcpInstance *instance, uint32_t timeout_ms) {
    if (timeout_ms != uint32_t(-1) ) {
        // TODO: support timeout
        ZLOG(LOG_WARN, "timout is not supported now. [timeout_ms: %u]", timeout_ms);
    }

    if (NULL == instance || NULL == instance->link_pool) {
        ZLOG(LOG_WARN, "No link pool found. [instance: %p] [link_pool: %p]", 
            instance, (instance) ? instance->link_pool : NULL);
        return NULL;
    }
    
    // TODO: borrow link.
    return NULL;
}

} // namespace net
} // namespace low
} // namespace net

