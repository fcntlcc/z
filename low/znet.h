#ifndef Z_ZLOW_ZNET_H__
#define Z_ZLOW_ZNET_H__

#include <stdint.h>

namespace z {
namespace low {
namespace net {
;

typedef int Socket;
extern const Socket NullSocket;

namespace async {

Socket tcp_connect(const char * node, const char * service);

/**
 * @retval  >=0   bytes read
 * @retval  -1    error
 */
int    tcp_read(Socket s, void * buffer, uint32_t bytes);

/**
 * @retval >=0    bytes written
 * @retval -1     error
 */
int    tcp_write(Socket s, void * buffer, uint32_t bytes);


} // namespace async
} // namespace net
} // namespace low
} // namespace z


#endif // Z_ZLOW_ZLOW_H__



