#ifndef Z_ZBETA_MMAP_FILE_H__
#define Z_ZBETA_MMAP_FILE_H__

#include <string>
#include <low/low.h>

namespace z {
;
namespace beta {
;

using z::low::FDHandle;
using z::low::Z_NULL_FD_HANDLE;

class MMapFile {
public:
    enum FileFlags {
        FLAG_OPEN   = 0x01,
        FLAG_MMAP   = 0X02,
    };

    struct DofFile {
        std::string     path;
        size_t          size;
        void *          buf;
        long            flags;
        FDHandle        fd;
    };

    MMapFile(const std::string & path);
    ~MMapFile();

    bool mmap();

    void * data() const;
    size_t size() const;
private:
    void release();
private:
    DofFile             d;
};


} // namespace beta
} // namespace z



#endif // Z_ZBETA_FILE_H__

