cmake_minimum_required(VERSION 3.16)
project(guard-system C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(guard-system
    src/main.c
    src/common/logger.c
    src/common/config.c
    src/shared_memory/shared_memory.c
    third_party/cJSON/cJSON.c
)

target_include_directories(guard-system PRIVATE
    include
    third_party/cJSON)

target_compile_options(guard-system PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(guard-system PRIVATE pthread)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()