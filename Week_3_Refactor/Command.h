#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <iostream>

class ICommand {
public:
    virtual void execute(const std::vector<std::string>& args) = 0;
    virtual ~ICommand() = default;
};

class ClearCommand : public ICommand {
public:
    void execute(const std::vector<std::string>& args) override {
        system("clear");
    }
};

class HelpCommand : public ICommand {
public:
    void execute(const std::vector<std::string>& args) override {
        std::cout << "Available commands: clear, help, exit\n";
    }
};

class ScreenCommand : public ICommand {
public:
    void execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cout << "screen command usage:\n"
                      << "  screen -ls           # list screens\n"
                      << "  screen -s <name>     # create screen\n";
            return;
        }

        if (args[0] == "-ls") {
            // add logic here to list all active screens
        } 
        else if (args[0] == "-s") {
            if (args.size() < 2) {
                std::cout << "Error: screen name required after -s\n";
            } else {
                const std::string& screenName = args[1];
                std::cout << "Switching to screen: " << screenName << "\n";
                // add logic here to create the screen
            }
        }
        else {
            std::cout << "Unknown option for screen command\n";
        }
    }
};

#endif