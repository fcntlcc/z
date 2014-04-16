# TEST: fork() performance
add_executable(test_fork test_fork.cpp)
target_link_libraries(test_fork pthread)

# TEST: libuv
add_executable(test_libuv test_libuv.cpp)
target_link_libraries(test_libuv uv low)

# TEST: disk io
add_executable(test_disk_io test_disk_io.cpp)
target_link_libraries(test_disk_io low)

# TEST: rapidjson
add_executable(test_rapidjson test_rapidjson.cpp)
target_link_libraries(test_rapidjson low beta)

