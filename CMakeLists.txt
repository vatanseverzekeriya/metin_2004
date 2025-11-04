cmake_minimum_required(VERSION 3.10)
project(Metin2_PvP_Server VERSION 1.0 LANGUAGES CXX)

# C++ standardı
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/common
    ${PROJECT_SOURCE_DIR}/include/db
    ${PROJECT_SOURCE_DIR}/include/game
    ${PROJECT_SOURCE_DIR}/include/network
)

# MySQL kütüphanesini bul
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h
    PATHS
    /usr/include
    /usr/local/include
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient mariadbclient
    PATHS
    /usr/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    message(STATUS "MySQL found: ${MYSQL_LIBRARY}")
    include_directories(${MYSQL_INCLUDE_DIR})
else()
    message(WARNING "MySQL library not found. Install with: sudo apt-get install libmysqlclient-dev")
endif()

# Lua kütüphanesini bul
find_package(Lua)
if(LUA_FOUND)
    message(STATUS "Lua found: ${LUA_LIBRARIES}")
    include_directories(${LUA_INCLUDE_DIR})
else()
    message(WARNING "Lua library not found. Install with: sudo apt-get install liblua5.3-dev")
endif()

# Source files
set(COMMON_SOURCES
    # Şimdilik boş - gerekirse common source eklenebilir
)

set(DB_SOURCES
    src/db/DBManager.cpp
)

set(NETWORK_SOURCES
    src/network/WebSocketServer.cpp
    src/network/UDPServer.cpp
    src/network/NetworkManager.cpp
)

set(MOBILE_SOURCES
    src/mobile/TouchInput.cpp
    src/mobile/VirtualJoystick.cpp
    src/mobile/UIButton.cpp
    src/mobile/GameControls.cpp
)

set(GAME_SOURCES
    src/game/Character.cpp
    src/game/GameServer.cpp
    src/game/QuestManager.cpp
    src/game/main.cpp
)

# Game server executable
add_executable(game_server
    ${COMMON_SOURCES}
    ${DB_SOURCES}
    ${NETWORK_SOURCES}
    ${MOBILE_SOURCES}
    ${GAME_SOURCES}
)

# Link libraries
if(MYSQL_LIBRARY)
    target_link_libraries(game_server ${MYSQL_LIBRARY})
endif()

if(LUA_LIBRARIES)
    target_link_libraries(game_server ${LUA_LIBRARIES})
endif()

target_link_libraries(game_server pthread)

# Install targets
install(TARGETS game_server DESTINATION bin)

# Print build info
message(STATUS "=================================")
message(STATUS "Metin2 PvP Server Build Info")
message(STATUS "=================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "MySQL Library: ${MYSQL_LIBRARY}")
message(STATUS "Lua Library: ${LUA_LIBRARIES}")
message(STATUS "=================================")
