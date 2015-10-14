#ifndef Z_ALGO_HASH_H__
#define Z_ALGO_HASH_H__

#include <stdint.h>
#include <string.h>
#include <openssl/md5.h>

namespace z {
;

template <typename T, typename HashType>
    HashType fast_hash(const T &v) {
        return (HashType)(v);
    }

template <typename T, typename HashType>
    HashType fast_hash(const T *v, uint32_t size) {
        HashType hv = 0;
        if (size <= sizeof(HashType) ) {
            ::memcpy(&hv, (const void*)v, size);
        } else {
            char md5sum[MD5_DIGEST_LENGTH];
            ::MD5((const unsigned char*)v, size, (unsigned char*)md5sum);
            ::memcpy(&hv, md5sum, sizeof(hv) );
        }

        return hv;
    }

template <typename T>
uint64_t hash64(const T &v) { return fast_hash<T, uint64_t>(v); }

template <typename T>
uint64_t hash64(const T *v, uint32_t size) {return fast_hash<T, uint64_t>(v, size); }

template <typename T>
uint32_t hash32(const T &v) { return fast_hash<T, uint32_t>(v); }

template <typename T>
uint32_t hash32(const T *v, uint32_t size) {return fast_hash<T, uint32_t>(v, size); }

} // namespace z


#endif
