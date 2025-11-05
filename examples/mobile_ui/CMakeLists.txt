# Metin2 Mobile UI Examples - CMake Build Script

cmake_minimum_required(VERSION 3.10)
project(Metin2MobileUI)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../../include/mobile
)

# Source files for mobile UI library
set(MOBILE_SOURCES
    ../../src/mobile/TouchInput.cpp
    ../../src/mobile/VirtualJoystick.cpp
    ../../src/mobile/UIButton.cpp
    ../../src/mobile/GameControls.cpp
)

# Create mobile UI library
add_library(metin2_mobile_ui STATIC ${MOBILE_SOURCES})

# Test executable - Simple test
add_executable(test_mobile_ui
    test_mobile_ui.cpp
)
target_link_libraries(test_mobile_ui metin2_mobile_ui)

# Integration example executable
add_executable(metin2_mobile_integration
    metin2_mobile_integration.cpp
)
target_link_libraries(metin2_mobile_integration metin2_mobile_ui)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(metin2_mobile_ui PRIVATE -Wall -Wextra)
endif()

# Output directories
set_target_properties(test_mobile_ui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../bin
)

set_target_properties(metin2_mobile_integration PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../bin
)

# Android build (optional)
if(ANDROID)
    message(STATUS "Building for Android")

    # Find Android libraries
    find_library(log-lib log)
    find_library(android-lib android)

    # Link Android libraries
    target_link_libraries(metin2_mobile_ui
        ${log-lib}
        ${android-lib}
    )
endif()

# iOS build (optional)
if(IOS)
    message(STATUS "Building for iOS")

    # iOS specific settings
    set_target_properties(metin2_mobile_ui PROPERTIES
        FRAMEWORK TRUE
        MACOSX_FRAMEWORK_IDENTIFIER com.metin2.mobileui
    )
endif()

# Installation
install(TARGETS test_mobile_ui metin2_mobile_integration
    RUNTIME DESTINATION bin
)

install(TARGETS metin2_mobile_ui
    ARCHIVE DESTINATION lib
)

# Print build info
message(STATUS "")
message(STATUS "=== Metin2 Mobile UI Build Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "==========================================")
message(STATUS "")
