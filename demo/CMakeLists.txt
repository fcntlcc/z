add_executable(demo_thread_pool thread_pool.cpp)
target_link_libraries(demo_thread_pool low pthread rt)

add_executable(demo_rand_client rand_client.cpp)
target_link_libraries(demo_rand_client low pthread rt)

add_executable(demo_internal_echo_server internal_echo_server.cpp)
target_link_libraries(demo_internal_echo_server rpc low pthread rt)

add_executable(demo_http_server http_server.cpp)
target_link_libraries(demo_http_server rpc low pthread rt)

add_executable(demo_sample_http_service sample_http_service.cpp)
target_link_libraries(demo_sample_http_service rpc low pthread rt)

add_executable(demo_sample_multi_link_client sample_multi_link_client.cpp)
target_link_libraries(demo_sample_multi_link_client rpc low pthread rt)
