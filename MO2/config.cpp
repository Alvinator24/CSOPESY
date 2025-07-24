#include "config.h"

Config::Config() : num_cpu(1), scheduler_type("fcfs"), quantum_cycles(10),
batch_process_freq(5), min_instructions(5), max_instructions(15),
delay_per_exec(100) {}