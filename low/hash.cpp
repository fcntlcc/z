#include "./hash.h"

namespace z {
namespace low {
;


} // namespace low
} // namespace z

#if Z_COMPILE_FLAG_ENABLE_UT 
#include <gtest/gtest.h>
TEST(ut_low_hash, fast_hash) {
    using namespace z::low;

    uint64_t v = 0;
    v = hash64("ABCDEFG", 7);
    EXPECT_STREQ("ABCDEFG", (const char*)&v);

    v = hash64("12345678", 8);
    std::string s((const char*)(&v), 8);
    EXPECT_STREQ("12345678", s.c_str() );

    v = hash64("123456789", 9);
    EXPECT_EQ(4054712127949633829lu, v);
    fprintf(stdout, "v64 = %lx\n", v);

    v = hash32("123456789", 9);
    EXPECT_EQ(2498230565lu, v);
    fprintf(stdout, "v32 = %lx\n", v);
    
    v = hash64("abcdefghijk", 11);
    EXPECT_EQ(11584269842975078802ul, v);
    fprintf(stdout, "v64 = %lx\n", v);

    v = hash32("abcdefghijk", 11);
    EXPECT_EQ(3435968914lu, v);
    fprintf(stdout, "v32 = %lx\n", v);
}

#endif // Z_COMPILE_FLAG_ENABLE_UT

