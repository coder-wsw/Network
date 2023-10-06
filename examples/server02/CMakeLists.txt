project(server02)

set(SRC
    main.cpp
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    ZLIB::ZLIB
)