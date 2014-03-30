#ifndef Z_ZLOW_ZTIME_H__
#define Z_ZLOW_ZTIME_H__
#include <time.h>
#include <stdint.h>

namespace z {
namespace low {
namespace time {
;

const long ZTIME_STRING_MIN_BUFSIZE = 32;

typedef struct timespec  ztime_t;

const char * now_local(char * buf, uint32_t bufsize);
const char * now_utc(char * buf, uint32_t bufsize);

ztime_t     ztime_now();
void        ztime_now(ztime_t * t);
long        ztime_length_us(const ztime_t & begin, const ztime_t & end);

} // namespace time
} // namespace low
} // namespace z


#endif

