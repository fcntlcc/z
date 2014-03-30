#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

const char * now_local(char * buf, uint32_t bufsize)
{
    if (NULL == buf || 0 == bufsize) {
        return buf;
    }

    struct timespec ts;
    struct tm       now;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &now);
    
    snprintf(buf, bufsize, "%.4d%.2d%.2d %.2d:%.2d:%.2d %.6ld", 
        now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
        now.tm_hour, now.tm_min, now.tm_sec, ts.tv_nsec/1000);

    return buf;
}

void * thread_run(void *) {
    char buf[64];
    for (uint32_t i = 0; i < 100000; ++i) {
        fprintf(stderr, "%s\n", now_local(buf, sizeof(buf) ) );
    }

    return NULL;
}

int main(int, char *[])
{
    char buf[64];
    pthread_t pid;
    pthread_create(&pid, NULL, thread_run, NULL);
    fprintf(stderr, "\nXXXXX %s\n", now_local(buf, sizeof(buf) ) );
    system("ls /");
    fprintf(stderr, "\nXXXXX %s\n", now_local(buf, sizeof(buf) ) );
    pthread_join(pid, NULL);
    fprintf(stderr, "\nXXXXX %s\n", now_local(buf, sizeof(buf) ) );
    return 0;
}

