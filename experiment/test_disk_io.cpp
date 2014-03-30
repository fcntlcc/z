#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <low/low.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(int, char *[])
{
    const long BLOCK_SIZE = 4 * 1024;
    const long FILE_SIZE  = 100 * 1024 * 1024;
    const long BLOCK_NUM  = FILE_SIZE / BLOCK_SIZE;

    fprintf(stdout, "block [%ld] * %ld = file [%ld]\n", BLOCK_SIZE, BLOCK_NUM, FILE_SIZE);

    int fd = open("./test.data", O_RDWR | O_DIRECT | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (-1 == fd) {
        return -1;
    }

    if (0 != ftruncate(fd, FILE_SIZE) ) {
        return -2;
    }

    char block[BLOCK_SIZE];
    for (int32_t i = 0; i < BLOCK_SIZE; ++i) {
        block[i] = 'a' + i % 26;
    }
    int  block_id[BLOCK_NUM];
    srand(time(NULL) );
    for (int32_t i = 0; i < BLOCK_NUM; ++i) {
        block_id[i] = rand() % BLOCK_NUM;
    }

    int32_t fail_cnt = 0;
    ZLOG(LOG_INFO, "START", "A");
    
    for (int32_t i = 0; i < BLOCK_NUM; ++i) {
        if (sizeof(block) != pwrite(fd, block, sizeof(block), block_id[i] * BLOCK_SIZE) ) {
            ZLOG(LOG_INFO, "block offset: %d", block_id[i] * BLOCK_SIZE);
            ++fail_cnt;
        }
    }

    ZLOG(LOG_INFO, "END", "B");
    ZLOG(LOG_INFO, "ERROR: %s", strerror(errno) );
    ZLOG(LOG_INFO, "Write - [fd: %d] [blocksize:%u] [filesize:%d] [total: %d] [fail: %d]", fd, sizeof(block), FILE_SIZE,  BLOCK_NUM, fail_cnt);

    close(fd);

    return 0;
}

