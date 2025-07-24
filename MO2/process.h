#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdint>

#include "mmu.h"

struct LogEntry {
    std::string timestamp;
    int core_id;
    std::string message;
};

class Process {
public:
    std::string name;
    int id;
    int priority;
    std::string status; // "ready", "running", "blocked", "finished", "memory_violation"
    int current_instruction;
    std::vector<std::string> instructions;
    std::vector<LogEntry> output_log;
    std::map<std::string, uint16_t> variables;
    int arrival_time;
    int start_time;
    int completion_time;
    int total_instructions;
    std::string creation_timestamp_str;
    int memory_size;
    MMU* mmu;
    uint32_t faulting_address;
    int sleep_until_tick;

    Process(const std::string& n, int p_id, int mem_size, MMU* mmu_ptr);
    void generate_instructions(int min_instr, int max_instr);
    void execute_instruction(int core_id);
    void display_status();
    ~Process();
};
