cmake_minimum_required(VERSION 3.10)
project(online_store)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

find_package(PostgreSQL REQUIRED)

find_package(OpenSSL REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_executable(online_store ${SOURCES} ${HEADERS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

target_include_directories(online_store PRIVATE 
    ${PostgreSQL_INCLUDE_DIRS}
    ${LIBPQXX_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    include
)

target_link_libraries(online_store PRIVATE 
    ${LIBPQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    OpenSSL::Crypto
    pthread
)

target_compile_options(online_store PRIVATE ${LIBPQXX_CFLAGS_OTHER})
