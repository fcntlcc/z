#ifndef Z_LOW_MACRO_UTILS_H__
#define Z_LOW_MACRO_UTILS_H__

#include "./zlog.h"
#include <cassert>

#ifndef Z_DISABLE_MACROS

#define Z_DECLARE_COPY_FUNCTIONS(class_name)        \
    class_name(const class_name&);                  \
    const class_name& operator=(const class_name&);

#define Z_DECLARE_DEFAULT_CONS_DES_FUNCTIONS(class_name)    \
    class_name();                                           \
    ~class_name();                                          \

#define Z_RET_IF(cond, retval)      \
    do {                            \
        if (cond) {                 \
            return retval;          \
        }                           \
    } while(0)

#define Z_LOG_RET_IF(cond, retval, level, msg, ...) \
    do {                                            \
        if (cond) {                                 \
            ZLOG(level, msg, ##__VA_ARGS__);        \
            return retval;                          \
        }                                           \
    } while(0)

#define Z_RET_IF_ANY_ZERO_1(a0, retval) Z_RET_IF(!(a0), retval)
#define Z_RET_IF_ANY_ZERO_2(a0, a1, retval) Z_RET_IF(!((a0) && (a1)), retval)
#define Z_RET_IF_ANY_ZERO_3(a0, a1, a2, retval) Z_RET_IF(!((a0) && (a1) && (a2)), retval)
#define Z_RET_IF_ANY_ZERO_4(a0, a1, a2, a3, retval) Z_RET_IF(!((a0) && (a1) && (a2) && (a3)), retval)
#define Z_RET_IF_ANY_ZERO_5(a0, a1, a2, a3, a4, retval) Z_RET_IF(!((a0) && (a1) && (a2) && (a3) && (a4)), retval)
#define Z_RET_IF_ANY_ZERO_6(a0, a1, a2, a3, a4, a5, retval) Z_RET_IF(!((a0) && (a1) && (a2) && (a3) && (a4) && (a5)), retval)
#define Z_RET_IF_ANY_ZERO_7(a0, a1, a2, a3, a4, a5, a6, retval) Z_RET_IF(!((a0) && (a1) && (a2) && (a3) && (a4) && (a5) && (a6)), retval)
#define Z_RET_IF_ANY_ZERO_8(a0, a1, a2, a3, a4, a5, a6, a7, retval) Z_RET_IF(!((a0) && (a1) && (a2) && (a3) && (a4) && (a5) && (a6) && (a7)), retval)
#define Z_RET_IF_ANY_ZERO_9(a0, a1, a2, a3, a4, a5, a6, a7, a8, retval) Z_RET_IF(!((a0) && (a1) && (a2) && (a3) && (a4) && (a5) && (a6) && (a7) && (a8)), retval)

#define Z_OBJ_DO_1(obj,s0) obj.s0
#define Z_OBJ_DO_2(obj,s0,s1) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_1(obj,s1)
#define Z_OBJ_DO_3(obj,s0,s1,s2) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_2(obj,s1,s2)
#define Z_OBJ_DO_4(obj,s0,s1,s2,s3) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_3(obj,s1,s2,s3)
#define Z_OBJ_DO_5(obj,s0,s1,s2,s3,s4) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_4(obj,s1,s2,s3,s4)
#define Z_OBJ_DO_6(obj,s0,s1,s2,s3,s4,s5) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_5(obj,s1,s2,s3,s4,s5)
#define Z_OBJ_DO_7(obj,s0,s1,s2,s3,s4,s5,s6) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_6(obj,s1,s2,s3,s4,s5,s6)
#define Z_OBJ_DO_8(obj,s0,s1,s2,s3,s4,s5,s6,s7) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_7(obj,s1,s2,s3,s4,s5,s6,s7)
#define Z_OBJ_DO_9(obj,s0,s1,s2,s3,s4,s5,s6,s7,s8) Z_OBJ_DO_1(obj,s0);Z_OBJ_DO_8(obj,s1,s2,s3,s4,s5,s6,s7,s8)

#define Z_PTR_DO_1(ptr,s0) ptr->s0
#define Z_PTR_DO_2(ptr,s0,s1) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_1(ptr,s1)
#define Z_PTR_DO_3(ptr,s0,s1,s2) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_2(ptr,s1,s2)
#define Z_PTR_DO_4(ptr,s0,s1,s2,s3) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_3(ptr,s1,s2,s3)
#define Z_PTR_DO_5(ptr,s0,s1,s2,s3,s4) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_4(ptr,s1,s2,s3,s4)
#define Z_PTR_DO_6(ptr,s0,s1,s2,s3,s4,s5) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_5(ptr,s1,s2,s3,s4,s5)
#define Z_PTR_DO_7(ptr,s0,s1,s2,s3,s4,s5,s6) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_6(ptr,s1,s2,s3,s4,s5,s6)
#define Z_PTR_DO_8(ptr,s0,s1,s2,s3,s4,s5,s6,s7) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_7(ptr,s1,s2,s3,s4,s5,s6,s7)
#define Z_PTR_DO_9(ptr,s0,s1,s2,s3,s4,s5,s6,s7,s8) Z_PTR_DO_1(ptr,s0);Z_PTR_DO_8(ptr,s1,s2,s3,s4,s5,s6,s7,s8)


#define Z_FIND_OBJ_BY_MEMBER(obj_type, member, member_ptr)      \
    ((obj_type*)(((char*)(member_ptr)) - (unsigned long long)(&((obj_type*)(0))->member) ) )

#define ZASSERT(X) assert(X)



#endif // Z_DISABLE_MACROS


#endif

