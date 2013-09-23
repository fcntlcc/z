cmake_minimum_required(VERSION 2.8)
project(zsframe)

include_directories(${CMAKE_SOURCE_DIR})
set(Z_FLAG_ENABLE_UT "-DZ_COMPILE_FLAG_ENABLE_UT=1")
add_definitions("-Wall -Wextra -Werror")

#3rd party libraries
find_package(GTest REQUIRED)

# LIB:  foo
file(GLOB_RECURSE foo_h     FOLLOW_SYMLINKS foo/*.h)
file(GLOB_RECURSE foo_cpp   FOLLOW_SYMLINKS foo/*.cpp)
add_library(foo ${foo_cpp} )
add_library(foo-ut ${foo_cpp} )
set_target_properties(foo-ut PROPERTIES COMPILE_FLAGS ${Z_FLAG_ENABLE_UT})

# TEST: unit test 
file(GLOB_RECURSE ut_h      FOLLOW_SYMLINKS ut/*.h)
file(GLOB_RECURSE ut_cpp    FOLLOW_SYMLINKS ut/*.cpp)
add_executable(ut ${ut_cpp} )
target_include_directories(ut PRIVATE foo)
target_link_libraries(ut -Wl,--whole-archive foo-ut -Wl,--no-whole-archive gtest)

enable_testing()
add_test(ut_all ut)

