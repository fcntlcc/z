#include <low/low.h>
#include <rpc/server_framework.h>
#include <rpc/sample_servers.h>
#include <string>
#include <stdint.h>
#include <stdlib.h>

struct Params {
    std::string     port;
    int             thread_num;
};

void help_msg(int , char*argv[]) {
    fprintf(stderr, "Usage: %s port thread_num\n",
        argv[0]);
}

bool parse_params(Params &pa, char* argv[]);
void run(Params &pa);

bool parse_params(Params &pa, char* argv[]) {
    pa.port             = argv[1];
    pa.thread_num       = atoi(argv[2]);

    if (pa.thread_num < 0) {
        pa.thread_num = 0;
    }

    return true;
}

void run(Params &pa) {
    int socket = z::low::net::tcp_listen(pa.port.c_str(), 1024, false/*async*/);
    z::rpc::server::RPCServiceHandle *s = z::rpc::sample::http::create_sample_http_service(socket, pa.thread_num);

    ZLOG(LOG_INFO, "service begin.");
    z::rpc::server::rpc_run_service(s);
    ZLOG(LOG_INFO, "service end.");

    z::rpc::sample::http::destroy_sample_http_service(s);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        help_msg(argc, argv);
        return -1;
    }

    Params pa;
    if (!parse_params(pa, argv) ) {
        help_msg(argc, argv);
        return -2;
    }

    run(pa);

    ZLOG(LOG_INFO, "exit now.");
    return 0;
}


