#ifndef Z_RPC_APP__H__
#define Z_RPC_APP__H__

#include "../low/low.h"
#include "./server_base.h"

namespace z {
namespace rpc {
;

class ZApp {
    Z_DECLARE_COPY_FUNCTIONS(ZApp);
public:
    ZApp();
    virtual ~ZApp();
};

class ZDaemon: public ZApp {
public:
    virtual int run() = 0;
    virtual int stop() = 0;
};

class ZEPollTCPDaemon : public ZDaemon {
    Z_DECLARE_COPY_FUNCTIONS(ZEPollTCPDaemon)
public:
    class ThreadTask;

    struct ServerInfo {
        uint32_t    thread_num;

        uint32_t    server_task_pair_num;
        ServerBase  **servers;
        TaskBase    **tasks;
    };

    struct ProcUnit {
        ServerBase  *server;
        TaskBase    *task;
        ThreadTask  *thread_task;
    };
    
    typedef ::z::low::ds::FixedLengthQueueWithFd<uint32_t, z::low::multi_thread::ZSpinLock> ProcUnitIdQueue;    

    class ThreadTask : public z::low::multi_thread::ZThreadTask {
        Z_DECLARE_COPY_FUNCTIONS(ThreadTask)
    public:
        ThreadTask() : _unit_id(0), _result_queue(nullptr) {}
        void reset_info(uint32_t unit_id, const ProcUnit &unit, ProcUnitIdQueue *result_queue) {
            _unit_id = unit_id, _unit = unit; _result_queue = result_queue;
        }
        int exec(void*);
    private:
        uint32_t          _unit_id;
        ProcUnit          _unit;
        ProcUnitIdQueue   *_result_queue;
    };

public:
    ZEPollTCPDaemon(int listen_socket, const ServerInfo &info);
    ~ZEPollTCPDaemon();

    int run();
    int stop();
private:
    void wait_on_listen();
    void stop_listen();
    void wait_on_results();
    void stop_on_results();

    void do_listen_socket_event(const epoll_event &e);
    void do_result_ready_event(const epoll_event &e);
    void do_client_socket_event(const epoll_event &e);

    void start_serve_client(int client_fd);
    void start_recv_request(uint32_t unit_id);
    void do_recv_request(uint32_t unit_id);
    void start_send_result(uint32_t unit_id); 
    void do_send_result(uint32_t unit_id);
    void do_io_error(uint32_t unit_id);
    void start_process_task(uint32_t unit_id);
    ProcUnit id_to_unit(uint32_t unit_id);
private:
    typedef ::z::low::ds::IDPool<z::low::multi_thread::ZNoLock>  ProcUnitIdPool;

    int             _io_epoll;
    int             _listen_socket;
    ServerInfo      _server_data;
    ProcUnitIdPool  _unit_pool;
    ProcUnitIdQueue _results;

    ::z::low::multi_thread::ZThreadPool     _thread_pool;
    ThreadTask                              *_thread_tasks;

    uint32_t        _should_stop:1;
};

// ------------------------------------------------------------------------ //


} // namespace rpc
} // namespace z


#endif

