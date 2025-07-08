#include "process.h"
#include "config.h"
#include "cpu.h"
#include <cstdlib>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

extern std::unique_ptr<Config> config;
extern std::unique_ptr<CPU> cpu;

namespace {
    std::string getCurrentTimestamp(bool for_report = false) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c);

        std::stringstream ss;
        const char* format = for_report ? "%m/%d/%Y %I:%M:%S%p" : "(%m/%d/%Y %I:%M:%S%p)";
        ss << std::put_time(&now_tm, format);
        return ss.str();
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
            instructions.push_back("ADD(var" + std::to_string(i) + ", var" + std::to_string(i - 1 >= 0 ? i - 1 : 0) + ", var" + std::to_string(i + 1) + ")");
            break;
        case 3: // SUBTRACT
            instructions.push_back("SUBTRACT(var" + std::to_string(i) + ", var" + std::to_string(i) + ", var" + std::to_string(i - 1 >= 0 ? i - 1 : 0) + ")");
            break;
        case 4: // SLEEP
            instructions.push_back("SLEEP(" + std::to_string(rand() % 5 + 1) + ")");
            break;
        case 5: // FOR loop
            instructions.push_back("FOR([PRINT(\"Loop iteration\")], " + std::to_string(rand() % 3 + 1) + ")");
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

    std::string instruction = instructions[current_instruction];

    if (instruction.substr(0, 5) == "PRINT") {
        size_t start = instruction.find('"');
        size_t end = instruction.rfind('"');
        if (start != std::string::npos && end != std::string::npos) {
            std::string message = instruction.substr(start + 1, end - start - 1);
            LogEntry log;
            log.timestamp = getCurrentTimestamp();
            log.core_id = core_id;
            log.message = message;
            output_log.push_back(log);
        }
    }
    else if (instruction.substr(0, 7) == "DECLARE") {
        size_t start = instruction.find('(');
        size_t comma = instruction.find(',');
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma != std::string::npos && end != std::string::npos) {
            std::string var_name = instruction.substr(start + 1, comma - start - 1);
            int value = std::stoi(instruction.substr(comma + 1, end - comma - 1));
            variables[var_name] = value;
        }
    }
    else if (instruction.substr(0, 3) == "ADD") {
        size_t start = instruction.find('(');
        size_t comma1 = instruction.find(',');
        size_t comma2 = instruction.find(',', comma1 + 1);
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma1 != std::string::npos && comma2 != std::string::npos && end != std::string::npos) {
            std::string var1 = instruction.substr(start + 1, comma1 - start - 1);
            std::string var2 = instruction.substr(comma1 + 1, comma2 - comma1 - 1);
            std::string var3 = instruction.substr(comma2 + 1, end - comma2 - 1);
            variables[var1] = variables[var2] + variables[var3];
        }
    }
    else if (instruction.substr(0, 8) == "SUBTRACT") {
        size_t start = instruction.find('(');
        size_t comma1 = instruction.find(',');
        size_t comma2 = instruction.find(',', comma1 + 1);
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma1 != std::string::npos && comma2 != std::string::npos && end != std::string::npos) {
            std::string var1 = instruction.substr(start + 1, comma1 - start - 1);
            std::string var2 = instruction.substr(comma1 + 1, comma2 - comma1 - 1);
            std::string var3 = instruction.substr(comma2 + 1, end - comma2 - 1);
            variables[var1] = variables[var2] - variables[var3];
        }
    }
    else if (instruction.substr(0, 5) == "SLEEP") {
        size_t start = instruction.find('(');
        size_t end = instruction.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            int sleep_ticks = std::stoi(instruction.substr(start + 1, end - start - 1));
        }
    }
    else if (instruction.substr(0, 3) == "FOR") {
    }

    current_instruction++;
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
