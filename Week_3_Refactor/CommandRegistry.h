#include <unordered_map>
#include <memory>
#include <iostream>
#include "Command.h"
#include <sstream>

class CommandDispatcher {
    std::unordered_map<std::string, std::shared_ptr<ICommand>> commandMap;

public:
    void registerCommand(const std::string& name, std::shared_ptr<ICommand> command) {
        commandMap[name] = command;
    }

    void dispatch(const std::string& inputLine) {
        std::istringstream iss(inputLine);
        std::string cmd;
        std::vector<std::string> args;
        iss >> cmd;

        std::string arg;
        while (iss >> arg) args.push_back(arg);

        auto it = commandMap.find(cmd);
        if (it != commandMap.end()) {
            it->second->execute(args);
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }
};