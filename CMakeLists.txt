cmake_minimum_required(VERSION 3.20)
project(ParellelStone VERSION 1.0.0 LANGUAGES CXX)

# Cross-platform settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
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
    set(CMAKE_CXX_FLAGS "/W4 /EHsc")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
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

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include)

# Find required packages
find_package(Threads REQUIRED)

# Try to find Boost using different methods
if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    # Using vcpkg toolchain
    find_package(Boost REQUIRED COMPONENTS system thread)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed")
    # Direct vcpkg paths
    if(APPLE)
        set(VCPKG_TARGET_TRIPLET "arm64-osx")
    elseif(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
        set(VCPKG_TARGET_TRIPLET "arm64-linux")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()
    
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    set(Boost_INCLUDE_DIR "${BOOST_ROOT}/include")
    set(Boost_LIBRARY_DIR "${BOOST_ROOT}/lib")
    
    find_library(Boost_SYSTEM_LIBRARY NAMES boost_system libboost_system PATHS ${Boost_LIBRARY_DIR} NO_DEFAULT_PATH)
    find_library(Boost_THREAD_LIBRARY NAMES boost_thread libboost_thread PATHS ${Boost_LIBRARY_DIR} NO_DEFAULT_PATH)
    
    if(Boost_SYSTEM_LIBRARY AND Boost_THREAD_LIBRARY AND EXISTS ${Boost_INCLUDE_DIR})
        message(STATUS "Found Boost: ${BOOST_ROOT}")
        add_library(Boost::system STATIC IMPORTED)
        set_target_properties(Boost::system PROPERTIES IMPORTED_LOCATION ${Boost_SYSTEM_LIBRARY})
        target_include_directories(Boost::system INTERFACE ${Boost_INCLUDE_DIR})
        
        add_library(Boost::thread STATIC IMPORTED)
        set_target_properties(Boost::thread PROPERTIES IMPORTED_LOCATION ${Boost_THREAD_LIBRARY})
        target_include_directories(Boost::thread INTERFACE ${Boost_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Could not find Boost libraries in ${BOOST_ROOT}")
    endif()
else()
    # System-wide Boost
    find_package(Boost REQUIRED COMPONENTS system thread)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Threads::Threads
    Boost::system
    Boost::thread
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

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()