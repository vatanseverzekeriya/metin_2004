cmake_minimum_required(VERSION 3.10)
project(AuthSystemTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MySQL ve OpenSSL kütüphanelerini bul
find_package(OpenSSL REQUIRED)
find_library(MYSQLCLIENT_LIB mysqlclient REQUIRED)

# Include dizinleri
include_directories(
    ${CMAKE_SOURCE_DIR}/../../include
    ${OPENSSL_INCLUDE_DIR}
)

# Kaynak dosyalar
set(SOURCES
    test_auth_system.cpp
    ../../src/db/DBManager.cpp
    ../../src/auth/SessionManager.cpp
    ../../src/auth/AuthManager.cpp
)

# Executable oluştur
add_executable(test_auth ${SOURCES})

# Kütüphaneleri bağla
target_link_libraries(test_auth
    ${MYSQLCLIENT_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# Derleme seçenekleri
target_compile_options(test_auth PRIVATE -Wall -Wextra)

message(STATUS "MySQL Library: ${MYSQLCLIENT_LIB}")
message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
