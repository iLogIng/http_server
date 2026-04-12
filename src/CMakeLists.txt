# 添加源文件
set(SOURCES main.cpp)

# 添加头文件
set(HEADERS
    config.hpp
    logger.hpp
    request_handler.hpp
    router.hpp
    server.hpp
    utils.hpp
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
    Boost::json
)
