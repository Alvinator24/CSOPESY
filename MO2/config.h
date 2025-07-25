#pragma once
#include <string>
#include <memory>

class Config {
public:
    int num_cpu;
    std::string scheduler_type;
    int quantum_cycles;
    int batch_process_freq;
    int min_instructions;
    int max_instructions;
    int delay_per_exec;
    int max_overall_mem;
    int mem_per_frame;
    int min_mem_per_proc;
    int max_mem_per_proc;

    Config();
};

extern std::unique_ptr<Config> config;