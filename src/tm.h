#ifndef Z_TM_H__
#define Z_TM_H__

#include <time.h>
#include <stdint.h>

namespace z {
;

typedef struct timespec  ztime_t;
const char * now_local(char * buf, uint32_t bufsize);
const char * now_utc(char * buf, uint32_t bufsize);

ztime_t     ztime_now();
void        ztime_now(ztime_t * t);
long        ztime_length_us(const ztime_t & begin, const ztime_t & end);

void        zsleep_sec(uint32_t seconds);
void        zsleep_ms(uint32_t  milliseconds);
void        zsleep_us(uint32_t  microseconds);
void        zsleep_ns(uint32_t  nanoseconds);

} // namespace z

#endif
