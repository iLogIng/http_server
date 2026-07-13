cmake_minimum_required(VERSION 3.15)
project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

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
