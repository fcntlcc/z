#include <low/low.h>
#include <rpc/app.h>
#include <rpc/http_server.h>
#include <string>
#include <stdint.h>
#include <stdlib.h>

using namespace z::low::multi_thread;
using namespace z::low::net;
using namespace z::low::time;
using namespace z::rpc;

struct Params {
    std::string     port;
    uint32_t        thread_num;
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

    return (pa.thread_num > 0);
}

void run(Params &pa) {
    ZEPollTCPDaemon::ServerInfo info;
    info.thread_num = pa.thread_num;
    info.server_task_pair_num = 64;
    info.servers = (ServerBase**) malloc(info.server_task_pair_num * sizeof(ServerBase*) );
    info.tasks = (TaskBase**) malloc(info.server_task_pair_num * sizeof(TaskBase*) );

    HttpServer echo_server;
    InternalServerTask *tasks = new HttpServerTask[info.server_task_pair_num];
    for (uint32_t i = 0; i < info.server_task_pair_num; ++i) {
        info.servers[i] = &echo_server;
        info.tasks[i] = &tasks[i];
        tasks[i].mempool = new z::low::mem::CacheAppendMempool(64 * 1024, 64 * 1024);
    }

    int socket = tcp_listen(pa.port.c_str(), 1024, false/*async*/);

    ZEPollTCPDaemon d(socket, info);

    ZLOG(LOG_INFO, "service is running.");
    d.run();
    ZLOG(LOG_INFO, "service end.");
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


