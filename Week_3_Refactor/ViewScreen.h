#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

class ViewScreen {
public:
    // Constructor and destructor
    ViewScreen();
    ~ViewScreen() = default;

    // Screen session management
    bool createSession(const std::string& name);
    bool attachToSession(const std::string& name);
    void detachFromSession();
    void listSessions() const;
    bool isAttached() const;
    std::optional<std::string> getCurrentSessionName() const;

    // Session information
    struct SessionInfo {
        std::string name;
        std::string createdAt;
        int currentLine;
        int totalLines;
    };

private:
    // Static members for session management
    static std::unordered_map<std::string, SessionInfo> active_sessions;
    static std::optional<std::string> current_session;

    // Helper methods
    std::string getCurrentTimestamp() const;
    void runSessionLoop(SessionInfo& session);
    void displaySessionInfo(const SessionInfo& session) const;
    void processSessionCommand(const std::string& command, SessionInfo& session);
}; 