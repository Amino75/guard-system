cmake_minimum_required(VERSION 3.16)
project(guard-system C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(guard-system

    src/main.c

    src/application/application.c

    src/common/logger.c
    src/common/config.c

    src/utils/utils.c

    src/shared_memory/shared_memory.c
    
    src/web/web_server.c
    third_party/cJSON/cJSON.c
    third_party/mongoose/mongoose.c
    src/camera/camera.c
)

target_include_directories(guard-system PRIVATE
    include
    third_party/cJSON
    third_party/mongoose)

target_compile_options(guard-system PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -DMG_TLS=MG_TLS_OPENSSL
)

find_package(OpenSSL REQUIRED)

target_link_libraries(guard-system
    PRIVATE
    pthread
    rt
    OpenSSL::SSL
    OpenSSL::Crypto
    jpeg
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
