add_executable(demo_thread_pool thread_pool.cpp)
target_link_libraries(demo_thread_pool low pthread rt)

add_executable(demo_rand_client rand_client.cpp)
target_link_libraries(demo_rand_client low pthread rt)

add_executable(demo_internal_echo_server internal_echo_server.cpp)
target_link_libraries(demo_internal_echo_server rpc low pthread rt)

