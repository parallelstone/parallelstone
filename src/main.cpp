/**
 * @file main.cpp
 * @brief Entry point for ParellelStone Minecraft Server
 * @details This file contains the main function that initializes and starts the
 *          ParellelStone Minecraft server with complete networking and session management.
 *          Implements high-performance cross-platform networking using platform-specific
 *          optimizations (io_uring on Linux, kqueue on macOS, RIO on Windows).
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 * @since Protocol Version 765 (Minecraft 1.20.4)
 */

#include <iostream>
#include <memory>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <filesystem>
#include <cstdio>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include "platform.hpp"
#include "network/core.hpp"
#include "server/core.hpp"
#include "server/session_manager.hpp"

using namespace Platform;
using namespace parallelstone;

// Global server instance and running flag for signal handling
std::unique_ptr<server::ServerCore> g_server;
std::atomic<bool> g_running{true};
std::atomic<bool> g_shutdown_requested{false};

/**
 * @brief Initialize structured logging system with spdlog
 * @details Sets up multi-sink logging with both console and file output.
 *          Console output includes colored formatting for better readability.
 *          File output uses rotating files to prevent disk space issues.
 * @throws std::runtime_error if logger initialization fails
 */
void InitializeLogging() {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");
        
        // Create console sink with color support
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        
        // Create rotating file sink (10MB max, 5 backup files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/parallelstone.log", 1024 * 1024 * 10, 5);
        file_sink->set_level(spdlog::level::debug);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v");
        
        // Create error file sink for errors and above
        auto error_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/error.log", true);
        error_sink->set_level(spdlog::level::err);
        error_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v");
        
        // Create multi-sink logger
        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink, error_sink};
        auto logger = std::make_shared<spdlog::logger>("parallelstone", sinks.begin(), sinks.end());
        
        // Set logger configuration
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::warn);
        
        // Register as default logger
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        
        // Set global flush policy (flush every 3 seconds)
        spdlog::flush_every(std::chrono::seconds(3));
        
    } catch (const std::exception& e) {
        // Cannot use spdlog here as logging initialization failed
        spdlog::error("[CRITICAL] Failed to initialize logging: {}", e.what());
        throw std::runtime_error("Logging initialization failed");
    }
}

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number received (SIGINT, SIGTERM, etc.)
 * @details Handles system signals for graceful server shutdown. Sets global flags
 *          to indicate shutdown request and triggers server stop sequence.
 * @note This function is called from signal context, so it must be async-signal-safe.
 *       We use direct write to stderr for signal safety instead of spdlog.
 */
void SignalHandler(int signal) {
    const char* signal_name = "UNKNOWN";
    switch (signal) {
        case SIGINT:  signal_name = "SIGINT";  break;
        case SIGTERM: signal_name = "SIGTERM"; break;
        default: break;
    }
    
    // Note: Cannot use spdlog in signal handler (not async-signal-safe)
    // Use async-signal-safe method for logging in signal handler
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), 
                      "\n[INFO] Received signal %d (%s), shutting down gracefully...\n", 
                      signal, signal_name);
    if (len > 0) {
      write(GetPlatformFileDescriptor(PlatformFileDescriptor::STDERR_FD), buffer, len);
    }
    
    g_running = false;
    g_shutdown_requested = true;
    
    if (g_server) {
        g_server->Stop();
    }
}

/**
 * @brief Print server banner with version and platform information
 * @details Displays formatted server information including version, protocol support,
 *          platform details, and build configuration for debugging and monitoring.
 */
void PrintServerBanner() {
    spdlog::info("========================================");
    spdlog::info("  ParellelStone Minecraft Server");
    spdlog::info("========================================");
    spdlog::info("Version: 1.0.0");
    spdlog::info("Protocol: Minecraft Java Edition 1.20.4 (Protocol 765)");
    spdlog::info("Platform: {}", Platform::GetPlatformName());
    spdlog::info("Build: {}", 
#ifdef DEBUG
        "Debug"
#else
        "Release"
#endif
    );
    spdlog::info("Author: @logpacket");
    spdlog::info("========================================");
}

/**
 * @brief Main entry point for the ParellelStone Minecraft server
 * @return int Exit code (0 for success, 1 for error)
 * @details This function initializes the server with complete networking and session management.
 *          It creates a ServerCore instance, configures it with optimal settings, and runs
 *          the main server loop. Implements comprehensive error handling and graceful shutdown.
 * 
 * @example Server startup sequence:
 *          1. Initialize platform-specific network subsystem
 *          2. Configure server with optimal parameters
 *          3. Start server and begin accepting connections
 *          4. Process events and monitor statistics
 *          5. Handle shutdown gracefully on signal
 */
int main() {
    try {
        // Initialize logging system first
        InitializeLogging();
        
        // Display server banner
        PrintServerBanner();
        
        // Set up signal handlers for graceful shutdown
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);
        
        spdlog::info("Initializing server components...");
        
        // Create platform-specific network core
        spdlog::info("Creating platform-specific network core...");
        auto network_core = network::CreateNetworkCore();
        if (!network_core) {
            spdlog::error("Failed to create network core - unsupported platform or insufficient resources");
            return 1;
        }
        
        // Log network implementation details
        spdlog::info("Network implementation: {}", network_core->GetImplementationName());
        
        // Configure network core with optimal settings
        network::NetworkConfig network_config;
        network_config.queue_depth = 512;        // Increased queue depth for better performance
        network_config.enable_nodelay = true;           // Disable Nagle's algorithm for low latency
        network_config.enable_keepalive = true;         // Enable connection monitoring
        // Note: socket_timeout and buffer_size are not part of NetworkConfig struct
        
        spdlog::info("Initializing network core...");
        auto network_result = network_core->Initialize(network_config);
        if (network_result != network::NetworkResult::SUCCESS) {
            spdlog::error("Failed to initialize network core: {}", 
                         network::NetworkResultToString(network_result));
            return 1;
        }
        spdlog::info("Network core initialized successfully");
        
        // Create server configuration with optimal settings
        spdlog::info("Configuring server...");
        server::ServerConfig server_config;
        server_config.SetPort(25565)                           // Standard Minecraft port
                     .SetMaxConnections(1000)                  // Support for 1000 concurrent connections
                     .SetWorkerThreads(std::thread::hardware_concurrency()) // Optimal thread count
                     .SetMotd("ParellelStone High-Performance Minecraft Server"); // Descriptive MOTD
        
        // Additional server configuration
        server_config.online_mode = false;                      // Disable online mode for development
        server_config.max_players = 100;                       // Maximum players
        server_config.protocol_version = 765;                  // Minecraft 1.20.4 protocol
        server_config.enable_tcp_nodelay = true;               // Low latency networking
        server_config.enable_keepalive = true;                 // Connection monitoring
        server_config.io_queue_depth = 512;                    // Match network queue depth
        
        spdlog::info("Server configuration:");
        spdlog::info("  - Port: {}", server_config.port);
        spdlog::info("  - Max connections: {}", server_config.max_connections);
        spdlog::info("  - Worker threads: {}", server_config.worker_threads);
        spdlog::info("  - Protocol version: {}", server_config.protocol_version);
        spdlog::info("  - Online mode: {}", server_config.online_mode ? "enabled" : "disabled");
        
        // Create server instance
        spdlog::info("Creating server instance...");
        g_server = std::make_unique<server::ServerCore>(server_config);
        
        // Start server
        spdlog::info("Starting server...");
        auto start_result = g_server->Start();
        if (start_result != network::NetworkResult::SUCCESS) {
            spdlog::error("Failed to start server: {}", 
                         network::NetworkResultToString(start_result));
            return 1;
        }
        
        spdlog::info("Server started successfully!");
        spdlog::info("Listening on {}:{}", server_config.bind_address, server_config.port);
        spdlog::info("Ready to accept connections");
        spdlog::info("Press Ctrl+C to shutdown gracefully");
        
        // Main server loop with enhanced monitoring
        spdlog::info("Server running. Monitoring connections...");
        
        size_t last_connection_count = 0;
        auto last_stats_time = std::chrono::steady_clock::now();
        auto server_start_time = std::chrono::steady_clock::now();
        
        while (g_running && g_server->GetState() == server::ServerState::RUNNING) {
            try {
                // Process server events
                size_t events_processed = g_server->ProcessEvents();
                
                // Print statistics every 10 seconds
                auto now = std::chrono::steady_clock::now();
                if (now - last_stats_time >= std::chrono::seconds(10)) {
                    const auto& stats = g_server->GetStatistics();
                    
                    // Log connection changes
                    if (stats.active_connections != last_connection_count) {
                        spdlog::info("Active connections: {}, Total processed: {}, Peak: {}", 
                                   stats.active_connections.load(), stats.total_connections.load(), stats.peak_connections.load());
                        last_connection_count = stats.active_connections;
                    }
                    
                    // Log periodic statistics
                    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - server_start_time);
                    if (uptime.count() % 60 == 0 && uptime.count() > 0) { // Every minute
                        spdlog::info("Uptime: {}s, Active: {}, Total: {}, Failed: {}, Bytes sent: {}, Bytes received: {}", 
                                   uptime.count(), stats.active_connections.load(), stats.total_connections.load(), 
                                   stats.failed_connections.load(), stats.bytes_sent.load(), stats.bytes_received.load());
                    }
                    
                    last_stats_time = now;
                }
                
                // Small delay to prevent busy waiting
                Platform::Sleep(10);
                
            } catch (const std::exception& e) {
                spdlog::error("Server loop error: {}", e.what());
                // Continue running unless critical error
                if (g_shutdown_requested) {
                    break;
                }
            }
        }
        
        // Graceful shutdown sequence
        spdlog::info("Initiating graceful shutdown...");
        
        // Stop accepting new connections
        g_server->Stop();
        
        // Wait for existing connections to finish
        auto shutdown_start = std::chrono::steady_clock::now();
        const auto shutdown_timeout = std::chrono::seconds(10);
        
        while (g_server->GetActiveConnectionCount() > 0) {
            auto elapsed = std::chrono::steady_clock::now() - shutdown_start;
            if (elapsed >= shutdown_timeout) {
                spdlog::warn("Shutdown timeout reached, forcing disconnection of remaining clients");
                g_server->DisconnectAllClients("Server shutdown");
                break;
            }
            
            spdlog::info("Waiting for {} connections to close...", g_server->GetActiveConnectionCount());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // Clean up resources
        g_server.reset();
        
        spdlog::info("Server stopped gracefully");
        
    } catch (const std::bad_alloc& e) {
        spdlog::critical("Memory allocation error: {}", e.what());
        spdlog::critical("Server may be out of memory");
        if (g_server) {
            g_server->Stop();
            g_server.reset();
        }
        spdlog::shutdown();
        return 1;
    } catch (const std::runtime_error& e) {
        spdlog::error("Runtime error: {}", e.what());
        if (g_server) {
            g_server->Stop();
            g_server.reset();
        }
        spdlog::shutdown();
        return 1;
    } catch (const std::exception& e) {
        spdlog::error("Unexpected error: {}", e.what());
        if (g_server) {
            g_server->Stop();
            g_server.reset();
        }
        spdlog::shutdown();
        return 1;
    } catch (...) {
        spdlog::critical("Unknown error occurred during server execution");
        if (g_server) {
            g_server->Stop();
            g_server.reset();
        }
        spdlog::shutdown();
        return 1;
    }
    
    // Ensure proper cleanup of logging system
    spdlog::shutdown();
    
    return 0;
}