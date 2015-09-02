#ifndef Z_ZLOW_HTTPCLIENT_H__
#define Z_ZLOW_HTTPCLIENT_H__

#include <stdint.h>
#include <stdlib.h>

namespace z {
namespace low {
namespace net {
;

enum {
    // max size for a URL
    URL_BUFFER_SIZE     = 4096,
    // max size for a port(in the string form)
    URL_PORT_SIZE       = 16,
    // page buffer size
    PAGE_BUFFER_SIZE    = 1024 * 256,
};

enum HttpGetStatus {
    HTTP_GET_OK     = 0,
    HTTP_GET_FAIL   = 1
};

struct HttpGetRequest {
    char host[URL_BUFFER_SIZE];
    char port[URL_PORT_SIZE];
    char path[URL_BUFFER_SIZE];
    int  timeout_ms;   ///< connect and I/O timeout in milliseconds

    uint64_t page_size;
    char *page;
    char buffer[PAGE_BUFFER_SIZE];
};

void http_get_init(HttpGetRequest *get_req);

// NOTE: only get the first 256KB of the page.
// @retval  HTTP_GET_OK/HTTP_GET_FAIL
int  http_get(HttpGetRequest *get_req);

void http_get_clear(HttpGetRequest *get_req);

} // namespace net
} // namespace low
} // namespace z

#endif

