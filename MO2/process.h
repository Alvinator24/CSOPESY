#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdint>
#include <stack>
#include <ctime>

#include "mmu.h"

class MMU; 

struct LogEntry {
    std::string timestamp;
    int core_id;
    std::string message;
};

struct LoopContext {
    int loop_counter;
    int loop_start_instruction;
    std::vector<std::string> loop_body;
};

struct Process {
    int pid;
    std::string name;
    std::string status; 
    int total_instructions;
    int current_instruction;
    std::vector<std::string> instructions;
    std::vector<LogEntry> output_log;
    std::map<std::string, uint16_t> variables;
    
    int arrival_time;
    int start_time;
    int completion_time;
    
    std::time_t creation_timestamp;
    std::string creation_timestamp_str;
    
    int memory_size;
    MMU* mmu;
    uint32_t faulting_address;
    int sleep_until_tick;
    bool needs_memory_check = false;
    int mem_required = 0;

    std::stack<LoopContext> loop_stack;
    std::vector<PageTableEntry> page_table;

    Process(const std::string& n, int p_id, int mem_size, MMU* mmu_ptr);
    void generate_instructions(int min_instr, int max_instr);
    void execute_instruction(int core_id);
    void display_status();
    ~Process();
};
