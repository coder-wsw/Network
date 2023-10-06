cmake_minimum_required(VERSION 3.15)

project(NetWork
    VERSION 1.0.0
    DESCRIPTION "NetWork"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
SET(CMAKE_TOOLCHAIN_FILE "E:/DockerRepo/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_PREFIX_PATH "E:/DockerRepo/vcpkg/installed/x64-windows-static" ${CMAKE_PREFIX_PATH})

find_package(Spdlog CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_subdirectory(examples)