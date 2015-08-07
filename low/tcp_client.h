#ifndef Z_LOW_TCP_CLIENT_H__
#define Z_LOW_TCP_CLIENT_H__

/**
 * @brief  The connection manager for tcp clients
 */

#include "./znet.h"
#include "./ds.h"
#include "./multi_thread.h"

namespace z {
namespace low {
namespace net {
;

struct TcpLink;
struct TcpInstance;
struct TcpInstanceGroup;

struct TcpLink {
    socket_fd_t     fd;
    uint32_t        flags;

    void            *d;
};

struct TcpInstance {
    typedef z::low::ds::FixedLengthQueue<TcpLink*>       tcp_link_pool_t;

    tcp_link_pool_t         *link_pool;
};

TcpInstance * tcp_connect_to_instance(const char *host, short int port, 
            uint32_t init_link, uint32_t max_link);

TcpLink* tcp_borrow_link_from_instance(TcpInstance *instance, uint32_t timeout_ms = uint32_t(-1) );


} // namespace net
} // namespace low
} // namespace z

#endif // Z_LOW_TCP_CLIENT_H__

