#ifndef Z_HEADER_ENV_UT__H__
#define Z_HEADER_ENV_UT__H__

#if (Z_COMPILE_FLAG_ENABLE_UT == 1)
#   include <gtest/gtest.h>

#   define Z_UNIT_TEST(X) \
            namespace z_ut { \
                X \
            }
#else // Z_COMPILE_FLAG_ENABLE_UT

#   define Z_UNIT_TEST(X) ;

#endif // Z_COMPILE_FLAG_ENABLE_UT

#endif // Z_HEADER_ENV_UT__H__

