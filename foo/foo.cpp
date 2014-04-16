#include "./foo.h"

static const char * foo_impl();

// the public foo()
const char * foo()
{
    return foo_impl();
}

// the inner foo() implementation
const char * foo_impl()
{
    return "foo";
}


#if Z_COMPILE_FLAG_ENABLE_UT
#include <gtest/gtest.h>
// test foo_impl()
TEST(ut_foo_impl, return_string_check) {
    ASSERT_STREQ("foo", foo_impl() );
}

#endif // Z_COMPILE_FLAG_ENABLE_UT

