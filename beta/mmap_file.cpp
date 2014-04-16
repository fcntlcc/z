#include "./mmap_file.h"
#include <low/low.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

namespace z {
;
namespace beta {
;

MMapFile::MMapFile(const std::string & path) {
    d.path = path;
    d.size = 0u;
    d.buf  = nullptr;
    d.flags= 0;
    d.fd   = Z_NULL_FD_HANDLE;
}

MMapFile::~MMapFile() {
    release();
}

bool MMapFile::mmap() {
    release();

    FDHandle fd = ::open(d.path.c_str(), O_RDWR | O_CREAT);
    if (Z_NULL_FD_HANDLE == fd) {
        ZLOG(LOG_WARN, "open [%s] failed.", d.path.c_str() );
        return false;
    }

    struct stat file_stat;
    if (-1 == ::fstat(fd, &file_stat) ) {
        ZLOG(LOG_WARN, "fstat [%s] failed.", d.path.c_str() );
        close(fd);
        return false;
    }

    void * addr = ::mmap(NULL, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (NULL == addr) {
        ZLOG(LOG_WARN, "mmap [%s] failed.", d.path.c_str() );
        close(fd);
        return false;
    }

    d.size  = file_stat.st_size;
    d.buf   = addr;
    d.fd    = fd;
    d.flags |= (FLAG_OPEN | FLAG_MMAP);

    return true;
}

void * MMapFile::data() const {
    return d.buf;
}

size_t MMapFile::size() const {
    return d.size;
}

void MMapFile::release() {
    if ((FLAG_MMAP & d.flags) && d.buf) {
        munmap(d.buf, d.size);
        d.buf = nullptr;
    }

    if (FLAG_OPEN & d.flags) {
        close(d.fd);
    }

    d.flags = 0;
}

} // namespace beta
} // namespace z

