#include "./znet.h"

namespace z {
namespace low {
namespace net {
;

extern const Socket NullSocket = -1;

namespace async {
;

Socket tcp_connect(const char * node, const char * service)
{
    // TODO
    if (node == service) {
    }
    return NullSocket;
}


} // namespace async


} // namespace net
} // namespace low
} // namespace z



