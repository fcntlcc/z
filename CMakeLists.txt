cmake_minimum_required(VERSION 2.8)
project(z)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/src)

set(Z_FLAG_ENABLE_UT "-DZ_COMPILE_FLAG_ENABLE_UT=1")
add_definitions("-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")

#3rd party libraries
find_package(GTest REQUIRED)

# LIB: zbase - the base level utils
file(GLOB_RECURSE zbase_h    FOLLOW_SYMLINKS src/*.h)
file(GLOB_RECURSE zbase_cpp  FOLLOW_SYMLINKS src/*.cpp)
add_library(zbase    ${zbase_cpp})
#add_library(zbase-ut ${zbase_cpp})
#set_target_properties(zbase-ut   PROPERTIES COMPILE_FLAGS ${Z_FLAG_ENABLE_UT})

# TEST: unit test 
#file(GLOB_RECURSE ut_h      FOLLOW_SYMLINKS ut/*.h)
#file(GLOB_RECURSE ut_cpp    FOLLOW_SYMLINKS ut/*.cpp)
#add_executable(ut ${ut_cpp} )
#target_include_directories(ut PRIVATE foo)
#target_link_libraries(ut
#                     -Wl,--whole-archive
#                     zbase-ut
#                     -Wl,--no-whole-archive
#                     gtest
#                     crypto
#                     pthread
#                     rt)
#enable_testing()
#add_test(ut_all ut)

# DIR: demo
#add_subdirectory(demo)

