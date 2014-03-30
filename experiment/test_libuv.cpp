#include <stdio.h>
#include <uv.h>
#include <low/low.h>

#define ZRUN_TEST(func) \
    do {                                        \
        ZLOG(LOG_DEBUG, "RUN: [%s]", #func);   \
        func();                                 \
        ZLOG(LOG_DEBUG, "END: [%s]", #func);   \
    } while (0)
        

static int hello();
static int idle_event();
static void idle_callback(uv_idle_t *handle, int status);
static int time_event();
static void time_callback(uv_timer_t * handle, int status);

int main(int argc, char *argv[])
{
    ZLOG(LOG_DEBUG, "argc = %d, argv[0] = %s", argc, argv[0]);

    ZRUN_TEST(hello);
    ZRUN_TEST(idle_event);
    ZRUN_TEST(time_event);

    return 0;
}

static int hello()
{
    uv_loop_t * loop = uv_loop_new();
    uv_run(loop, UV_RUN_DEFAULT);
    
    uv_loop_delete(loop);

    return 0;
}

static int idle_event()
{
    uv_idle_t idle_handle;
    uv_idle_init(uv_default_loop(), &idle_handle);
    uv_idle_start(&idle_handle, idle_callback);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return 0;
}

static void idle_callback(uv_idle_t * handle, int status)
{
    static int cnt = 0;
    ++cnt;
    ZLOG(LOG_DEBUG, "IDLE [%p][%d] status[%d]", handle, cnt, status);
    if (cnt >= 10) {
        uv_idle_stop(handle);
    }
}

static int time_event()
{
    uv_timer_t time_handle;
    uv_timer_init(uv_default_loop(), &time_handle);
    uv_timer_start(&time_handle, time_callback, 10, 20);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return 0;
}

static void time_callback(uv_timer_t * handle, int status)
{
    static int cnt = 0;
    ++cnt;
    ZLOG(LOG_DEBUG, "IDLE [%p][%d] status[%d]", handle, cnt, status);
    if (cnt >= 10) {
        uv_timer_stop(handle);
    }
}




