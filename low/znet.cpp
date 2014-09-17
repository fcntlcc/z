#include "./znet.h"
#include "./zlog.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace z {
namespace low {
namespace net {
;

extern const socket_fd_t NullSocket = -1;

void tcp_socket_set_async(socket_fd_t s) {
    int flags = fcntl(s, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(s, F_SETFL, flags);
}

void tcp_socket_set_sync(socket_fd_t s) {
    int flags = fcntl(s, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(s, F_SETFL, flags);
}

socket_fd_t tcp_create_socket_to(const char *node, short int port, bool async) {
    char port_str[64];
    snprintf(port_str, sizeof(port_str), "%hd", port);

    return tcp_create_socket_to(node, port_str, async);
}

socket_fd_t tcp_create_socket_to(const char *node, const char *service, bool async) {
    if (!(node && service) ) {
        return NullSocket;
    }

    struct addrinfo hint;
    struct addrinfo *res = nullptr;

    memset(&hint, 0, sizeof(hint) );
    hint.ai_family      = AF_INET;
    hint.ai_socktype    = SOCK_STREAM;

    int r = ::getaddrinfo(node, service, &hint, &res);
    if (0 != r) {
        return NullSocket;
    }

    socket_fd_t s = ::socket(res->ai_family, 
                (res->ai_socktype | SOCK_CLOEXEC | ((async) ? SOCK_NONBLOCK : 0)), 
                res->ai_protocol);

    r = ::connect(s, res->ai_addr, res->ai_addrlen);
    if (r == -1 && !(errno = EINPROGRESS || errno == EINTR) ) {
        close(s);
        s = NullSocket;
    }

    ::freeaddrinfo(res);

    return s;
}

socket_fd_t tcp_listen(short int port, int backlog, bool async) {
    char port_str[64];
    snprintf(port_str, sizeof(port_str), "%hd", port);
   
    return tcp_listen(port_str, backlog, async);
}

socket_fd_t tcp_listen(const char *service, int backlog, bool async) {
    struct addrinfo hint;
    struct addrinfo *res = nullptr;

    memset(&hint, 0, sizeof(hint) );
    hint.ai_family      = AF_INET;
    hint.ai_socktype    = SOCK_STREAM;
    hint.ai_flags       = AI_PASSIVE;

    int r = ::getaddrinfo(nullptr, service, &hint, &res);
    if (r != 0) {
        return NullSocket;
    }

    socket_fd_t s = ::socket(res->ai_family, res->ai_socktype | SOCK_CLOEXEC, res->ai_protocol);
    r = bind(s, res->ai_addr, res->ai_addrlen);
    if (r == -1) {
        ::close(s);
        ::freeaddrinfo(res);
        return NullSocket;
    }
    ::freeaddrinfo(res);

    if (async) {
        tcp_socket_set_async(s);
    }

    r = ::listen(s, backlog);
    if (r == -1) {
        ::close(s);
        return NullSocket;
    }

    return s;
}

socket_fd_t tcp_accept(socket_fd_t listen_fd, bool async, network_peer_t *peer_info) {
    socket_fd_t s = NullSocket;
    int flags = SOCK_CLOEXEC | ((async) ? SOCK_NONBLOCK : 0);
    for (int i = 0; i < 1024 * 10; ++i) {
        if (peer_info) {
            socklen_t len = sizeof(peer_info->addrinfo);
            s = ::accept4(listen_fd, (struct sockaddr*)&peer_info->addrinfo, &len, flags);
        } else {
            s = ::accept4(listen_fd, nullptr, nullptr, flags);
        }

        if (s == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            } else {
                return NullSocket;
            }
        } else {
            if (peer_info) {
                peer_info->socket = s;
            }

            return s;
        }
    }

    return NullSocket;
}

int tcp_read(socket_fd_t s, void *buffer, uint32_t bytes) {
    ssize_t r = ::read(s, buffer, bytes);
    if (r == -1) {
        return (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) ? 0 : -1;
    }

    return r;
}

int tcp_write(socket_fd_t s, void *buffer, uint32_t bytes) {
    ssize_t r = ::write(s, buffer, bytes);
    if (r == -1) {
        return (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) ? 0 : -1;
    }

    return r;
}

} // namespace net
} // namespace low
} // namespace z



