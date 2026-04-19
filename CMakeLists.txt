cmake_minimum_required(VERSION 3.16)
project(distributed_kv_raft CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(kv_store
    src/kv_store.cpp
)
target_include_directories(kv_store PUBLIC src)

add_executable(kv_store_test
    test/kv_store_test.cpp
)
target_link_libraries(kv_store_test PRIVATE kv_store)