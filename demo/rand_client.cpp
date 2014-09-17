#include <low/low.h>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
using namespace z::low::multi_thread;
using namespace z::low::net;
using namespace z::low::time;

struct Params {
    std::string     host;
    std::string     port;
    uint32_t        thread_num;
    uint32_t        min_bytes;
    uint32_t        max_bytes;
    uint64_t        req_num;
};

void help_msg(int , char*argv[]) {
    fprintf(stderr, "Usage: %s host port thread_num min_length max_length request_num\n",
        argv[0]);
}

bool parse_params(Params &pa, char* argv[]);
void run(Params &pa);
void* thread_func(void* pa);

bool parse_params(Params &pa, char* argv[]) {
    pa.host             = argv[1];
    pa.port             = argv[2];
    pa.thread_num       = atoi(argv[3]);
    pa.min_bytes        = atoi(argv[4]);
    pa.max_bytes        = atoi(argv[5]);
    pa.req_num          = atol(argv[6]);

    return (pa.thread_num > 0 && pa.min_bytes <= pa.max_bytes  && pa.req_num >= pa.thread_num);
}

void run(Params &pa) {
    pthread_t id[pa.thread_num];
    
    ZLOG(LOG_INFO, "CLIENT STARTING");
    for (uint32_t i = 0; i < pa.thread_num; ++i) {
        pthread_create(&id[i], nullptr, thread_func, &pa);
    }

    ZLOG(LOG_INFO, "CLIENT STARTED");
    for (uint32_t i = 0; i < pa.thread_num; ++i) {
        pthread_join(id[i], nullptr);
    }
    ZLOG(LOG_INFO, "CLIENT DONE");
}

void* thread_func(void* pa_) {
    Params &pa = *((Params*)(pa_));
    ztime_t t = ztime_now();
    uint32_t seed = t.tv_nsec;
    uint32_t range = pa.max_bytes - pa.min_bytes;
    uint32_t req_num = pa.req_num / pa.thread_num;

    char *req_base  = (char*) malloc(pa.max_bytes * 2 + 4);
    char *req = req_base + 4;
    char *res = (char*) malloc(pa.max_bytes * 2 + 4);

    for (uint32_t i = 0; i < pa.max_bytes * 2 + 4; ++i) {
        req[i] = char(i);
    }

    socket_fd_t socket = tcp_create_socket_to(pa.host.c_str(), pa.port.c_str() );

    for (uint32_t i = 0; i < req_num; ++i) {
        uint32_t offset = rand_r(&seed) % pa.max_bytes;
        uint32_t length = pa.min_bytes + ((range == 0) ? 0 : rand_r(&seed) % range);

        int32_t n_len = htonl(length);
        //ZLOG(LOG_INFO, "#%d. SEND: %x", i, n_len);
        //tcp_write(socket, &n_len, sizeof(n_len) );
        ((int32_t*)(req + offset - 4))[0] = n_len;
        tcp_write(socket, req + offset - 4, length + 4);

        //ZLOG(LOG_INFO, "#%d. RECV", i);
        int32_t r_len = 0;
        tcp_read(socket, &r_len, sizeof(r_len) );
        uint32_t to_read = length;
        while (to_read > 0) {
            int rd = tcp_read(socket, res + length - to_read, to_read);
            if (-1 == rd) {
                ZLOG(LOG_INFO, "#%d. ERROR: %x", i, r_len);
                break;
            }
            to_read -= rd;
        }
        //ZLOG(LOG_INFO, "#%d. DONE: %x", i, r_len);

        /*
        ZLOG(LOG_INFO, "#%d. S[%d] [%d] -->%d, [%lx]",
            i, n_len, length, length + 4, *(uint64_t*)(req + offset) );
        ZLOG(LOG_INFO, "#%d. R[%d] [%d] -->%d, [%lx]", 
            i, r_len, int(ntohl(r_len)), int(ntohl(r_len)) + 4, *(uint64_t*)(res));
        */
        ZLOG(LOG_INFO, "#%d. check error. [%d]", i, memcmp(req + offset, res, length) );
    }

    free(req_base);
    free(res);
    close(socket);
    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        help_msg(argc, argv);
        return -1;
    }

    Params pa;
    if (!parse_params(pa, argv) ) {
        help_msg(argc, argv);
        return -2;
    }

    run(pa);

    return 0;
}


