#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <optional>
#include <algorithm> // std::find
#include <cstdlib>   // system()
#include <chrono>    // std::chrono
#include <iomanip>   // std::put_time
#include <ctime>     // std::localtime

void printHeader() {
    std::cout << "  _____  _____  ____  _____  ______  _______     __\n";
    std::cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\   / /\n";
    std::cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\_/ / \n";
    std::cout << "| |     \\___ \\| |  | |  ___/|  __|  \\___ \\  \\   /  \n";
    std::cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   \n";
    std::cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   \n";
    std::cout << "Welcome to the Command-Line Operating System!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen, 'help' for commands.\n";
}

struct CommandArgs {
    std::string command;
    std::vector<std::string> args;
};

class CommandParser {
public:
    static CommandArgs parse(const std::string& input) {
        CommandArgs result;
        std::istringstream iss(input);
        std::string token;
        
        if (!(iss >> result.command)) { 
            return result; 
        }
        
        while (iss >> token) {
            result.args.push_back(token);
        }
        
        return result;
    }
};

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(const std::vector<std::string>& args) = 0;
    virtual std::string getHelp() const = 0;
};

class ArgParser {
private:
    std::vector<std::string> args_copy; 

public:
    ArgParser(const std::vector<std::string>& args) : args_copy(args) {}
    
    bool hasFlag(const std::string& flag) const {
        return std::find(args_copy.begin(), args_copy.end(), flag) != args_copy.end();
    }
    
    std::optional<std::string> getValueForFlag(const std::string& flag) const {
        auto it = std::find(args_copy.begin(), args_copy.end(), flag);
        if (it != args_copy.end() && std::next(it) != args_copy.end()) {
            // Ensure the next token is not another flag (basic check)
            if (std::next(it)->rfind('-', 0) != 0) {
                 return *std::next(it);
            }
        }
        return std::nullopt;
    }
};

class ScreenCommand : public ICommand {
private:
    struct ScreenInfo {
        std::string name;
        std::string createdAt;
        int currentLine;
        int totalLines;
        // Add other screen-specific data here if needed
    };

    // Static members to hold state across command calls
    static std::unordered_map<std::string, ScreenInfo> active_screens;
    static std::optional<std::string> currently_attached_screen_name;

    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

    void runScreenSession(ScreenInfo& screen_ref) { // Pass by reference to update
        std::system("cls");
        bool screen_running = true;
        std::cout << "Attached to screen '" << screen_ref.name << "'.\n";

        while (screen_running) {
            std::cout << "\n[SCREEN: " << screen_ref.name << "]\n";
            std::cout << "Process Name: " << screen_ref.name << "_process (dummy)\n";
            std::cout << "Instruction: Line " << screen_ref.currentLine << " / " << screen_ref.totalLines << "\n";
            std::cout << "Created At: " << screen_ref.createdAt << "\n";
            std::cout << "Type 'exit' to detach and return to main menu.\n";
            std::cout << "[" << screen_ref.name << "]$ "; // Screen-specific prompt

            std::string screen_input_line;
            std::getline(std::cin, screen_input_line);

            if (std::cin.eof()){ // Handle Ctrl+D as exit
                std::cout << "Detaching due to EOF...\n";
                screen_running = false;
                break;
            }

            if (screen_input_line == "exit") {
                screen_running = false;
            } else if (screen_input_line.empty()) {
                // Just show prompt again
            }
            else {
                std::cout << "  Screen '" << screen_ref.name << "' processing: '" << screen_input_line << "'\n";
                // Simulate work by advancing the line
                screen_ref.currentLine = (screen_ref.currentLine % screen_ref.totalLines) + 1;
                std::cout << "  (Work simulated, line advanced)\n";
            }
        }
        std::cout << "Detaching from screen '" << screen_ref.name << "'.\n";

        std::system("cls");

        printHeader();
    }

public:
    void execute(const std::vector<std::string>& args) override {
        ArgParser parser(args);

        if (currently_attached_screen_name.has_value()) {
            std::cout << "Error: You are already in screen session '" << *currently_attached_screen_name << "'.\n";
            std::cout << "Type 'exit' within that screen session to return to the main CLI first.\n";
            return;
        }

        if (parser.hasFlag("-s")) {
            auto nameOpt = parser.getValueForFlag("-s");
            if (!nameOpt || nameOpt->empty()) {
                std::cout << "Error: -s flag requires a screen name.\nUsage: screen -s <screen_name>\n";
                return;
            }
            std::string name = *nameOpt;
            if (active_screens.count(name)) {
                std::cout << "Error: Screen with name '" << name << "' already exists.\n";
                std::cout << "Use 'screen -r " << name << "' to reattach or choose a different name.\n";
                return;
            }
            
            ScreenInfo new_screen;
            new_screen.name = name;
            new_screen.createdAt = getCurrentTimestamp();
            new_screen.currentLine = 1;
            new_screen.totalLines = 100; // Dummy total lines
            active_screens[name] = new_screen;
            
            std::cout << "Screen '" << name << "' created.\n";
            
            currently_attached_screen_name = name;
            runScreenSession(active_screens.at(name)); // Pass by reference
            currently_attached_screen_name = std::nullopt; // Detach after session ends

        } else if (parser.hasFlag("-r")) {
            auto nameOpt = parser.getValueForFlag("-r");
            if (!nameOpt || nameOpt->empty()) {
                std::cout << "Error: -r flag requires a screen name.\nUsage: screen -r <screen_name>\n";
                return;
            }
            std::string name = *nameOpt;
            auto it = active_screens.find(name);
            if (it == active_screens.end()) {
                std::cout << "Error: Screen with name '" << name << "' not found.\n";
                std::cout << "Use 'screen -ls' to see available screens or 'screen -s " << name << "' to create it.\n";
                return;
            }

            currently_attached_screen_name = name;
            runScreenSession(it->second); // Pass by reference
            currently_attached_screen_name = std::nullopt; // Detach after session ends

        } else if (parser.hasFlag("-ls")) {
            if (active_screens.empty()) {
                std::cout << "No active screens.\n";
            } else {
                std::cout << "Available screens:\n";
                for (const auto& pair : active_screens) {
                    std::cout << "  - " << pair.first 
                              << " (Created: " << pair.second.createdAt 
                              << ", Lines: " << pair.second.currentLine << "/" << pair.second.totalLines << ")\n";
                }
            }
        } else {
            std::cout << getHelp() << std::endl;
        }
    }

    std::string getHelp() const override {
        return "screen -s <name>         : Create and attach to a new screen session.\n"
               "       screen -r <name>         : Reattach to an existing screen session.\n"
               "       screen -ls             : List all active screen sessions.";
    }
};

// Define static members of ScreenCommand
std::unordered_map<std::string, ScreenCommand::ScreenInfo> ScreenCommand::active_screens;
std::optional<std::string> ScreenCommand::currently_attached_screen_name;


class InitializeCommand : public ICommand {
public:
    void execute(const std::vector<std::string>& args) override {
        std::cout << "'initialize' command recognized. Doing something.\n";
        ArgParser parser(args);
        if (parser.hasFlag("--system")) {
            std::cout << "  System-wide initialization sequence started.\n";
        } else if (auto module = parser.getValueForFlag("--module")) {
            std::cout << "  Initializing module: " << module.value() << "\n";
        } else if (args.empty()){
            // std::cout << "  Use 'initialize --system' or 'initialize --module <name>'.\n";
        } else {
            std::cout << "  Unrecognized initialization parameters: ";
            for(const auto& arg : args) {
                std::cout << arg << " ";
            }
            std::cout << "\n  Hint: Try 'initialize --system' or 'initialize --module <name>'.\n";
        }
    }

    std::string getHelp() const override {
        return "initialize [--system | --module <name>] - Initializes system components.";
    }
};

class SchedulerTestCommand : public ICommand {
public:
    void execute([[maybe_unused]] const std::vector<std::string>& args) override {
        std::cout << "'scheduler-test' command recognized. Doing something.\n";
        // Add logic here if needed
    }

    std::string getHelp() const override {
        return "scheduler-test - Simulates a test of the process scheduler.";
    }
};

class SchedulerStopCommand : public ICommand {
public:
    void execute([[maybe_unused]] const std::vector<std::string>& args) override {
        std::cout << "'scheduler-stop' command recognized. Doing something.\n";
        // Add logic here if needed
    }

    std::string getHelp() const override {
        return "scheduler-stop - Simulates stopping the scheduler.";
    }
};

class ReportUtilCommand : public ICommand {
public:
    void execute([[maybe_unused]] const std::vector<std::string>& args) override {
        std::cout << "'report-util' command recognized. Doing something.\n";
        // Add logic here if needed
    }

    std::string getHelp() const override {
        return "report-util - Generates a simulated system report.";
    }
};

// --- End of Command Definitions ---


class CommandRegistry {
private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands;

public:
    void registerCommand(const std::string& name, std::unique_ptr<ICommand> command) {
        commands[name] = std::move(command);
    }
    
    bool executeCommand(const std::string& name, const std::vector<std::string>& args) {
        auto it = commands.find(name);
        if (it != commands.end()) {
            it->second->execute(args);
            return true;
        }
        return false;
    }

    std::optional<std::string> getCommandHelp(const std::string& name) const {
        auto it = commands.find(name);
        if (it != commands.end()) {
            return it->second->getHelp();
        }
        return std::nullopt;
    }

    std::vector<std::string> getAvailableCommands() const {
        std::vector<std::string> command_names;
        for (const auto& pair : commands) {
            command_names.push_back(pair.first);
        }
        std::sort(command_names.begin(), command_names.end()); 
        return command_names;
    }
};


class CLI {
private:
    CommandRegistry registry;
    bool running;

public:
    CLI() : running(true) {
        registry.registerCommand("screen", std::make_unique<ScreenCommand>());
        registry.registerCommand("initialize", std::make_unique<InitializeCommand>());
        registry.registerCommand("scheduler-test", std::make_unique<SchedulerTestCommand>());
        registry.registerCommand("scheduler-stop", std::make_unique<SchedulerStopCommand>());
        registry.registerCommand("report-util", std::make_unique<ReportUtilCommand>());
    }

    void run() {
        printHeader();
        
        while (running) {
            std::cout << "Enter a command: ";
            std::string input_line;
            std::getline(std::cin, input_line);

            if (std::cin.eof()){ // Handle Ctrl+D as exit for main CLI
                std::cout << "Exiting due to EOF...\n";
                running = false;
                break;
            }
            
            CommandArgs parsed = CommandParser::parse(input_line);
            
            if (parsed.command.empty()) { 
                continue;
            }

            if (parsed.command == "exit") {
                running = false;
            } else if (parsed.command == "clear") {
                clearScreen();
            } else if (parsed.command == "help") { 
                handleHelp(parsed.args);
            }
            else {
                if (!registry.executeCommand(parsed.command, parsed.args)) {
                    std::cout << "Unknown command: " << parsed.command << "\n";
                    std::cout << "Type 'help' for a list of available commands.\n";
                }
            }
        }
        std::cout << "Shutting down the OS emulator...\n";
        std::cout << "Shut down successfully.";
    }

private:
    void clearScreen() {
        std::system("cls");
        printHeader();
    }

    void handleHelp(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Available commands:\n";
            for (const auto& cmd_name : registry.getAvailableCommands()) {
                std::cout << "  " << cmd_name;
                if (auto help_text_opt = registry.getCommandHelp(cmd_name)) {
                    // Simple formatting for multi-line help
                    std::string help_text = *help_text_opt;
                    std::istringstream iss(help_text);
                    std::string line;
                    bool first_line = true;
                    while(std::getline(iss, line)) {
                        if (first_line) {
                             std::cout << " - " << line << "\n";
                             first_line = false;
                        } else {
                            std::cout << "    " << line << "\n"; // Indent subsequent lines
                        }
                    }
                } else {
                    std::cout << "\n";
                }
            }
            std::cout << "  help [command_name] - Display help for a specific command.\n";
            std::cout << "  clear               - Clears the screen.\n";
            std::cout << "  exit                - Exits the application.\n";
        } else {
            const std::string& cmd_to_help = args[0];
            if (auto help_text_opt = registry.getCommandHelp(cmd_to_help)) {
                 std::cout << *help_text_opt << "\n";
            } else if (cmd_to_help == "exit") {
                std::cout << "exit - Exits the application.\n";
            } else if (cmd_to_help == "clear") {
                std::cout << "clear - Clears the screen and reprints the header.\n";
            } else if (cmd_to_help == "help") {
                std::cout << "help [command_name] - Displays this help message or help for a specific command.\n";
            }
            else {
                std::cout << "No help available for unknown command: " << cmd_to_help << "\n";
            }
        }
    }
};

int main() {
    CLI cli;
    cli.run();
    return 0;
}
