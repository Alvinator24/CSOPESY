#include "process.h"
#include "config.h"
#include "cpu.h"
#include <cstdlib>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>
#include <vector>
#include <string>


extern std::unique_ptr<Config> config;
extern std::unique_ptr<CPU> cpu;

namespace {
    std::string getCurrentTimestamp(bool for_report = false) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_r(&now_c, &now_tm);

        std::stringstream ss;
        const char* format = for_report ? "%m/%d/%Y %I:%M:%S%p" : "(%m/%d/%Y %I:%M:%S%p)";
        ss << std::put_time(&now_tm, format);
        return ss.str();
    }

    // Helper function to parse instruction arguments
    std::vector<std::string> get_args(const std::string& instruction) {
        std::vector<std::string> args;
        size_t start = instruction.find('(');
        size_t end = instruction.find(')');
        if (start == std::string::npos || end == std::string::npos) {
            return args;
        }
        std::string args_str = instruction.substr(start + 1, end - start - 1);
        std::stringstream ss(args_str);
        std::string arg;
        while (std::getline(ss, arg, ',')) {
            arg.erase(0, arg.find_first_not_of(" \t"));
            arg.erase(arg.find_last_not_of(" \t") + 1);
            args.push_back(arg);
        }
        return args;
    }
}

Process::Process(const std::string& n, int p_id) : name(n), id(p_id), priority(0),
status("ready"), current_instruction(0), arrival_time(0),
start_time(-1), completion_time(-1), total_instructions(0) {
    creation_timestamp_str = getCurrentTimestamp(true);
    generate_instructions();
}

void Process::generate_instructions() {
    int num_instructions = rand() % (config->max_instructions - config->min_instructions + 1) + config->min_instructions;
    total_instructions = num_instructions;

    for (int i = 0; i < num_instructions; i++) {
        int instruction_type = rand() % 6;
        switch (instruction_type) {
        case 0: // PRINT
            instructions.push_back("PRINT(\"Process " + name + " message " + std::to_string(i) + "\")");
            break;
        case 1: // DECLARE
            instructions.push_back("DECLARE(var" + std::to_string(i) + ", " + std::to_string(rand() % 100) + ")");
            break;
        case 2: // ADD
            if (rand() % 2 == 0) {
                instructions.push_back("ADD(var" + std::to_string(i) + ", var" + std::to_string(i > 0 ? i - 1 : 0) + ", " + std::to_string(rand() % 100) + ")");
            } else {
                instructions.push_back("ADD(var" + std::to_string(i) + ", var" + std::to_string(i > 0 ? i - 1 : 0) + ", var" + std::to_string(i + 1) + ")");
            }
            break;
        case 3: // SUBTRACT
            if (rand() % 2 == 0) {
                instructions.push_back("SUBTRACT(var" + std::to_string(i) + ", var" + std::to_string(i) + ", " + std::to_string(rand() % 50) + ")");
            } else {
                instructions.push_back("SUBTRACT(var" + std::to_string(i) + ", var" + std::to_string(i) + ", var" + std::to_string(i > 0 ? i - 1 : 0) + ")");
            }
            break;
        case 4: // SLEEP
            instructions.push_back("SLEEP(" + std::to_string(rand() % 5 + 1) + ")");
            break;
        case 5: // FOR loop
            instructions.push_back("FOR(PRINT(\"Loop iteration\"), " + std::to_string(rand() % 3 + 2) + ")");
            break;
        }
    }
}

void Process::execute_instruction(int core_id) {
    if (current_instruction >= instructions.size()) {
        status = "finished";
        completion_time = cpu->get_current_time();
        return;
    }

    if (cpu->get_current_time() < sleep_until) {
        status = "blocked";
        return; // Process is sleeping
    } else {
        status = "running";
    }

    std::string instruction = instructions[current_instruction];
    std::vector<std::string> args = get_args(instruction);

    if (instruction.substr(0, 5) == "PRINT") {
        if (!args.empty()) {
            std::string message = args[0];
            message.erase(0, message.find_first_not_of("\""));
            message.erase(message.find_last_not_of("\"") + 1);
            LogEntry log;
            log.timestamp = getCurrentTimestamp();
            log.core_id = core_id;
            log.message = message;
            output_log.push_back(log);
        }
    }
    else if (instruction.substr(0, 7) == "DECLARE") {
        if (args.size() == 2) {
            variables[args[0]] = std::stoi(args[1]);
        }
    }
    else if (instruction.substr(0, 3) == "ADD") {
        if (args.size() == 3) {
            int val2 = isdigit(args[1][0]) ? std::stoi(args[1]) : variables[args[1]];
            int val3 = isdigit(args[2][0]) ? std::stoi(args[2]) : variables[args[2]];
            variables[args[0]] = val2 + val3;
        }
    }
    else if (instruction.substr(0, 8) == "SUBTRACT") {
        if (args.size() == 3) {
            int val2 = isdigit(args[1][0]) ? std::stoi(args[1]) : variables[args[1]];
            int val3 = isdigit(args[2][0]) ? std::stoi(args[2]) : variables[args[2]];
            variables[args[0]] = val2 - val3;
        }
    }
    else if (instruction.substr(0, 5) == "SLEEP") {
        if (!args.empty()) {
            int sleep_ticks = std::stoi(args[0]);
            sleep_until = cpu->get_current_time() + sleep_ticks;
            status = "blocked";
        }
    }
    else if (instruction.substr(0, 3) == "FOR") {
        if (for_loop_counter == 0) {
            for_loop_start_instruction = current_instruction;
            for_loop_counter = std::stoi(args[1]);
        }
        
        if (for_loop_counter > 0) {
            // Execute the instruction inside the for loop
            std::string inner_instruction = args[0];
            // This is a simplified execution of the inner instruction.
            // A more robust implementation would require a proper parser.
            if (inner_instruction.substr(0, 5) == "PRINT") {
                std::string message = inner_instruction.substr(inner_instruction.find('"') + 1, inner_instruction.rfind('"') - inner_instruction.find('"') - 1);
                LogEntry log;
                log.timestamp = getCurrentTimestamp();
                log.core_id = core_id;
                log.message = message;
                output_log.push_back(log);
            }
            for_loop_counter--;
        }

        if(for_loop_counter == 0) {
            // Loop finished, move to instruction after FOR
            current_instruction++;
        }
    }

    if (instruction.substr(0, 3) != "FOR") {
        current_instruction++;
    }

    if (current_instruction >= instructions.size()) {
        status = "finished";
        completion_time = cpu->get_current_time();
    }
}

void Process::display_status() {
    std::cout << "\nProcess Name: " << name << std::endl;
    std::cout << "ID: " << id << std::endl;
    std::cout << "Logs:" << std::endl;
    for (const auto& log : output_log) {
        std::cout << log.timestamp << " Core:" << log.core_id << " \"" << log.message << "\"" << std::endl;
    }

    if (status != "finished") {
        std::cout << "\nCurrent instruction line: " << current_instruction << std::endl;
        std::cout << "Lines of code: " << total_instructions << "\n" << std::endl;
    }
    else {
        std::cout << "Finished!" << std::endl;
    }
}

Process::~Process() {
    std::cout << "[DEBUG] Destroying process: " << name << std::endl;
}