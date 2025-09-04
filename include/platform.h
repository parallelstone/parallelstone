/**
 * @file platform.hpp
 * @brief Cross-platform compatibility header for platform detection and utilities
 * 
 * This header provides macros for platform detection (Windows, Linux, macOS),
 * compiler detection (MSVC, GCC, Clang), and platform-specific utility functions.
 * It ensures consistent behavior across different operating systems and compilers.
 * 
 * @author @logpacket https://github.com/logpacket
 * @date 2025/07/07
 */

#pragma once

/**
 * @defgroup PlatformDetection Platform Detection Macros
 * @brief Macros for detecting the target platform at compile time
 * @{
 */

// Platform detection macros
#ifdef _WIN32
/** @brief Defined when compiling for Windows platform */
#define PLATFORM_WINDOWS
#ifdef _WIN64
/** @brief Defined when compiling for 64-bit Windows */
#define PLATFORM_WIN64
#else
/** @brief Defined when compiling for 32-bit Windows */
#define PLATFORM_WIN32
#endif
#elif defined(__linux__)
/** @brief Defined when compiling for Linux platform */
#define PLATFORM_LINUX
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
/** @brief Defined when compiling for macOS platform */
#define PLATFORM_MACOS
#endif
#endif

/** @} */ // end of PlatformDetection group

/**
 * @defgroup CompilerDetection Compiler Detection Macros
 * @brief Macros for detecting the compiler being used
 * @{
 */

// Compiler detection
#ifdef _MSC_VER
/** @brief Defined when using Microsoft Visual C++ compiler */
#define COMPILER_MSVC
#elif defined(__GNUC__)
/** @brief Defined when using GNU Compiler Collection (GCC) */
#define COMPILER_GCC
#elif defined(__clang__)
/** @brief Defined when using Clang compiler */
#define COMPILER_CLANG
#endif

/** @} */ // end of CompilerDetection group

/**
 * @defgroup PlatformIncludes Platform-Specific System Headers
 * @brief Platform-specific header file inclusions
 * @{
 */

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
/** @brief Exclude rarely-used stuff from Windows headers */
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
/** @brief Prevent Windows.h from defining min/max macros */
#define NOMINMAX
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(PLATFORM_LINUX)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(PLATFORM_MACOS)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/** @} */ // end of PlatformIncludes group

/**
 * @namespace Platform
 * @brief Platform abstraction utilities and functions
 * 
 * This namespace provides cross-platform utility functions that abstract
 * platform-specific operations like sleeping and platform identification.
 */
namespace Platform {

/**
 * @brief Cross-platform sleep function
 * @param milliseconds The number of milliseconds to sleep
 * 
 * This function provides a unified interface for sleeping/pausing execution
 * across different platforms. On Windows, it uses Sleep() from the Windows API,
 * while on Unix-like systems (Linux/macOS), it uses usleep() from unistd.h.
 * 
 * @note The precision may vary between platforms. Windows Sleep() has
 *       millisecond precision, while usleep() has microsecond precision.
 * 
 * @example
 * @code
 * // Sleep for 1 second (1000 milliseconds)
 * Platform::Sleep(1000);
 * @endcode
 */
inline void Sleep(int milliseconds) {
#ifdef PLATFORM_WINDOWS
  ::Sleep(milliseconds);
#else
  usleep(milliseconds * 1000);
#endif
}

/**
 * @brief Get the name of the current platform
 * @return const char* A string representing the platform name
 * 
 * This function returns a human-readable string identifying the current
 * platform. The returned string is one of:
 * - "Windows" for Windows platforms
 * - "Linux" for Linux platforms  
 * - "macOS" for macOS platforms
 * - "Unknown" for unrecognized platforms
 * 
 * @note The returned pointer points to a static string and does not
 *       need to be freed.
 * 
 * @example
 * @code
 * std::cout << "Running on: " << Platform::GetPlatformName() << std::endl;
 * @endcode
 */
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

enum PlatformFileDescriptor {
  INVALID_FD = -1,  ///< Invalid file descriptor
  STDIN_FD = 0,     ///< Standard input file descriptor
  STDOUT_FD = 1,    ///< Standard output file descriptor
  STDERR_FD = 2     ///< Standard error file descriptor
};

inline const int GetPlatformFileDescriptor(PlatformFileDescriptor fd) {
#ifdef PLATFORM_WINDOWS
  switch (fd) {
	case INVALID_FD: return -1;
	case STDIN_FD: return _fileno(stdin);
	case STDOUT_FD: return _fileno(stdout);
	case STDERR_FD: return _fileno(stderr);
	default: return -1; // Unknown descriptor
  }
#else
  switch (fd) {
	case INVALID_FD: return -1;
	case STDIN_FD: return STDIN_FILENO;
	case STDOUT_FD: return STDOUT_FILENO;
	case STDERR_FD: return STDERR_FILENO;
	default: return -1; // Unknown descriptor
  }
#endif
}

}  // namespace Platform