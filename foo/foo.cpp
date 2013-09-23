#include "./foo.h"
#include <env/ut.h>

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

Z_UNIT_TEST (
;

// test foo_impl()
TEST(ut_foo_impl, return_string_check) {
    ASSERT_STREQ("foo", foo_impl() );
}

); // Z_UNIT_TEST

