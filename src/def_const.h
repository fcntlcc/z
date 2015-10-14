#ifndef Z_DEF_CONST_H__
#define Z_DEF_CONST_H__

namespace z {
;

enum DEF_CONST_ENUM {
    NULL_FD    = -1, ///< the NULL value for fd

    DEF_SIZE_PAGE           = 4 * 1024,
    DEF_SIZE_WORD           = 256,
    DEF_SIZE_LINE           = 1024,
    DEF_SIZE_LONG_PAGE      = 64 * 1024,
    DEF_SIZE_LONG_LINE      = 4 * 1024,
    
    // bit in a byte
    BYTE_B0   = 0x01u,
    BYTE_B1   = 0x02u,
    BYTE_B2   = 0x04u,
    BYTE_B3   = 0x08u,
    BYTE_B4   = 0x10u,
    BYTE_B5   = 0x20u,
    BYTE_B6   = 0x40u,
    BYTE_B7   = 0x80u,
    BYTE_HALF = 0x07u,
    BYTE_LOW  = 0x07u,
    BYTE_HIGH = 0x70u,
    BYTE_FULL = 0xFFu,
};



} // namespace z

#endif
