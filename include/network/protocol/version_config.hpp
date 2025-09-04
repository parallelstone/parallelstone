#pragma once

// default protocol version
#ifndef PROTOCOL_VERSION
#define PROTOCOL_VERSION PROTOCOL_1_21_7
#endif

// Version identification macros
#define PROTOCOL_1_20_4   765
#define PROTOCOL_1_21_7   772

// Include the appropriate protocol header based on the version
#if PROTOCOL_VERSION == PROTOCOL_1_20_4
    #include "protocol/1_20_4.hpp"
    namespace current_protocol = protocol::v1_20_4;
#elif PROTOCOL_VERSION == PROTOCOL_1_21_7
    #include "protocol/1_21_7.hpp"
    namespace current_protocol = protocol::v1_21_7;
#else
    #error "Unsupported protocol version. Please define PROTOCOL_VERSION to one of the supported versions."
#endif

// Protocol version information and feature flags

// Version information functions
constexpr int GetProtocolVersion() {
    return PROTOCOL_VERSION;
}

constexpr const char* GetVersionString() {
#if PROTOCOL_VERSION == PROTOCOL_1_20_4
    return "1.20.4";
#elif PROTOCOL_VERSION == PROTOCOL_1_21_7
    return "1.21.7";
#else
    return "Unknown";
#endif
}

// Feature flags (all current versions support these)
constexpr bool HasConfigurationState() {
    return true;
}

constexpr bool HasLoginPluginMessages() {
    return true;
}

constexpr bool HasCookies() {
    return true;
}

// Protocol version checks
constexpr bool IsProtocolVersion(int version) {
    return PROTOCOL_VERSION == version;
}