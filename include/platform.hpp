#pragma once

// Platform detection macros
#ifdef _WIN32
    #define PLATFORM_WINDOWS
    #ifdef _WIN64
        #define PLATFORM_WIN64
    #else
        #define PLATFORM_WIN32
    #endif
#elif defined(__linux__)
    #define PLATFORM_LINUX
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define PLATFORM_MACOS
    #endif
#endif

// Compiler detection
#ifdef _MSC_VER
    #define COMPILER_MSVC
#elif defined(__GNUC__)
    #define COMPILER_GCC
#elif defined(__clang__)
    #define COMPILER_CLANG
#endif

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif defined(PLATFORM_LINUX)
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#elif defined(PLATFORM_MACOS)
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

// Platform-specific sleep function
namespace Platform {
    inline void Sleep(int milliseconds) {
        #ifdef PLATFORM_WINDOWS
            ::Sleep(milliseconds);
        #else
            usleep(milliseconds * 1000);
        #endif
    }
    
    inline const char* GetPlatformName() {
        #ifdef PLATFORM_WINDOWS
            return "Windows";
        #elif defined(PLATFORM_LINUX)
            return "Linux";
        #elif defined(PLATFORM_MACOS)
            return "macOS";
        #else
            return "Unknown";
        #endif
    }
}