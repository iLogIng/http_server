cmake_minimum_required(VERSION 3.15)
project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找 Boost 库依赖
find_package(Boost CONFIG COMPONENTS 
    system
    thread
    filesystem
    log
    program_options
    log_setup
    json
)

add_subdirectory(src)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()