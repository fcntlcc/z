#include "net_http.h"
#include "net_tcp.h"
#include <stdio.h>
#include <string.h>

namespace z {
;

// Interested HTTP response status numbers
enum HTTPResponseStatus {
    HTTP_RS_NULL        = 0,
    HTTP_RS_OK          = 200,
    HTTP_RS_MOVED       = 301,
    HTTP_RS_REDIRECT    = 302,
};

// HTTP headers
enum HTTPResponseHeaderEnum {
    HTTP_RHE_HTTP1_0            = 0,    //< "HTTP/1.0"
    HTTP_RHE_HTTP1_1,                   //< "HTTP/1.1"
    HTTP_RHE_CONTENT_LENGTH,            //< "Content-Length:"
    HTTP_RHE_LOCATION,                  //< "Location:"
    HTTP_RHE_NO_INSTEREST,
};

// return value of HTTP text processing
enum HTTPReadRet {
    HTTP_READ_OK    = 0,
    HTTP_READ_ERROR
};

// result of doing a task
enum SpiderTaskRet {
    SPIDER_CONTINUE     =   0,  //< Can do the next task now.
    SPIDER_SKIP_SAVE,           //< Network error, skip the save step
    SPIDER_NEED_EXIT,           //< need to stop this spider
};

#define HTTP_HEADER_MAX_LEN  (64)

/**
 * @brief HTTP response header
 *
 * Only including the insterested items
 */
typedef struct http_response_head_type {
    long    http_status;    //< status number, @cite HTTPResponseStatus
    long    page_length;    //< value of "Content-Length:"
    char    redirect_url[URL_BUFFER_SIZE+1]; //< redirect URL in "Location:"
} HTTPResponseHead;

static void http_receive_response(int fd, HttpGetRequest *get_req);

void http_get_init(HttpGetRequest *get_req) {
    if (NULL == get_req) {
        return ;
    }

    get_req->page_size = 0;
    http_get_clear(get_req);
}


int http_get(HttpGetRequest *get_req) {
    if (NULL == get_req) {
        return HTTP_GET_FAIL;
    }

    // connect
    socket_fd_t fd = tcp_create_socket_timeout(get_req->host, get_req->port, get_req->timeout_ms);
    if (-1 == fd) {
        return HTTP_GET_FAIL;
    }

    // set I/O timeout
    tcp_socket_set_timeout(fd, get_req->timeout_ms, get_req->timeout_ms);

    // send the get req0uest
    char request[URL_BUFFER_SIZE];
    int bytes = snprintf(request, URL_BUFFER_SIZE, \
        "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: INHttpClient1.0\r\n\r\n", \
        get_req->path, \
        get_req->host);

    // send request
    int w = tcp_write(fd, request, bytes);
    if (w <= 0) {
        return HTTP_GET_FAIL;
    }
    
    // receive response
    http_receive_response(fd, get_req);
    
    if (get_req->page) {
        get_req->page[get_req->page_size] = '\0';
        return HTTP_GET_OK;
    } else {
        return HTTP_GET_FAIL;
    }
}

void http_get_clear(HttpGetRequest *get_req) {
    if (NULL == get_req) {
        return ;
    }

    if (get_req->page_size > PAGE_BUFFER_SIZE) {
        free(get_req->page);
    }

    get_req->host[0] = 0;
    get_req->port[0] = 0;
    get_req->path[0] = 0;

    get_req->page_size = 0;
    get_req->page = NULL;
    get_req->buffer[0] = 0;
}

static int
http_read_line(FILE * http_file, char * buf, size_t buf_size)
{
    size_t line_len = 0;

    // read in one line text
    if (NULL == fgets(buf, buf_size, http_file) ) {
        return HTTP_READ_ERROR;
    }

    line_len = strlen(buf);
    
    // for tailing "blanks" 
    buf[line_len] = 0;
    while (    buf[line_len] == '\0'
            || buf[line_len] == '\r'
            || buf[line_len] == '\n') {
        buf[line_len] = '\0';
        --line_len;
        if (0 == line_len+1) break;
    }

    ++line_len;

    return HTTP_READ_OK;
}

static void
http_read_int(const char * buf, long * i_n_t)
{
    sscanf(buf, "%ld", i_n_t);
}

/**
 * @brief check the text in *line*, return which HTTP header it is.
 * @param line  the input line stream
 * @param end   return the start point of the value of this header
 * @return @cite HTTPResponseHeaderEnum
 */
static int http_check_response_header(const char * line, const char **end) {
    char header[HTTP_HEADER_MAX_LEN] = {0};
    size_t header_len = 0;

    // read the header string
    for (header_len = 0; 
         header_len < HTTP_HEADER_MAX_LEN && line[header_len];
         ++header_len) {
        if (' ' == line[header_len]) {
            break;
        }
        header[header_len] = line[header_len];
        if (header[header_len] >= 'a' && header[header_len] <= 'z') {
            header[header_len] += ('A' - 'a');
        }
    }
    header[header_len] = '\0';
    if (line[header_len]) {
        *end = &line[header_len+1];
    } else {
        *end = &line[header_len];
    }

    // check header string
    if (!strcmp("HTTP/1.0", header)) {
        return HTTP_RHE_HTTP1_0;
    } else
    if (!strcmp("HTTP/1.1", header)) {
        return HTTP_RHE_HTTP1_1;
    } else
    if (!strcmp("CONTENT-LENGTH:", header)) {
        return HTTP_RHE_CONTENT_LENGTH;
    } else
    if (!strcmp("LOCATION:", header)) {
        return HTTP_RHE_LOCATION;
    } else {
        ;
    }

    return HTTP_RHE_NO_INSTEREST;
}


/**
 * @brief read and save the insterested item from the HTTP response stream
 * @param http_file  the input HTTP response stream
 * @return @cite HTTPReadRet
 */
static int http_read_response_head(FILE * http_file, HTTPResponseHead * head) {
    char buf[URL_BUFFER_SIZE + 1];
    const char *p = NULL;
    int ret = HTTP_READ_OK;
   
    head->http_status = HTTP_RS_NULL;
    head->page_length = -1;
    head->redirect_url[0] = '\0';

    // read each line untile the whole header is consumed 
    while ( HTTP_READ_OK == (ret = http_read_line(http_file, 
                                                 buf, 
                                                 sizeof(buf) ) )
           && buf[0] != '\0') {
        switch (http_check_response_header(buf, &p)) {
        case HTTP_RHE_HTTP1_0:
        case HTTP_RHE_HTTP1_1:
            http_read_int(p, &head->http_status);
            break;
        case HTTP_RHE_CONTENT_LENGTH:
            http_read_int(p, &head->page_length);
            break;
        case HTTP_RHE_LOCATION:
            strncpy(head->redirect_url, p, URL_BUFFER_SIZE);
            break;
        case HTTP_RHE_NO_INSTEREST:
            break;
        default:
            break;
        }
    }

    return ret;
}

static int
http_save_page_seg(FILE * http_file, HttpGetRequest *get_req, size_t bytes)
{
    char buf[URL_BUFFER_SIZE+1];
    size_t io_bytes = 0;
    size_t one_read_bytes = 0;

    while (io_bytes < bytes) {
        one_read_bytes = (URL_BUFFER_SIZE < (bytes - io_bytes)) 
                        ? (size_t)URL_BUFFER_SIZE : (bytes - io_bytes);

        // read
        one_read_bytes = fread(buf, 1, one_read_bytes, http_file);
        if (0 == one_read_bytes) {
            return SPIDER_CONTINUE;
        }
        // save
        /*
        one_write_bytes = fwrite(buf, 1, one_read_bytes, save_file);
        if (one_write_bytes < one_read_bytes) {
            return SPIDER_NEED_EXIT;
        }
        io_bytes += one_write_bytes;
        sinfo->total_size += one_write_bytes;
        */
        if (one_read_bytes + get_req->page_size <= sizeof(get_req->buffer) ) {
            memcpy(get_req->buffer + get_req->page_size, buf, one_read_bytes);
            get_req->page_size += one_read_bytes;
            get_req->page = get_req->buffer;
        } else {
            memcpy(get_req->buffer + get_req->page_size, buf, sizeof(get_req->buffer) - get_req->page_size);
            get_req->page_size = sizeof(get_req->buffer);
            get_req->page = get_req->buffer;
        }
        io_bytes += one_read_bytes; 
    }

    return SPIDER_CONTINUE;
}

static void http_get_page(FILE *http_file, HttpGetRequest *get_req, size_t bytes) {
    long seg_size = 0;

    if (size_t(-1) == bytes) {
        // unkonwn page size, guess it is sent by segments
        for (;;) {
            char buf[URL_BUFFER_SIZE];
            seg_size = 0;
            if (0 >= fscanf(http_file, "%lx", &seg_size)) {
                // not by segments
                http_save_page_seg(http_file, get_req, -1);
                break;
            }
            if (0 == seg_size) {
                break;
            }
            fgets(buf, sizeof(buf), http_file);
            int ret = http_save_page_seg(http_file, get_req, seg_size);
            if (SPIDER_NEED_EXIT == ret) {
                break;
            }
        }
        return; 
    } else {
        // have a valid page size
        http_save_page_seg(http_file, get_req, bytes);
    }
}

static void http_receive_response(int fd, HttpGetRequest *get_req) {
    FILE * socket_file = NULL;
    HTTPResponseHead rhead = {0, 0, ""};

    if (-1 == fd || NULL == get_req) {
        return ;
    }

    get_req->page = NULL;

    // construct a FILE* from the socket
    socket_file = fdopen(fd, "rw");
    if (NULL == socket_file) {
        goto EXIT;
    }

    // read HTTP response head
    if (HTTP_READ_ERROR == http_read_response_head(socket_file, &rhead)) {
        goto EXIT;
    }

    // check the response head and ...
    switch (rhead.http_status) {
    case HTTP_RS_OK:
        // download page
        http_get_page(socket_file, get_req, rhead.page_length);
        break;
    case HTTP_RS_MOVED:
    case HTTP_RS_REDIRECT:
        // TODO: do_redirect
        // ret = do_redirect(sinfo, task, rhead.redirect_url);
        // break;
    default:
        goto EXIT;
    }
EXIT:
    if (socket_file) {
        fclose(socket_file);
    }
    return ;
}

} // namespace z
