#pragma once
#include <vector>
#include <string>
#include "process.h"
#include <memory>

class Process;

struct Core {
    int id;
    Process* current_process;
    int time_slice_remaining;
    Core() : id(-1), current_process(nullptr), time_slice_remaining(0) {}
};

class CPU {
private:
    int current_time;
    std::vector<Core> cores;
    std::vector<Process*> running_processes;
    int num_cores;
    long long total_busy_time = 0;

public:
    CPU(int cores);
    int get_current_time() const;
    int get_num_cores() const;
    int get_utilization() const;
    const std::vector<Core>& get_cores() const { return cores; }
    void tick();
    bool can_run_process();
    void add_process(Process* process);
    void remove_process(Process* process);
    void execute_processes();
    void clear_processes();

private:
};
