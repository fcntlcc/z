#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* /*argv*/[])
{
    fprintf(stdout, "zclient started. [argc: %d]\n", argc);

    const char * host = "127.0.0.1";
    const char * port = "";

    addrinfo * addr = NULL;
    int err = getaddrinfo(host, port, NULL, &addr);
    if (0 != err) {
        fprintf(stderr, "Resolve [host:%s] [service:%s] failed. [errno: %d, errmsg: %s]\n", 
            host, port, err, gai_strerror(err) );
        return -1;
    }

    char buf[256];
    fprintf(stderr, "ADDR: [%s], PORT[%d]\n", 
        inet_ntop(addr->ai_family, &((sockaddr_in*)(addr->ai_addr))->sin_addr, buf, sizeof(buf)), 
        int(((sockaddr_in*)(addr->ai_addr))->sin_port) );

    int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock < 0) {
        fprintf(stderr, "create socket error. [sock: %d], [errno: %d, errmsg: %s]\n",
            sock, errno, strerror(errno) );
        freeaddrinfo(addr);
        return -2;
    }
    
    // fcntl(sock, F_SETFD, O_NONBLOCK | fcntl(sock, F_GETFD) );
    if (   0 != connect(sock, addr->ai_addr, addr->ai_addrlen) 
        && EINPROGRESS != errno )
    {
        fprintf(stderr, "connect error: [errno: %d, errmsg: %s]\n", errno, strerror(errno) );
        freeaddrinfo(addr);
        close(sock);
        return -3;
    }
    
    freeaddrinfo(addr);

    fprintf(stderr, "connect succ.\n");

    const char * get_msg = 
        "GET /cluster=ys&&query=WIFI HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n\r\n";
    write(sock, get_msg, strlen(get_msg) );
    
    fcntl(sock, F_SETFD, O_NONBLOCK | fcntl(sock, F_GETFD) );
    int epoll = epoll_create1(EPOLL_CLOEXEC);
    if (-1 == epoll) {
        fprintf(stderr, "create epoll failed. [errno: %d, errmsg: %s]\n", errno, strerror(errno) );
        close(sock);
    }

    const int EV_LIST_SIZE = 32;
    epoll_event ev, ev_list[EV_LIST_SIZE];

    ev.events   = EPOLLIN | EPOLLRDHUP;
    ev.data.fd  = sock;

    epoll_ctl(epoll, EPOLL_CTL_ADD, sock, &ev);
    while (1) {
        int n = epoll_wait(epoll, ev_list, EV_LIST_SIZE, -1);
        for (int i = 0; i < n; ++i) {
            epoll_event & e = ev_list[i];
            if (e.data.fd == sock) {
                if (e.events & EPOLLIN) {
                    char buf[2048];
                    int ret = read(sock, buf, sizeof(buf) - 1);
                    if (-1 == ret || 0 == ret) {
                        fprintf(stderr, "EXIT [ret = %d]\n", ret);
                        goto exit;
                    }
                    buf[ret] = 0;
                    fprintf(stderr, "RD: [%2.d] [%s]\n", ret, buf);
                } else {
                    fprintf(stderr, "ERR\n");
                    goto exit;
                }
            }
        }
    }
exit:

    close(sock);

    return 0;
}

