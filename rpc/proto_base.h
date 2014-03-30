#ifndef Z_ZRPC_PROTO_BASE_H__
#define Z_ZRPC_PROTO_BASE_H__

#include <stdint.h>

namespace zs {
namespace rpc {
;

#pragma pack(push,1)
struct RPCMessageHeader {
    union {
        uint64_t        data;
        struct {
            uint16_t    cmd;
            uint16_t    zeros;
            uint32_t    length;
        };
    };
};
#pragma pack(pop)

struct RPCMessage {
    uint32_t            bytes_done;
    uint32_t            buffer_size;
    void *              buffer;
};

enum RPC_SYS_CMD_ENUM {
    RPC_SYS_CMD_ENUM_BEGIN   = 60000U,
    RPC_SYS_CMD_NO_OP        = RPC_CMD_SYSTEM_START,
    RPC_SYS_CMD_MSG_OK,
    RPC_SYS_CMD_MSG_ERR,
    RPC_SYS_CMD_ECHO,

    RPC_SYS_CMD_ENUM_END,
    RPC_SYS_CMD_COUNT        = RPC_SYS_CMD_ENUM_END - RPC_SYS_CMD_ENUM_BEGIN  
};

enum RPC_RETVAL_ENUM {
    RPC_ERROR                = -1,
    RPC_OK                   = 0,
    RPC_CONTINUE,
};

RPC_RETVAL_ENUM     rpc_read_message(int fd, RPCMessage * msg);
RPC_RETVAL_ENUM     rpc_write_message(int fd, RPCMessage * msg);

} // namespace rpc
} // namespace zs

#endif

