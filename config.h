#pragma once
#include <string>

class Config {
public:
    int num_cpu;
    std::string scheduler_type;
    int quantum_cycles;
    int batch_process_freq;
    int min_instructions;
    int max_instructions;
    int delay_per_exec;

    Config();
};