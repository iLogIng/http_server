cmake_minimum_required(VERSION 3.15)
project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

# 限制编译/链接并行度：逻辑核心数的一半（至少 1），自动适应平台。
# 可用 -DHTTP_SERVER_BUILD_JOBS=N 覆盖；编译池约束任何更大的 -j。
cmake_host_system_information(RESULT _http_default_jobs
    QUERY NUMBER_OF_LOGICAL_CORES)
math(EXPR _http_default_jobs "${_http_default_jobs} / 2")
if(_http_default_jobs LESS 1)
    set(_http_default_jobs 1)
endif()
set(HTTP_SERVER_BUILD_JOBS ${_http_default_jobs} CACHE STRING
    "Parallel compile jobs (default: half of logical cores)")
set_property(GLOBAL PROPERTY JOB_POOLS
    compile_jobs=${HTTP_SERVER_BUILD_JOBS}
    link_jobs=1
)
set(CMAKE_JOB_POOL_COMPILE compile_jobs)
set(CMAKE_JOB_POOL_LINK link_jobs)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认 Release 构建
#   仅对单配置生成器 Makefile Ninja 有效
#   MSVC 等多配置生成器通过 --config 在构建时指定
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BUILD_TESTING "Build unit tests" ON)

find_package(Boost CONFIG REQUIRED COMPONENTS
    system
    thread
    filesystem
    log
    program_options
    log_setup
    json
)

find_package(ZLIB REQUIRED)
find_package(ZLIB REQUIRED)

add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_library(project_defs INTERFACE)
target_compile_definitions(project_defs INTERFACE
    BOOST_LOG_DYN_LINK
    $<$<CXX_COMPILER_ID:MSVC>:_WIN32_WINNT=0x0A00>
)

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
