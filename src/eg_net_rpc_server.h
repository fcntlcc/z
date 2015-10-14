#ifndef Z_RPC_SAMPLE_SERVERS_H__
#define Z_RPC_SAMPLE_SERVERS_H__

#include "./server_framework.h"

namespace z {
namespace rpc {
namespace sample {
using namespace z::rpc::server;

namespace http {
;

// A smaple of a light-weight http server
int sample_http_server_op_err(RPCTask *t);
int sample_http_server_op_begin(RPCTask *t);
int sample_http_server_op_read(RPCTask *t);
int sample_http_server_op_sched(RPCTask *t);
int sample_http_server_op_calc(RPCTask *t);
int sample_http_server_op_write(RPCTask *t);
int sample_http_server_op_end(RPCTask *t);
int sample_http_server_op_close(RPCTask *t);


RPCServiceHandle * create_sample_http_service(int listen_fd, int thread_num);
void destroy_sample_http_service(RPCServiceHandle *service);


} // namespace http



} // namespace sample
} // namespace rpc
} // namespace z


#endif

