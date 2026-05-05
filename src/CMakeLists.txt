# 添加源文件
set(SOURCES
    main.cpp
    config.cpp
    graceful_shutdown.cpp
    request_handler.cpp
    router.cpp
    server.cpp
    static_file_service.cpp
    utils.cpp
)

# 添加头文件
set(HEADERS
    ../includes/config.hpp
    ../includes/graceful_shutdown.hpp
    ../includes/logger.hpp
    ../includes/request_handler.hpp
    ../includes/router.hpp
    ../includes/server.hpp
    ../includes/static_file_service.hpp
    ../includes/utils.hpp
)

add_executable(http_server ${SOURCES} ${HEADERS})

target_include_directories(http_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(http_server PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(http_server
PRIVATE
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup
    Boost::program_options
    Boost::json
)
