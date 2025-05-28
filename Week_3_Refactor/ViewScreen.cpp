#include "ViewScreen.h"
#include <iostream>

// Initialize static members
std::unordered_map<std::string, ViewScreen::SessionInfo> ViewScreen::active_sessions;
std::optional<std::string> ViewScreen::current_session;

ViewScreen::ViewScreen() {}

std::string ViewScreen::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    std::tm* time_info = std::localtime(&in_time_t);
    ss << std::put_time(time_info, "%Y-%m-%d %X");
    return ss.str();
}

bool ViewScreen::createSession(const std::string& name) {
    if (isAttached()) {
        std::cout << "Error: Already attached to session '" << *current_session << "'.\n";
        return false;
    }

    if (active_sessions.count(name) > 0) {
        std::cout << "Error: Session '" << name << "' already exists.\n";
        std::cout << "Use 'screen -r " << name << "' to reattach or choose a different name.\n";
        return false;
    }

    SessionInfo new_session;
    new_session.name = name;
    new_session.createdAt = getCurrentTimestamp();
    new_session.currentLine = 1;
    new_session.totalLines = 100; // Default total lines

    active_sessions[name] = new_session;
    std::cout << "Session '" << name << "' created.\n";
    return true;
}

bool ViewScreen::attachToSession(const std::string& name) {
    if (isAttached()) {
        std::cout << "Error: Already attached to session '" << *current_session << "'.\n";
        return false;
    }

    auto it = active_sessions.find(name);
    if (it == active_sessions.end()) {
        std::cout << "Error: Session '" << name << "' not found.\n";
        std::cout << "Use 'screen -ls' to see available screens or 'screen -s " << name << "' to create it.\n";
        return false;
    }

    current_session = name;
    runSessionLoop(it->second);
    return true;
}

void ViewScreen::detachFromSession() {
    if (!isAttached()) {
        std::cout << "Error: Not attached to any session.\n";
        return;
    }

    std::cout << "Detaching from session '" << *current_session << "'.\n";
    current_session = std::nullopt;
}

void ViewScreen::listSessions() const {
    if (active_sessions.empty()) {
        std::cout << "No active sessions.\n";
        return;
    }

    std::cout << "Active sessions:\n";
    for (const auto& [name, session] : active_sessions) {
        std::cout << "  " << name << " (Created: " << session.createdAt << ")\n";
    }
}

bool ViewScreen::isAttached() const {
    return current_session.has_value();
}

std::optional<std::string> ViewScreen::getCurrentSessionName() const {
    return current_session;
}

void ViewScreen::displaySessionInfo(const SessionInfo& session) const {
    std::cout << "\n[SCREEN: " << session.name << "]\n";
    std::cout << "Process Name: " << session.name << "_process (dummy)\n";
    std::cout << "Instruction: Line " << session.currentLine << " / " << session.totalLines << "\n";
    std::cout << "Created At: " << session.createdAt << "\n";
    std::cout << "Type 'exit' to detach and return to main menu.\n";
    std::cout << "[" << session.name << "]$ ";
}

void ViewScreen::processSessionCommand(const std::string& command, SessionInfo& session) {
    if (command == "exit") {
        detachFromSession();
    } else if (command == "help") {
        std::cout << "Available commands:\n";
        std::cout << "  exit    - Detach from current session\n";
        std::cout << "  help    - Show this help message\n";
        std::cout << "  status  - Show session status\n";
    } else if (command == "status") {
        displaySessionInfo(session);
    } else if (!command.empty()) {
        std::cout << "  Screen '" << session.name << "' processing: '" << command << "'\n";
        // Simulate somekind of work
        session.currentLine = (session.currentLine % session.totalLines) + 1;
        std::cout << "  (Work simulated, line advanced)\n";
    }
}

void ViewScreen::runSessionLoop(SessionInfo& session) {
    std::system("cls");  // Clear screen when entering session
    std::cout << "Attached to screen '" << session.name << "'.\n";
    
    while (isAttached()) {
        displaySessionInfo(session);
        
        std::string input;
        std::getline(std::cin, input);
        
        if (std::cin.eof()) {
            std::cout << "Detaching due to EOF...\n";
            detachFromSession();
            break;
        }
        
        processSessionCommand(input, session);
    }

    std::system("cls");
} 