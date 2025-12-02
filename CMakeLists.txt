cmake_minimum_required(VERSION 3.10)
project(UnifiedPerfTest)

set(CMAKE_CXX_STANDARD 17)

# 生成 compile_commands.json 供 clangd 使用
# 注意：compile_commands.json 会自动生成在 CMAKE_BINARY_DIR (build 目录) 中
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 调试模式：启用调试符号
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置调试编译选项
# 使用完整的调试信息（-g3 包含宏定义）
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0 -fno-omit-frame-pointer")

find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(proto)
include_directories(lib) # For perfetto.h
include_directories(include) # For project headers

# Add libperfetto shared library
add_library(perfetto SHARED lib/perfetto.cc)
target_include_directories(perfetto PUBLIC lib)
find_package(Threads REQUIRED)
target_link_libraries(perfetto Threads::Threads ${CMAKE_DL_LIBS})


# Generate Protobuf sources
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/unified_perf_format.proto)

# Unified Format Generator
add_executable(unified_generator src/unified_generator.cc ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(unified_generator ${Protobuf_LIBRARIES} perfetto) # Still link for generator usage if needed, or remove if dynamic only

# Unified Format Processor
add_executable(unified_processor src/unified_processor.cc ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(unified_processor ${Protobuf_LIBRARIES} perfetto ${CMAKE_DL_LIBS})

# PerfShower Main Executable
add_executable(perf_shower_main 
    src/main.cc
    src/perf_shower.cc
    src/perfetto_wrapper.cc
    src/trace_categories.cc
    ${PROTO_SRCS} 
    ${PROTO_HDRS}
)
target_link_libraries(perf_shower_main ${Protobuf_LIBRARIES} perfetto ${CMAKE_DL_LIBS})
