#include <low/low.h>

using namespace z::low::multi_thread;

class MyTask : public ZThreadTask {
public:
    virtual int exec(void*) {
        ZLOG(LOG_INFO, "run task");
        return 0;
    }
};



int main(int, char*[])
{
    MyTask t1, t2;
    ZThreadPool pool;

    pool.start();

    pool.commit(&t1);
    pool.commit(&t2);

    t1.wait();
    t2.wait();

    return 0;
}


