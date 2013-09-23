#include <gtest/gtest.h>
#include <foo/foo.h>

TEST(ut_foo, return_string_check) {
    ASSERT_STREQ("foo", foo() );
}

