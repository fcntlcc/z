#ifndef Z_ZLOW_ZNET_H__
#define Z_ZLOW_ZNET_H__

#include <stdint.h>
#include <netinet/in.h>
namespace z {
namespace low {
namespace net {
;

typedef int socket_fd_t;
extern const socket_fd_t NullSocket;

struct network_peer_t {
    socket_fd_t     socket;
    sockaddr_in     addrinfo;
};

void tcp_socket_set_async(socket_fd_t s);
void tcp_socket_set_sync(socket_fd_t s);

void tcp_socket_set_timeout(socket_fd_t fd, int read_timout_ms, int write_timeout_ms);

socket_fd_t tcp_create_socket_to(const char *host, short int port, bool async = false);

socket_fd_t tcp_create_socket_to(const char *node, const char *service, bool async = false);

socket_fd_t tcp_create_socket_timeout(const char *host, short int port, int timeout_ms = 0);

socket_fd_t tcp_create_socket_timeout(const char *node, const char *service, int timeout_ms = 0);

socket_fd_t tcp_listen(short int port, int backlog = 1024, bool async = false);

socket_fd_t tcp_listen(const char *service, int backlog = 1024, bool async = false);

socket_fd_t tcp_accept(socket_fd_t listen_fd, bool async = false, network_peer_t *peer_info = nullptr);

/**
 * @retval  >=0   bytes read
 * @retval  -1    error
 */
int    tcp_read(socket_fd_t s, void *buffer, uint32_t bytes);

/**
 * @retval >=0    bytes written
 * @retval -1     error
 */
int    tcp_write(socket_fd_t s, void *buffer, uint32_t bytes);


} // namespace net
} // namespace low
} // namespace z


#endif // Z_ZLOW_ZLOW_H__



