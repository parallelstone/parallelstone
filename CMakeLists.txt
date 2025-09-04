cmake_minimum_required(VERSION 3.20)
project(ParellelStone VERSION 1.0.0 LANGUAGES CXX)

# Cross-platform settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Minecraft version selection
option(MINECRAFT_VERSION "Minecraft version to compile for" 121700)
set(MINECRAFT_VERSION ${MINECRAFT_VERSION} CACHE STRING "Minecraft version (format: XXYYZZ, e.g., 121700 for 1.21.7)")

# Validate and set version
if(MINECRAFT_VERSION EQUAL 120100)
    message(STATUS "Compiling for Minecraft 1.20.1")
    add_definitions(-DMINECRAFT_VERSION=120100)
elseif(MINECRAFT_VERSION EQUAL 120400)
    message(STATUS "Compiling for Minecraft 1.20.4") 
    add_definitions(-DMINECRAFT_VERSION=120400)
elseif(MINECRAFT_VERSION EQUAL 121100)
    message(STATUS "Compiling for Minecraft 1.21.1")
    add_definitions(-DMINECRAFT_VERSION=121100)
elseif(MINECRAFT_VERSION EQUAL 121300)
    message(STATUS "Compiling for Minecraft 1.21.3")
    add_definitions(-DMINECRAFT_VERSION=121300)
elseif(MINECRAFT_VERSION EQUAL 121700)
    message(STATUS "Compiling for Minecraft 1.21.7")
    add_definitions(-DMINECRAFT_VERSION=121700)
else()
    message(WARNING "Unsupported Minecraft version: ${MINECRAFT_VERSION}, defaulting to 1.21.7")
    add_definitions(-DMINECRAFT_VERSION=121700)
    set(MINECRAFT_VERSION 121700)
endif()

# Architecture and platform detection
if(APPLE)
    # macOS: arm64 only
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    message(STATUS "Building for macOS arm64")
elseif(WIN32)
    # Windows: x86_64 only
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "Building for Windows x86_64")
    else()
        message(FATAL_ERROR "Only x86_64 architecture is supported on Windows")
    endif()
    
    # Ensure Windows SDK is found and configured properly
    if(MSVC)
        # Set Windows SDK version if not already set
        if(NOT CMAKE_SYSTEM_VERSION)
            set(CMAKE_SYSTEM_VERSION "10.0")
        endif()
        
        # Force find Windows SDK
        find_path(WINDOWS_H_PATH windows.h
            HINTS
                "$ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/um"
                "$ENV{WindowsSdkDir}/Include/*/um"
                "C:/Program Files (x86)/Windows Kits/10/Include/*/um"
                "C:/Program Files/Windows Kits/10/Include/*/um"
        )
        
        if(WINDOWS_H_PATH)
            message(STATUS "Found windows.h at: ${WINDOWS_H_PATH}")
            # Add Windows SDK include directories explicitly
            get_filename_component(WINDOWS_SDK_INCLUDE_DIR "${WINDOWS_H_PATH}/.." ABSOLUTE)
            include_directories(SYSTEM "${WINDOWS_SDK_INCLUDE_DIR}/um")
            include_directories(SYSTEM "${WINDOWS_SDK_INCLUDE_DIR}/shared")
            include_directories(SYSTEM "${WINDOWS_SDK_INCLUDE_DIR}/ucrt")
        else()
            message(WARNING "Windows SDK headers not found. Please ensure Windows 10 SDK is installed.")
        endif()
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux: support both x86_64 and arm64
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
        message(STATUS "Building for Linux arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)")
        message(STATUS "Building for Linux x86_64")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}. Only x86_64 and arm64 are supported on Linux")
    endif()
endif()

# Platform-specific compiler flags
if(MSVC)
    # Windows (Visual Studio)
    set(CMAKE_CXX_FLAGS "/W4 /EHsc /utf-8")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00)
    
    # Additional compiler definitions for better Windows compatibility
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -DNTDDI_VERSION=0x0A000000)
else()
    # GCC/Clang (Linux/macOS)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    
    # Additional flags for better security and performance
    if(NOT APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,relro,-z,now")
    endif()
endif()

# Position Independent Code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files (excluding test files)
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include)

# Find required packages
find_package(Threads REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Platform-specific linking
if(WIN32)
    # Windows specific libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux specific libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE rt)
elseif(APPLE)
    # macOS specific settings (if needed)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    if(COREFOUNDATION_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${COREFOUNDATION_LIBRARY})
    endif()
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Test configuration
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # Find Google Test
    find_package(GTest REQUIRED CONFIG)
    
    # Create a separate library for core functionality (excluding main.cpp)
    set(CORE_SOURCES ${SOURCES})
    list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
    
    if(CORE_SOURCES)
        add_library(${PROJECT_NAME}_core STATIC ${CORE_SOURCES} ${HEADERS})
        target_include_directories(${PROJECT_NAME}_core PUBLIC include)
        target_link_libraries(${PROJECT_NAME}_core PUBLIC 
            Threads::Threads
            spdlog::spdlog
        )
        
        # Platform-specific linking for core library
        if(WIN32)
            target_link_libraries(${PROJECT_NAME}_core PUBLIC ws2_32 wsock32)
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_link_libraries(${PROJECT_NAME}_core PUBLIC rt)
        elseif(APPLE)
            if(COREFOUNDATION_LIBRARY)
                target_link_libraries(${PROJECT_NAME}_core PUBLIC ${COREFOUNDATION_LIBRARY})
            endif()
        endif()
        
        # Update main executable to use core library
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
    endif()
    
    # Collect test files
    file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")
    
    # Filter out old standalone test files and CMakeLists.txt
    list(FILTER TEST_SOURCES EXCLUDE REGEX "CMakeLists\\.txt$")
    
    if(TEST_SOURCES)
        # Create test executable
        add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})
        
        target_include_directories(${PROJECT_NAME}_tests PRIVATE include)
        
        # Link with core library if it exists, otherwise with individual sources
        if(CORE_SOURCES)
            target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_core)
        else()
            target_link_libraries(${PROJECT_NAME}_tests PRIVATE 
                Threads::Threads
                spdlog::spdlog
            )
            # Platform-specific linking for tests when no core library
            if(WIN32)
                target_link_libraries(${PROJECT_NAME}_tests PRIVATE ws2_32 wsock32)
            elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
                target_link_libraries(${PROJECT_NAME}_tests PRIVATE rt)
            elseif(APPLE)
                if(COREFOUNDATION_LIBRARY)
                    target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${COREFOUNDATION_LIBRARY})
                endif()
            endif()
        endif()
        
        # Link with Google Test
        target_link_libraries(${PROJECT_NAME}_tests PRIVATE
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
            GTest::gmock_main
        )
        
        # Set test properties
        set_target_properties(${PROJECT_NAME}_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
        
        # Add test to CTest with better discovery
        include(GoogleTest)
        gtest_discover_tests(${PROJECT_NAME}_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            PROPERTIES
                TIMEOUT 60
        )
    else()
        message(STATUS "No test files found, skipping test executable creation")
    endif()
endif()