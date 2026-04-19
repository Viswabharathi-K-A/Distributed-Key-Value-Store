cmake_minimum_required(VERSION 3.16)
project(distributed_kv_raft CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# reflecting cmd : protoc --cpp_out=generated --grpc_out=generated   --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin)   raft.proto
# ── Find protobuf and gRPC ────────────────────────────────────────────────────
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)

# ── Locate the grpc_cpp_plugin binary ─────────────────────────────────────────
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# ── Define where proto files live and where to generate output ────────────────
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(GEN_DIR  ${CMAKE_CURRENT_BINARY_DIR}/proto/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

#── The files protoc will generate ───────────────────────────────────────────
set(PROTO_SRCS ${GEN_DIR}/raft.pb.cc ${GEN_DIR}/raft.grpc.pb.cc)
set(PROTO_HDRS ${GEN_DIR}/raft.pb.h ${GEN_DIR}/raft.grpc.pb.h)

# ── Run protoc during build ───────────────────────────────────────────────────
add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --cpp_out=${GEN_DIR}
            --grpc_out=${GEN_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I ${PROTO_DIR}
            raft.proto 
    DEPENDS ${PROTO_DIR}/raft.proto 
    COMMENT "Generating protobuf and gRPC sources"
)

# ── Proto library ─────────────────────────────────────────────────────────────
add_library(raft_proto ${PROTO_SRCS})
target_include_directories(raft_proto PUBLIC ${GEN_DIR} ${Protobuf_INCLUDE_DIRS})
target_link_directories(raft_proto PUBLIC ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES})

# ── KV store library ──────────────────────────────────────────────────────────
add_library(kv_store src/kv_store.cpp)
target_include_directories(kv_store PUBLIC src)

# ── Tests ─────────────────────────────────────────────────────────────────────
add_executable(kv_store_test test/kv_store_test.cpp)
target_link_libraries(kv_store_test PRIVATE kv_store)