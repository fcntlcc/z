#include <low/low.h>
#include <string.h>
#include <string>

using namespace z::low::net;

struct Params {
    std::string     host;
    int             port;
    uint32_t        link_num;
};

void help_msg(int, char *argv[]) {
    fprintf(stderr, "Usage: %s host port link_num\n", argv[0]);
}

bool parse_params(Params &pa, int argc, char *argv[]);
void run(Params &pa);

bool parse_params(Params &pa, int argc, char *argv[]) {
    pa.host = "localhost";
    pa.port = 8888;
    pa.link_num = 16;

    if (argc != 4) {
        return false;
    }

    pa.host = argv[1];
    pa.port = atoi(argv[2]);
    pa.link_num = atoi(argv[3]);

    return true;
}

void run(Params &pa) {
    fprintf(stdout, "creating links to [%s:%d]. [link_num: %u] ...\n", pa.host.c_str(), pa.port, pa.link_num);

    TcpInstance *instance = tcp_connect_to_instance(pa.host.c_str(), pa.port, pa.link_num, pa.link_num);
    fprintf(stdout, "done\n");

    while (instance) {
        usleep(1000 * 1000);
    }
}

int main(int argc, char *argv[]) {
    Params pa;
    if (!parse_params(pa, argc, argv) ) {
        help_msg(argc, argv);
        return -1;
    }

    run(pa);

    return 0;
}

