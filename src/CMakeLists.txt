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

set(HEADERS
    ../includes/cache.hpp
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

target_include_directories(http_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/includes
)

target_link_libraries(http_server PRIVATE
    project_warnings
    project_defs
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup
    Boost::program_options
    Boost::json
)
