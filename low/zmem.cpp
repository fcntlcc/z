#include "./zmem.h"
#include "./macro_utils.h"
#include <algorithm>
#include <string.h>

namespace z {
namespace low {
namespace mem {
;

Mempool::~Mempool() {}

void* Mempool::malloc(size_t size) {
    return ::malloc(size);
}

void Mempool::free(void *ptr) {
    ::free(ptr);
}

void Mempool::reset() {}

CacheAppendMempool::CacheAppendMempool(uint32_t head_size, uint32_t append_size)
: _append_size(append_size), _own_head(0) {
    alloc_head(head_size);
}

CacheAppendMempool::CacheAppendMempool(void *head, uint32_t head_size, uint32_t append_size)
: _append_size(append_size), _own_head(0) {
    if (head == nullptr || head_size < sizeof(MemBlock) ) {
        alloc_head(4 * 1024u);
    } else {
        _head = _current = (MemBlock *)(head);
        _head->size = head_size - sizeof(MemBlock);
        _head->used = 0;

        _own_head = 0;
    }
}

CacheAppendMempool::~CacheAppendMempool() {
    reset();
    if (_own_head) {
        ::free(_head);
    }
}

void* CacheAppendMempool::malloc(size_t size) {
    // align to 8 bytes
    uint32_t alloc_size = (size & ~0x07u) + (((size & 0x07u) && 1) << 3u);
    if (!make_space(alloc_size) ) {
        return nullptr;
    }

    void* ret = &_current->data[_current->used];
    _current->used += alloc_size;
    return ret;
}

void CacheAppendMempool::free(void *) {}

void CacheAppendMempool::reset() {
    MemBlock *block = (_head) ? (_head->next) : nullptr;
    while (block) {
        MemBlock *to_free = block;
        block = block->next;
        ::free(to_free);
    }

    if (_head) {
        _head->used = 0;
        _head->next = nullptr;
    }

    _current = _head;
}

void CacheAppendMempool::alloc_head(uint32_t head_size) {
    uint32_t alloc_size = std::max<uint32_t>(head_size, 2*sizeof(MemBlock) );
    _head = _current = (MemBlock*) ::malloc(alloc_size);
    if (_head) {
        _head->size = alloc_size - sizeof(MemBlock);
        _head->used = 0;
        _head->next = nullptr;
    }
    _own_head = 1;
}

bool CacheAppendMempool::make_space(size_t size) {
    if (!_current) {
        return false;
    }

    if (_current->used + size > _current->size) {
        // need a new block
        uint32_t block_size = (size + sizeof(MemBlock) > _append_size) ? 
            (size + sizeof(MemBlock) ) : _append_size;
        MemBlock *next = (MemBlock*) ::malloc(block_size);
        if (next == nullptr) {
            return false;
        }

        next->size = block_size - sizeof(MemBlock);
        next->used = 0;
        next->next = nullptr;

        _current->next = next;
        _current = next;
    }

    return true;
}

RWBuffer::RWBuffer(Mempool *mempool, uint32_t block_size)
: _mempool(mempool), _block_size(block_size), 
  _block_data_size(block_size - sizeof(BufferBlock)), 
  _data_size(0), _own_mempool(0) {
    if (_mempool == nullptr) {
        _mempool = new CacheAppendMempool;
        _own_mempool = 1;
    }

    _r_pos.block = _w_pos.block = make_new_block();
    _r_pos.offset = _w_pos.offset = 0;
}

RWBuffer::~RWBuffer() {
    if (_own_mempool) {
        delete _mempool;
    }

    _own_mempool = 0;
    _mempool = nullptr;
}

uint32_t RWBuffer::data_size() const {
    return _data_size;
}

int32_t RWBuffer::skip(uint32_t bytes) {
    int32_t read_cnt = 0;

    while (bytes > 0 && _data_size) {
        if (_r_pos.offset >= _block_data_size) {
            if (_r_pos.block->next == nullptr) {
                break;
            }
            BufferBlock *old_block = _r_pos.block;
            _r_pos.block = _r_pos.block->next;
            _r_pos.offset = 0;
            release_block(old_block);
        }

        uint32_t block_bytes = (_r_pos.block == _w_pos.block) ?
            (_w_pos.offset - _r_pos.offset) : (_block_data_size - _r_pos.offset);
        uint32_t r_len = std::min(bytes, block_bytes);

        ZASSERT(r_len <= _data_size);

        read_cnt += r_len;
        _r_pos.offset += r_len;
        bytes -= r_len;
        _data_size -= r_len;
    }

    return read_cnt;
} 
int32_t RWBuffer::write(void *buf, uint32_t bytes) {
    int32_t write_cnt = 0;
    char *   pwrite = (char*)(buf);
    while (buf && (bytes > 0) ) {
        if (_w_pos.offset >= _block_data_size) {
            _w_pos.block->next = make_new_block();
            _w_pos.block = _w_pos.block->next;
            _w_pos.offset = 0;
            if (_w_pos.block == nullptr) {
                break;
            }
        }

        uint32_t w_len = std::min(bytes, _block_data_size - _w_pos.offset);
        ::memcpy(_w_pos.block->data + _w_pos.offset, pwrite, w_len);
        write_cnt += w_len;
        pwrite += w_len;
        _w_pos.offset += w_len;
        bytes -= w_len;
        _data_size += w_len;
    }

    return write_cnt;
}

int32_t RWBuffer::read(void *buf, uint32_t bytes, bool inc_pos) {
    int32_t read_cnt = 0;
    char * pread = (char*)(buf);

    BufferOffset start_pos = _r_pos;
    uint32_t start_size = _data_size;
    while (buf && (bytes > 0) && _data_size > 0) {
        if (_r_pos.offset >= _block_data_size) {
            if (_r_pos.block->next == nullptr) {
                break;
            }
            BufferBlock *old_block = _r_pos.block;
            _r_pos.block = _r_pos.block->next;
            _r_pos.offset = 0;
            if (inc_pos) {
                release_block(old_block);
            }
        }
        uint32_t block_bytes = (_r_pos.block == _w_pos.block) ? 
                    (_w_pos.offset - _r_pos.offset) : (_block_data_size - _r_pos.offset);

        uint32_t r_len = std::min(bytes, block_bytes);
        ::memcpy(pread, _r_pos.block->data + _r_pos.offset, r_len);
        read_cnt += r_len;
        pread += r_len;
        _r_pos.offset += r_len;
        bytes -= r_len;
        _data_size -= r_len;
    }

    if (!inc_pos) {
        _r_pos = start_pos;
        _data_size = start_size;
    }

    return read_cnt;
}

void RWBuffer::block_read(void **buf, uint32_t *bytes) {
    Z_RET_IF_ANY_ZERO_2(buf, bytes,);

    *buf   = _r_pos.block->data + _r_pos.offset;
    *bytes = (_r_pos.block == _w_pos.block) ? 
                (_w_pos.offset - _r_pos.offset) : ( _block_data_size - _r_pos.offset);

    ZASSERT(*bytes <= _data_size);

    _r_pos.offset += *bytes;
    _data_size -= *bytes;
    
    if (_r_pos.block->next) {
        BufferBlock *old_block = _r_pos.block;
        _r_pos.block = _r_pos.block->next;
        _r_pos.offset = 0;

        release_block(old_block);
    }
}

void RWBuffer::block_ref(void **buf, uint32_t *bytes) {
    if (!(buf && bytes)) {
        return ;
    }

    *buf   = _r_pos.block->data + _r_pos.offset;
    *bytes = (_r_pos.block == _w_pos.block) ? 
                (_w_pos.offset - _r_pos.offset) : ( _block_data_size - _r_pos.offset);
}

RWBuffer::BufferBlock *RWBuffer::make_new_block() {
    if (_mempool) {
        BufferBlock *b = (BufferBlock*) _mempool->malloc(_block_size);
        if (b) {
            b->next = nullptr;
        }

        return b;
    }

    return nullptr;
}

void RWBuffer::release_block(BufferBlock *block) {
    if (block && _mempool) {
        _mempool->free(block);
    }
}


} // namespace mem
} // namespace low
} // namespace z


#if Z_COMPILE_FLAG_ENABLE_UT 
#include <gtest/gtest.h>
TEST(ut_low_mem, rwbuffer_read_write) {
    using namespace z::low::mem;

    RWBuffer io;
    EXPECT_EQ(0, io.data_size() );

    const uint32_t DSIZE = 100 * 1024;
    char req[DSIZE];
    char res[DSIZE];
    for (uint32_t i = 0; i < DSIZE; ++i) {
        req[i] = char(i);
    }

    ASSERT_EQ(DSIZE, io.write(req, DSIZE) );
    ASSERT_EQ(DSIZE, io.data_size());
    ASSERT_EQ(DSIZE, io.read(res, DSIZE) );
    ASSERT_EQ(0, io.data_size());
    ASSERT_EQ(0, memcmp(req, res, DSIZE) );

    ASSERT_EQ(DSIZE, io.write(req, DSIZE) );
    ASSERT_EQ(DSIZE, io.data_size());
    ASSERT_EQ(DSIZE/4, io.read(res, DSIZE/4) );
    ASSERT_EQ(DSIZE-DSIZE/4, io.data_size());
}

TEST(ut_low_mem, rwbuffer_block_read) {
    using namespace z::low::mem;

    RWBuffer io;
    EXPECT_EQ(0, io.data_size() );

    const uint32_t DSIZE = 100 * 1024;
    char req[DSIZE];
    char res[DSIZE] = {0};
    for (uint32_t i = 0; i < DSIZE; ++i) {
        req[i] = char(i);
    }

    ASSERT_EQ(DSIZE, io.write(req, DSIZE) );
    ASSERT_EQ(DSIZE, io.data_size());
    void*r = nullptr;
    uint32_t bytes = 0;
    uint32_t total_bytes = 0;
    do {
        io.block_read(&r, &bytes);
        memcpy(res + total_bytes, r, bytes);
        total_bytes += bytes;
    } while (bytes > 0);
    ASSERT_EQ(DSIZE, total_bytes);
    ASSERT_EQ(0, io.data_size() );
    ASSERT_EQ(0, memcmp(req, res, DSIZE) );
}

TEST(ut_low_mem, rwbuffer_block_ref_and_skip) {
    using namespace z::low::mem;

    RWBuffer io;
    EXPECT_EQ(0, io.data_size() );

    const uint32_t DSIZE = 100 * 1024;
    char req[DSIZE];
    char res[DSIZE] = {0};
    for (uint32_t i = 0; i < DSIZE; ++i) {
        req[i] = char(i);
    }

    ASSERT_EQ(DSIZE, io.write(req, DSIZE) );
    ASSERT_EQ(DSIZE, io.data_size());
    void*r = nullptr;
    uint32_t bytes = 0;
    uint32_t total_bytes = 0;
    do {
        io.block_ref(&r, &bytes);
        if (0 == bytes) {
            io.block_read(&r, &bytes);
            io.block_ref(&r, &bytes);
        }
        bytes = (bytes > 300) ? 300 : bytes;
        memcpy(res + total_bytes, r, bytes);
        total_bytes += bytes;
        io.skip(bytes);
    } while (bytes > 0);
    ASSERT_EQ(DSIZE, total_bytes);
    ASSERT_EQ(0, io.data_size() );
    ASSERT_EQ(0, memcmp(req, res, DSIZE) );
}
#endif


