/**
 * @file session_manager.cpp
 * @brief Implementation of session management for ParellelStone server
 * @details This file implements the SessionManager class that manages all
 *          active client sessions and provides coordination functionality.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#include "server/session_manager.hpp"
#include "server/session.hpp"
#include "network/buffer.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstddef>
#include <cstdint>

namespace parallelstone {
namespace server {

SessionManager::SessionManager(const SessionManagerConfig& config, 
                             std::shared_ptr<network::INetworkCore> network_core)
    : m_config(config)
    , m_statistics()
    , m_network_core(network_core)
    , m_running(false)
    , m_next_session_id(1) {
    
    // Validate configuration
    if (m_config.max_sessions > MAX_SESSIONS_ABSOLUTE) {
        m_config.max_sessions = MAX_SESSIONS_ABSOLUTE;
    }
    
    if (m_config.max_sessions_per_ip > MAX_SESSIONS_PER_IP_ABSOLUTE) {
        m_config.max_sessions_per_ip = MAX_SESSIONS_PER_IP_ABSOLUTE;
    }
    
    // Initialize statistics
    m_statistics.Reset();
}

SessionManager::~SessionManager() {
    Stop();
}

void SessionManager::Start() {
    if (m_running.load()) {
        return;
    }
    
    m_running = true;
    
    // Start cleanup thread if auto cleanup is enabled
    if (m_config.enable_auto_cleanup) {
        m_cleanup_thread = std::thread(&SessionManager::CleanupThreadMain, this);
    }
    
    // Start heartbeat thread
    m_heartbeat_thread = std::thread(&SessionManager::HeartbeatThreadMain, this);
    
    EmitSessionEvent(nullptr, "Session manager started");
}

void SessionManager::Stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running = false;
    
    // Disconnect all sessions
    DisconnectAllSessions(DisconnectReason::SERVER_SHUTDOWN, "Session manager shutdown");
    
    // Stop threads
    if (m_cleanup_thread.joinable()) {
        m_cleanup_thread.join();
    }
    
    if (m_heartbeat_thread.joinable()) {
        m_heartbeat_thread.join();
    }
    
    // Clear all sessions
    std::unique_lock<std::shared_mutex> lock(m_sessions_mutex);
    m_sessions.clear();
    m_socket_to_session.clear();
    m_player_to_session.clear();
    m_ip_sessions.clear();
    
    EmitSessionEvent(nullptr, "Session manager stopped");
}

std::shared_ptr<Session> SessionManager::CreateSession(network::SocketType socket, const std::string& client_ip) {
    // Check capacity
    if (IsAtCapacity()) {
        m_statistics.rejected_sessions++;
        return nullptr;
    }
    
    // Check IP limit if enabled
    if (m_config.enable_ip_limiting && !client_ip.empty() && IsIPAtLimit(client_ip)) {
        m_statistics.rejected_sessions++;
        return nullptr;
    }
    
    // Create new session
    auto session = std::make_shared<Session>(socket, *this);
    
    // Set up callbacks
    session->SetDisconnectCallback(
        [this](std::shared_ptr<Session> session, DisconnectReason reason) {
            OnSessionDisconnect(session, reason);
        });
    
    session->SetStateChangeCallback(
        [this](std::shared_ptr<Session> session, SessionState old_state, SessionState new_state) {
            OnSessionStateChange(session, old_state, new_state);
        });
    
    // Add to tracking
    AddSessionInternal(session);
    
    // Update statistics
    m_statistics.total_sessions++;
    m_statistics.active_sessions++;
    
    size_t current_sessions = m_statistics.active_sessions.load();
    size_t peak = m_statistics.peak_sessions.load();
    if (current_sessions > peak) {
        m_statistics.peak_sessions = current_sessions;
    }
    
    // Start the session
    session->Start();
    
    EmitSessionEvent(session, "Session created");
    
    return session;
}

bool SessionManager::RemoveSession(const std::string& session_id) {
    std::unique_lock<std::shared_mutex> lock(m_sessions_mutex);
    return RemoveSessionInternal(session_id);
}

bool SessionManager::RemoveSession(std::shared_ptr<Session> session) {
    if (!session) {
        return false;
    }
    
    return RemoveSession(session->GetSessionId());
}

std::shared_ptr<Session> SessionManager::GetSession(const std::string& session_id) const {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    auto it = m_sessions.find(session_id);
    return (it != m_sessions.end()) ? it->second : nullptr;
}

std::shared_ptr<Session> SessionManager::GetSessionByPlayer(const std::string& player_name) const {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    auto it = m_player_to_session.find(player_name);
    if (it != m_player_to_session.end()) {
        auto session_it = m_sessions.find(it->second);
        return (session_it != m_sessions.end()) ? session_it->second : nullptr;
    }
    
    return nullptr;
}

std::shared_ptr<Session> SessionManager::GetSessionBySocket(network::SocketType socket) const {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    auto it = m_socket_to_session.find(socket);
    if (it != m_socket_to_session.end()) {
        auto session_it = m_sessions.find(it->second);
        return (session_it != m_sessions.end()) ? session_it->second : nullptr;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<Session>> SessionManager::GetAllSessions() const {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    std::vector<std::shared_ptr<Session>> sessions;
    sessions.reserve(m_sessions.size());
    
    for (const auto& pair : m_sessions) {
        sessions.push_back(pair.second);
    }
    
    return sessions;
}

std::vector<std::shared_ptr<Session>> SessionManager::FindSessions(const SessionQuery& query) const {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    std::vector<std::shared_ptr<Session>> result;
    
    for (const auto& pair : m_sessions) {
        auto session = pair.second;
        bool matches = true;
        
        // Check state filter
        if (query.state && session->GetState() != *query.state) {
            matches = false;
        }
        
        // Check player name filter
        if (query.player_name && session->GetInfo().player_name != *query.player_name) {
            matches = false;
        }
        
        // Check client IP filter
        if (query.client_ip && session->GetInfo().client_ip != *query.client_ip) {
            matches = false;
        }
        
        // Check minimum duration filter
        if (query.min_duration && session->GetInfo().GetDuration() < *query.min_duration) {
            matches = false;
        }
        
        // Check maximum idle time filter
        if (query.max_idle && session->GetInfo().GetIdleTime() > *query.max_idle) {
            matches = false;
        }
        
        if (matches) {
            result.push_back(session);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<Session>> SessionManager::GetSessionsByIP(const std::string& client_ip) const {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    std::vector<std::shared_ptr<Session>> result;
    
    auto it = m_ip_sessions.find(client_ip);
    if (it != m_ip_sessions.end()) {
        for (const std::string& session_id : it->second) {
            auto session_it = m_sessions.find(session_id);
            if (session_it != m_sessions.end()) {
                result.push_back(session_it->second);
            }
        }
    }
    
    return result;
}

size_t SessionManager::BroadcastPacket(const std::vector<uint8_t>& packet_data, int32_t packet_id,
                                      SessionState target_state) {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    size_t sent_count = 0;
    
    for (const auto& pair : m_sessions) {
        auto session = pair.second;
        if (session->GetState() == target_state && session->IsActive()) {
            // Create buffer from packet data
            parallelstone::network::Buffer packet_buffer(packet_data);
            session->Send(packet_buffer);
            sent_count++;
        }
    }
    
    return sent_count;
}

size_t SessionManager::DisconnectAllSessions(DisconnectReason reason, const std::string& message) {
    auto sessions = GetAllSessions();
    
    for (auto session : sessions) {
        if (session->IsActive()) {
            session->Disconnect(reason, message);
        }
    }
    
    return sessions.size();
}

size_t SessionManager::DisconnectSessionsByIP(const std::string& client_ip,
                                             DisconnectReason reason, const std::string& message) {
    auto sessions = GetSessionsByIP(client_ip);
    
    for (auto session : sessions) {
        if (session->IsActive()) {
            session->Disconnect(reason, message);
        }
    }
    
    return sessions.size();
}

void SessionManager::ProcessAllSessions() {
    auto sessions = GetAllSessions();
    
    for (auto session : sessions) {
        session->ProcessEvents();
    }
}

size_t SessionManager::CleanupSessions() {
    std::vector<std::string> sessions_to_remove;
    
    {
        std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
        
        for (const auto& pair : m_sessions) {
            auto session = pair.second;
            
            // Check if session is disconnected
            if (session->GetState() == SessionState::DISCONNECTED) {
                sessions_to_remove.push_back(pair.first);
                continue;
            }
            
            // Check for timeout
            if (session->HasTimedOut(m_config.session_timeout)) {
                sessions_to_remove.push_back(pair.first);
                continue;
            }
        }
    }
    
    // Remove sessions (requires write lock)
    for (const std::string& session_id : sessions_to_remove) {
        auto session = GetSession(session_id);
        if (session && session->IsActive()) {
            session->Disconnect(DisconnectReason::TIMEOUT, "Session cleanup");
        }
        RemoveSession(session_id);
    }
    
    if (!sessions_to_remove.empty()) {
        m_statistics.cleanup_runs++;
        m_statistics.timed_out_sessions += sessions_to_remove.size();
    }
    
    return sessions_to_remove.size();
}

bool SessionManager::IsIPAtLimit(const std::string& client_ip) const {
    if (!m_config.enable_ip_limiting || client_ip.empty()) {
        return false;
    }
    
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    auto it = m_ip_sessions.find(client_ip);
    if (it != m_ip_sessions.end()) {
        return it->second.size() >= m_config.max_sessions_per_ip;
    }
    
    return false;
}

void SessionManager::UpdateStatistics() {
    std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    m_statistics.active_sessions = m_sessions.size();
    
    // Count sessions by state
    size_t handshaking = 0, status = 0, login = 0, configuration = 0, play = 0;
    
    for (const auto& pair : m_sessions) {
        switch (pair.second->GetState()) {
            case SessionState::HANDSHAKING: handshaking++; break;
            case SessionState::STATUS: status++; break;
            case SessionState::LOGIN: login++; break;
            case SessionState::CONFIGURATION: configuration++; break;
            case SessionState::PLAY: play++; break;
            default: break;
        }
    }
    
    m_statistics.handshaking_sessions = handshaking;
    m_statistics.status_sessions = status;
    m_statistics.login_sessions = login;
    m_statistics.configuration_sessions = configuration;
    m_statistics.play_sessions = play;
}

void SessionManager::OnSessionDisconnect(std::shared_ptr<Session> session, DisconnectReason reason) {
    RemoveSession(session->GetSessionId());
    
    m_statistics.active_sessions--;
    
    std::ostringstream oss;
    oss << "Session disconnected: " << DisconnectReasonToString(reason);
    EmitSessionEvent(session, oss.str());
}

void SessionManager::OnSessionStateChange(std::shared_ptr<Session> session, 
                                        SessionState old_state, SessionState new_state) {
    // Update player name mapping when entering login/play state
    if (new_state == SessionState::PLAY && !session->GetInfo().player_name.empty()) {
        std::unique_lock<std::shared_mutex> lock(m_sessions_mutex);
        m_player_to_session[session->GetInfo().player_name] = session->GetSessionId();
    }
    
    std::ostringstream oss;
    oss << "State change: " << SessionStateToString(old_state) 
        << " -> " << SessionStateToString(new_state);
    EmitSessionEvent(session, oss.str());
}

void SessionManager::CleanupThreadMain() {
    while (m_running.load()) {
        try {
            CleanupSessions();
            std::this_thread::sleep_for(m_config.cleanup_interval);
        } catch (const std::exception& e) {
            EmitSessionEvent(nullptr, "Cleanup thread error: " + std::string(e.what()));
        }
    }
}

void SessionManager::HeartbeatThreadMain() {
    while (m_running.load()) {
        try {
            UpdateStatistics();
            std::this_thread::sleep_for(m_config.heartbeat_interval);
        } catch (const std::exception& e) {
            EmitSessionEvent(nullptr, "Heartbeat thread error: " + std::string(e.what()));
        }
    }
}

std::string SessionManager::GenerateSessionId() {
    uint64_t id = m_next_session_id.fetch_add(1);
    
    std::ostringstream oss;
    oss << "session_" << std::setfill('0') << std::setw(8) << id;
    
    return oss.str();
}

void SessionManager::AddSessionInternal(std::shared_ptr<Session> session) {
    std::unique_lock<std::shared_mutex> lock(m_sessions_mutex);
    
    const std::string& session_id = session->GetSessionId();
    
    // Add to main session map
    m_sessions[session_id] = session;
    
    // Add socket mapping
    m_socket_to_session[session->GetSocket()] = session_id;
    
    // Add IP mapping
    const std::string& client_ip = session->GetInfo().client_ip;
    if (!client_ip.empty() && client_ip != "unknown") {
        m_ip_sessions[client_ip].insert(session_id);
    }
}

bool SessionManager::RemoveSessionInternal(const std::string& session_id) {
    auto it = m_sessions.find(session_id);
    if (it == m_sessions.end()) {
        return false;
    }
    
    auto session = it->second;
    
    // Remove from main session map
    m_sessions.erase(it);
    
    // Remove socket mapping
    m_socket_to_session.erase(session->GetSocket());
    
    // Remove player name mapping
    const std::string& player_name = session->GetInfo().player_name;
    if (!player_name.empty()) {
        m_player_to_session.erase(player_name);
    }
    
    // Remove IP mapping
    const std::string& client_ip = session->GetInfo().client_ip;
    if (!client_ip.empty() && client_ip != "unknown") {
        auto ip_it = m_ip_sessions.find(client_ip);
        if (ip_it != m_ip_sessions.end()) {
            ip_it->second.erase(session_id);
            if (ip_it->second.empty()) {
                m_ip_sessions.erase(ip_it);
            }
        }
    }
    
    return true;
}

void SessionManager::EmitSessionEvent(std::shared_ptr<Session> session, const std::string& event_message) {
    if (m_session_event_callback) {
        m_session_event_callback(session, event_message);
    } else {
        // Use spdlog for default logging
        if (session) {
            spdlog::info("[SessionManager] [{}] {}", session->GetSessionId(), event_message);
        } else {
            spdlog::info("[SessionManager] {}", event_message);
        }
    }
}

// Utility functions
SessionManagerConfig CreateDefaultSessionManagerConfig() {
    SessionManagerConfig config;
    config.SetMaxSessions(1000)
          .SetSessionTimeout(std::chrono::minutes(5))
          .SetCleanupInterval(std::chrono::seconds(30));
    
    return config;
}

} // namespace server
} // namespace parallelstone