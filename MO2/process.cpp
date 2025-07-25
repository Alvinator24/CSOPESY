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

Process::Process(const std::string& n, int p_id, int mem_size, MMU* mmu_ptr) 
    : name(n), id(p_id), priority(0),
      status("ready"), current_instruction(0), arrival_time(0),
      start_time(-1), completion_time(-1), total_instructions(0),
      memory_size(mem_size), mmu(mmu_ptr), faulting_address(0), sleep_until_tick(0) {
    creation_timestamp_str = getCurrentTimestamp(true);
    if (mmu) {
        mmu->allocate_memory(id, memory_size);
    }
}

void Process::generate_instructions(int min_instr, int max_instr) {
    int num_instructions = rand() % (max_instr - min_instr + 1) + min_instr;
    total_instructions = num_instructions;

    for (int i = 0; i < num_instructions; i++) {
        int instruction_type = rand() % 8; // Increased for READ/WRITE
        switch (instruction_type) {
        case 0: // PRINT
            instructions.push_back("PRINT(\"Process " + name + " message " + std::to_string(i) + "\")");
            break;
        case 1: // DECLARE
            if (variables.size() < 32) {
                instructions.push_back("DECLARE(var" + std::to_string(i) + ", " + std::to_string(rand() % 65535) + ")");
            } else {
                i--; // Try again
            }
            break;
        case 2: // ADD
            instructions.push_back("ADD(var" + std::to_string(i) + ", var" + std::to_string(i > 0 ? i - 1 : 0) + ", var" + std::to_string(i > 1 ? i - 2 : 0) + ")");
            break;
        case 3: // SUBTRACT
            instructions.push_back("SUBTRACT(var" + std::to_string(i) + ", var" + std::to_string(i > 0 ? i - 1 : 0) + ", var" + std::to_string(i > 1 ? i - 2 : 0) + ")");
            break;
        case 4: // SLEEP
            instructions.push_back("SLEEP(" + std::to_string(rand() % 5 + 1) + ")");
            break;
        case 5: // FOR loop
            instructions.push_back("FOR([PRINT(\"Loop iteration\")], " + std::to_string(rand() % 3 + 1) + ")");
            break;
        case 6: // READ
        {
            std::stringstream ss;
            ss << "0x" << std::hex << (rand() % (memory_size > 0 ? memory_size : 256));
            instructions.push_back("READ(var" + std::to_string(i) + ", " + ss.str() + ")");
            break;
        }
        case 7: // WRITE
        {
            std::stringstream ss;
            ss << "0x" << std::hex << (rand() % (memory_size > 0 ? memory_size : 256));
            instructions.push_back("WRITE(" + ss.str() + ", " + std::to_string(rand() % 65535) + ")");
            break;
        }
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
            var_name.erase(0, var_name.find_first_not_of(" \t"));
            var_name.erase(var_name.find_last_not_of(" \t") + 1);
            
            uint16_t value = std::stoi(instruction.substr(comma + 1, end - comma - 1));
            if (variables.size() < 32) {
                variables[var_name] = value;
            } else {
                // Optional: Log an error that symbol table is full
            }
        }
    }
    else if (instruction.substr(0, 3) == "ADD") {
        size_t start = instruction.find('(');
        size_t comma1 = instruction.find(',');
        size_t comma2 = instruction.find(',', comma1 + 1);
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma1 != std::string::npos && comma2 != std::string::npos && end != std::string::npos) {
            std::string dest_var = instruction.substr(start + 1, comma1 - start - 1);
            std::string src_var1 = instruction.substr(comma1 + 1, comma2 - comma1 - 1);
            std::string src_var2 = instruction.substr(comma2 + 1, end - comma2 - 1);
            
            // trim whitespace
            dest_var.erase(0, dest_var.find_first_not_of(" \t"));
            dest_var.erase(dest_var.find_last_not_of(" \t") + 1);
            src_var1.erase(0, src_var1.find_first_not_of(" \t"));
            src_var1.erase(src_var1.find_last_not_of(" \t") + 1);
            src_var2.erase(0, src_var2.find_first_not_of(" \t"));
            src_var2.erase(src_var2.find_last_not_of(" \t") + 1);

            variables[dest_var] = variables[src_var1] + variables[src_var2];
        }
    }
    else if (instruction.substr(0, 8) == "SUBTRACT") {
        size_t start = instruction.find('(');
        size_t comma1 = instruction.find(',');
        size_t comma2 = instruction.find(',', comma1 + 1);
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma1 != std::string::npos && comma2 != std::string::npos && end != std::string::npos) {
            std::string dest_var = instruction.substr(start + 1, comma1 - start - 1);
            std::string src_var1 = instruction.substr(comma1 + 1, comma2 - comma1 - 1);
            std::string src_var2 = instruction.substr(comma2 + 1, end - comma2 - 1);

            // trim whitespace
            dest_var.erase(0, dest_var.find_first_not_of(" \t"));
            dest_var.erase(dest_var.find_last_not_of(" \t") + 1);
            src_var1.erase(0, src_var1.find_first_not_of(" \t"));
            src_var1.erase(src_var1.find_last_not_of(" \t") + 1);
            src_var2.erase(0, src_var2.find_first_not_of(" \t"));
            src_var2.erase(src_var2.find_last_not_of(" \t") + 1);

            variables[dest_var] = variables[src_var1] - variables[src_var2];
        }
    }
    else if (instruction.substr(0, 4) == "READ") {
        size_t start = instruction.find('(');
        size_t comma = instruction.find(',');
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma != std::string::npos && end != std::string::npos) {
            std::string var_name = instruction.substr(start + 1, comma - start - 1);
            var_name.erase(0, var_name.find_first_not_of(" \t"));
            var_name.erase(var_name.find_last_not_of(" \t") + 1);
            
            std::string address_str = instruction.substr(comma + 1, end - comma - 1);
            address_str.erase(0, address_str.find_first_not_of(" \t"));
            address_str.erase(address_str.find_last_not_of(" \t") + 1);
            
            uint32_t address = 0;
            try {
                address = std::stoul(address_str, nullptr, 16);
                if (address >= static_cast<uint32_t>(memory_size)) throw std::out_of_range("Address out of bounds");
                variables[var_name] = mmu->read(this, address);
            } catch (const std::out_of_range&) {
                status = "memory_violation";
                faulting_address = address;
                completion_time = cpu->get_current_time();
                return;
            }
        }
    }
    else if (instruction.substr(0, 5) == "WRITE") {
        size_t start = instruction.find('(');
        size_t comma = instruction.find(',');
        size_t end = instruction.find(')');
        if (start != std::string::npos && comma != std::string::npos && end != std::string::npos) {
            std::string address_str = instruction.substr(start + 1, comma - start - 1);
            address_str.erase(0, address_str.find_first_not_of(" \t"));
            address_str.erase(address_str.find_last_not_of(" \t") + 1);

            std::string value_str = instruction.substr(comma + 1, end - comma - 1);
            value_str.erase(0, value_str.find_first_not_of(" \t"));
            value_str.erase(value_str.find_last_not_of(" \t") + 1);

            uint32_t address = 0;
            try {
                address = std::stoul(address_str, nullptr, 16);
                if (address >= static_cast<uint32_t>(memory_size)) throw std::out_of_range("Address out of bounds");

                uint16_t value;
                if (variables.count(value_str)) {
                    value = variables[value_str];
                } else {
                    value = std::stoi(value_str);
                }
                mmu->write(this, address, value);
            } catch (const std::out_of_range&) {
                status = "memory_violation";
                faulting_address = address;
                completion_time = cpu->get_current_time();
                return;
            }
        }
    }
    else if (instruction.substr(0, 5) == "SLEEP") {
        size_t start = instruction.find('(');
        size_t end = instruction.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            int sleep_ticks = std::stoi(instruction.substr(start + 1, end - start - 1));
            sleep_until_tick = cpu->get_current_time() + sleep_ticks;
            status = "blocked";
        }
    }
    else if (instruction.substr(0, 3) == "FOR") {
        size_t start_bracket = instruction.find('[');
        size_t end_bracket = instruction.rfind(']');
        size_t comma = instruction.rfind(',');

        if (start_bracket != std::string::npos && end_bracket != std::string::npos && comma != std::string::npos && comma > end_bracket) {
            if (loop_stack.size() >= 3) {
                // As per spec, ignore loops nested deeper than 3 levels
                current_instruction++;
                return;
            }

            std::string loop_body_str = instruction.substr(start_bracket + 1, end_bracket - start_bracket - 1);
            std::string repetitions_str = instruction.substr(comma + 1);
            repetitions_str.erase(0, repetitions_str.find_first_not_of(" \t"));
            repetitions_str.erase(repetitions_str.find_last_not_of(" \t") + 1);
            
            try {
                int repetitions = std::stoi(repetitions_str);
                
                LoopContext new_loop;
                new_loop.loop_counter = repetitions;
                new_loop.loop_start_instruction = current_instruction;

                std::stringstream ss(loop_body_str);
                std::string item;
                while (std::getline(ss, item, ';')) {
                    item.erase(0, item.find_first_not_of(" \t\n\r"));
                    item.erase(item.find_last_not_of(" \t\n\r") + 1);
                    if (!item.empty()) {
                        new_loop.loop_body.push_back(item);
                    }
                }
                new_loop.loop_body.push_back("END"); // Sentinel to mark end of loop iteration

                // Replace the FOR instruction with the body
                instructions.erase(instructions.begin() + current_instruction);
                instructions.insert(instructions.begin() + current_instruction, new_loop.loop_body.begin(), new_loop.loop_body.end());
                
                new_loop.loop_counter--; // Decrement for the first run
                loop_stack.push(new_loop);

            } catch (const std::invalid_argument& e) {
                // Could not parse repetitions, skip instruction
                current_instruction++;
            }
        } else {
            // Malformed FOR, skip
            current_instruction++;
        }
        // Do not increment current_instruction here, we want to execute the first instruction of the loop body
        return; 
    } else if (instruction == "END") {
        if (!loop_stack.empty()) {
            LoopContext& current_loop = loop_stack.top();
            if (current_loop.loop_counter > 0) {
                current_loop.loop_counter--;
                current_instruction = current_loop.loop_start_instruction; // Jump to start of loop
            } else {
                loop_stack.pop(); // Loop finished
                current_instruction++;
            }
        } else {
            // END outside of a loop, just skip
            current_instruction++;
        }
        return;
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

    if (status != "finished" && status != "memory_violation") {
        std::cout << "\nCurrent instruction line: " << current_instruction << std::endl;
        std::cout << "Lines of code: " << total_instructions << "\\n" << std::endl;
    }
    else if (status == "memory_violation") {
        std::cout << "Finished due to memory access violation." << std::endl;
        std::cout << "Faulting address: 0x" << std::hex << faulting_address << std::dec << std::endl;
    }
    else {
        std::cout << "Finished!" << std::endl;
    }
}

Process::~Process() {
    if (mmu) {
        mmu->free_memory(id);
    }
    std::cout << "[DEBUG] Destroying process: " << name << std::endl;
}
