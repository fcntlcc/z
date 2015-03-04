#include "./app.h"
#include <algorithm>
#include <sys/epoll.h>
#include <unistd.h>

namespace z {
namespace rpc {
;

ZApp::ZApp() {}
ZApp::~ZApp() {}

// ------------------------------------------------------------------------ //

int ZEPollTCPDaemon::ThreadTask::exec(void *) {
    Z_RET_IF_ANY_ZERO_3(_unit.server, _unit.task, _result_queue, -1);

    _unit.server->processTask(_unit.task);
    this->signal_done();

    ZASSERT(_result_queue->enqueue(_unit_id) );

    return 0;
}

ZEPollTCPDaemon::ZEPollTCPDaemon(int listen_socket, const ServerInfo &info) 
: _io_epoll(-1), _listen_socket(listen_socket), _server_data(info), 
  _unit_pool(info.server_task_pair_num), _results(info.server_task_pair_num),
  _thread_pool(info.server_task_pair_num), _thread_tasks(nullptr), _should_stop(0)
{
    _io_epoll = epoll_create1(EPOLL_CLOEXEC);
    ZASSERT(_io_epoll >= 0);
    ZASSERT(_listen_socket >= 0);
    ZASSERT(info.servers);
    ZASSERT(info.tasks);
    _thread_tasks = new ThreadTask[info.server_task_pair_num];
}

ZEPollTCPDaemon::~ZEPollTCPDaemon() {
    stop();
    ::close(_io_epoll);
    delete [] _thread_tasks;
}

int ZEPollTCPDaemon::run() {
    _thread_pool.start(_server_data.thread_num);
    wait_on_listen();
    wait_on_results();

    const uint32_t FLAG_LISTEN = uint32_t(-1);
    const uint32_t FLAG_RESULT = uint32_t(-2);
    const int FD_NUM = 16;
   
    int err_cnt = 0;
    while (!_should_stop) {
        epoll_event e[FD_NUM];
        int n = epoll_wait(_io_epoll, e, FD_NUM, 100);
        if (n < 0) {
            ZLOG(LOG_WARN, "IO Epoll error: ret = %d. errno: %d (%s)",
                n, errno, ZSTRERR(errno).c_str() );
            ++err_cnt;
            if (err_cnt > 1000) {
                ZLOG(LOG_FATAL, "Too many epoll error, wait 3 seconds.");
                usleep(1000 * 1000 * 3);
                err_cnt = 0;
            }
        } else if (n == 0) {
            // No event now.
            continue;
        }
        // n > 0
        for (int i = 0; i < n; ++i) {
            switch (e[i].data.u32) { 
            case FLAG_LISTEN:
                do_listen_socket_event(e[i]);
                break;
            case FLAG_RESULT:
                do_result_ready_event(e[i]);
                break;
            default: // Client Socket
                do_client_socket_event(e[i]);
                break;
            }
        }
        err_cnt = 0;
    }

    return 0;
}

int ZEPollTCPDaemon::stop() {
    stop_listen();
    _thread_pool.stop();
    while (!_results.isEmpty() ) {
        usleep(1000 * 100);
    }

    usleep(1000 * 500);
    _should_stop = 1;
    usleep(1000 * 500);

    return 0;
}

void ZEPollTCPDaemon::wait_on_listen() {
    epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.u32 = uint32_t(-1);

    ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_ADD, _listen_socket, &ev) );
}

void ZEPollTCPDaemon::stop_listen() {
    ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_DEL, _listen_socket, nullptr) );
}

void ZEPollTCPDaemon::wait_on_results() {
    epoll_event ev;
    ev.events   = EPOLLIN;
    ev.data.u32 = uint32_t(-2);

    ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_ADD, _results.dequeue_fd(), &ev) );
}

void ZEPollTCPDaemon::stop_on_results() {
    ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_DEL, _results.dequeue_fd(), nullptr) );
}


void ZEPollTCPDaemon::do_listen_socket_event(const epoll_event &e) {
    using namespace ::z::low::net;
    if (e.events & EPOLLIN) {
        //network_peer_t client;
        int fd = tcp_accept(_listen_socket, true/*async*/, nullptr/*&client*/);
        if (fd >= 0) {
            start_serve_client(fd);
        } else {
            ZLOG(LOG_WARN, "Accept return %d: errno = %d(%s)", fd, errno, ZSTRERR(errno).c_str() );
        }
    } else {
        ZLOG(LOG_FATAL, "Listen socket(%d) error, stop listen. errno = %d (%s)", 
            _listen_socket, errno, ZSTRERR(errno).c_str() );
        stop_listen();
    }
}

void ZEPollTCPDaemon::do_result_ready_event(const epoll_event &e) {
    ZASSERT(e.data.u32 == uint32_t(-2) );
    uint32_t n = _results.count();
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t unit_id;
        if (!_results.dequeue(&unit_id) ) {
            break;
        }

        start_send_result(unit_id);
    }
}

void ZEPollTCPDaemon::do_client_socket_event(const epoll_event &e) {
    uint32_t unit_id = e.data.u32;
    if (e.events & EPOLLIN) {
        do_recv_request(unit_id);
    } else if (e.events & EPOLLOUT) {
        do_send_result(unit_id);
    } else {
        ProcUnit unit = id_to_unit(unit_id);
        ZLOG(LOG_WARN, "Socket on client %d, disconnect it. [event: 0x%lx]",
            unit.task->socket, uint64_t(e.events) );
        do_io_error(unit_id);
    }
}

void ZEPollTCPDaemon::start_serve_client(int client_fd) {
    uint32_t id = _unit_pool.allocate();
    if (id >= _server_data.server_task_pair_num) {
        ZLOG(LOG_WARN, "Too many clients. Only %lu allowed. [id: %u]",
            _server_data.server_task_pair_num, id);
        ::close(client_fd);
        return ;
    }

    ProcUnit unit = id_to_unit(id);
    ZASSERT(unit.server && unit.task);
    
    unit.task->socket = client_fd;
    start_recv_request(id);
}

void ZEPollTCPDaemon::start_recv_request(uint32_t unit_id) {
    ProcUnit unit = id_to_unit(unit_id);
    ZASSERT(unit.server && unit.task);

    unit.server->prepareTask(unit.task);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u32 = unit_id;
    if (-1 == epoll_ctl(_io_epoll, EPOLL_CTL_MOD, unit.task->socket, &ev) ) {
        if (errno == ENOENT) {
            ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_ADD, unit.task->socket, &ev) );
        } else {
            ZASSERT(0);
        }
    }
}

void ZEPollTCPDaemon::do_recv_request(uint32_t unit_id) {
    ProcUnit unit = id_to_unit(unit_id);
    ZASSERT(unit.server && unit.task);

    const uint32_t TCP_READ_BLOCK_SIZE = 1024 * 2;
    char buf[TCP_READ_BLOCK_SIZE];
    int rd = ::z::low::net::tcp_read(unit.task->socket, buf, sizeof(buf) );
    if (-1 == rd) {
        ZLOG(LOG_WARN, "Can not read from client: %d, errno = %d (%s)",
            unit.task->socket, errno, ZSTRERR(errno).c_str() );
        do_io_error(unit_id);
    //} else if (0 == rd) {
        // continue reading
    } else {
        // read some
        ::z::low::mem::RWBuffer *pbuf = unit.task->req_buf;
        pbuf->write(buf, rd);

        switch (unit.server->checkRequestBuffer(unit.task) ) {
        case ServerBase::REQ_CK_DONE:
            start_process_task(unit_id);
            break;
        case ServerBase::REQ_CK_INPROGRESS:
            // continue reading
            if (rd == 0) {
                do_io_error(unit_id);
            }
            break;
        case ServerBase::REQ_CK_ERROR:
        default:
            do_io_error(unit_id);
        }
    }
}

void ZEPollTCPDaemon::start_send_result(uint32_t unit_id) {
    ProcUnit unit = id_to_unit(unit_id);
    ZASSERT(unit.server && unit.task);

    epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.u32 = unit_id;

    ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_ADD, unit.task->socket, &ev) );
}

void ZEPollTCPDaemon::do_send_result(uint32_t unit_id) {
    ProcUnit unit = id_to_unit(unit_id);
    ZASSERT(unit.server && unit.task);

    ::z::low::mem::RWBuffer *pbuf = unit.task->res_buf;
    void *w = nullptr;
    uint32_t bytes = 0;

    pbuf->block_ref(&w, &bytes);
    if (bytes == 0) {
        pbuf->block_read(&w, &bytes);
        pbuf->block_ref(&w, &bytes);
    }

    if (bytes == 0) {
        ZASSERT(pbuf->data_size() == 0);
        // sending result done.
        if (unit.task->flag_keepalive) {
            unit.server->resetTask(unit.task);
            start_recv_request(unit_id);
        } else {
            do_io_error(unit_id);
        }
    } else {
        // do send result
        int r = ::z::low::net::tcp_write(unit.task->socket, w, bytes);
        //ZLOG(LOG_INFO, "TCP_SEND %d <-- %u", r, bytes);
        if (r == -1) {
            ZLOG(LOG_WARN, "Can not write to client. fd = %d, errno = %d (%s)",
                unit.task->socket, errno, ZSTRERR(errno).c_str() );
            do_io_error(unit_id);
        } else if (r == 0) {
            // continue writing
        } else {
            pbuf->skip(r);
        }
    }
}

void ZEPollTCPDaemon::do_io_error(uint32_t unit_id) {
    ProcUnit unit = id_to_unit(unit_id);
    ZASSERT(unit.server && unit.task);

    epoll_ctl(_io_epoll, EPOLL_CTL_DEL, unit.task->socket, nullptr);
    ::close(unit.task->socket);
    unit.server->resetTask(unit.task);

    _unit_pool.release(unit_id);
}

void ZEPollTCPDaemon::start_process_task(uint32_t unit_id) {
    ProcUnit unit = id_to_unit(unit_id);
    ZASSERT(unit.server && unit.task && unit.thread_task);

    ZASSERT(0 == epoll_ctl(_io_epoll, EPOLL_CTL_DEL, unit.task->socket, nullptr) );

    unit.thread_task->reset();
    unit.thread_task->reset_info(unit_id, unit, &_results);

    while (!_thread_pool.commit(unit.thread_task) ) {
        ZLOG(LOG_WARN, "Can not enqueue to the task queue. Too many requests. Delay this request for 1 ms.");
        usleep(1000);
    }
}

ZEPollTCPDaemon::ProcUnit ZEPollTCPDaemon::id_to_unit(uint32_t unit_id) {
    ProcUnit u = {nullptr, nullptr, nullptr};

    if (unit_id >= _server_data.server_task_pair_num) {
        return u;
    }
    u.server = _server_data.servers[unit_id];
    u.task   = _server_data.tasks[unit_id];
    u.thread_task = &_thread_tasks[unit_id];

    return u;
}

} // namespace rpc
} // namespace z


