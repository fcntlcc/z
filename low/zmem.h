#ifndef Z_LOW_ZMEM_H__
#define Z_LOW_ZMEM_H__

#include <stdlib.h>
#include <stdint.h>

namespace z {
namespace low {
namespace mem {
;

class Mempool {
public:
    virtual ~Mempool();

    virtual void* malloc(size_t size);
    virtual void  free(void *ptr);

    virtual void reset();
};

class CacheAppendMempool : public Mempool {
    struct MemBlock {
        uint32_t    size;
        uint32_t    used;
        MemBlock    *next;
        char        data[];
    };
public:
    CacheAppendMempool(uint32_t head_size = 4 * 1024, uint32_t append_size = 4 * 1024);
    CacheAppendMempool(void *head, uint32_t head_size, uint32_t append_size = 4 * 1024);
    ~CacheAppendMempool();

    void* malloc(size_t size);
    void free(void *ptr);
    void reset();
private:
    void alloc_head(uint32_t head_size);
    bool make_space(size_t size);
protected:
    uint32_t        _append_size;
    MemBlock        *_head;
    MemBlock        *_current;
    uint32_t        _own_head:1;
};


class RWBuffer {
    struct BufferBlock {
        BufferBlock     *next;
        char            data[];
    };

    struct BufferOffset {
        BufferBlock     *block;
        uint32_t        offset;
    };
public:
    RWBuffer(Mempool *mempool = nullptr, uint32_t block_size = 1024 * 4u);
    ~RWBuffer();

    uint32_t data_size() const;

    int32_t skip(uint32_t bytes);

    int32_t write(void *buf, uint32_t bytes);
    int32_t read(void *buf, uint32_t bytes, bool inc_pos = true);

    void block_read(void **buf, uint32_t *bytes);
    void block_ref(void **buf, uint32_t *bytes);
private:
    BufferBlock* make_new_block();
    void release_block(BufferBlock *block);
private:
    Mempool         *_mempool;
    uint32_t        _block_size;
    uint32_t        _block_data_size;
    uint32_t        _data_size;
    BufferOffset    _r_pos;
    BufferOffset    _w_pos;
    uint32_t        _own_mempool:1;
};


}
}
} 

#endif

