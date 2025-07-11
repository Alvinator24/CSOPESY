#include "cpu.h"
#include "process.h"
#include "config.h"
#include "scheduler.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <iostream>

extern std::unique_ptr<Config> config;
extern std::unique_ptr<Scheduler> scheduler;

CPU::CPU(int n_cores) : num_cores(n_cores), current_time(0), total_busy_time(0) {
    cores.resize(n_cores);
    for (int i = 0; i < n_cores; ++i) {
        cores[i].id = i;
    }
    std::cout << "[DEBUG] CPU initialized with " << n_cores << " cores." << std::endl;
}

int CPU::get_current_time() const {
    return current_time;
}

int CPU::get_num_cores() const {
    return num_cores;
}

int CPU::get_utilization() const {
    return static_cast<int>(running_processes.size());
}

void CPU::tick() {
    current_time++;
    for (const auto& core : cores) {
        if (core.current_process) {
            total_busy_time++;
        }
    }
    // Simulate instruction execution delay
    // std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_per_exec));
}

bool CPU::can_run_process() {
    for (const auto& core : cores) {
        if (core.current_process == nullptr) {
            return true;
        }
    }
    return false;
}

void CPU::add_process(Process* process) {
    std::lock_guard<std::mutex> lock(cpu_mutex);
    for (auto& core : cores) {
        if (core.current_process == nullptr) { //check for free core
            core.current_process = process;
            process->status = "running";

            if (process->start_time == -1) {
                process->start_time = current_time;
            }

            // time quantum for Round-Robin
            if (config->scheduler_type == "rr") {
                core.time_slice_remaining = scheduler->get_quantum();
            }

            running_processes.push_back(process);
            break;
        }
    }
}


void CPU::remove_process(Process* process) {
    std::lock_guard<std::mutex> lock(cpu_mutex);
    for (auto& core : cores) {
        if (core.current_process == process) {
            core.current_process = nullptr;
            break;
        }
    }
    running_processes.erase(std::remove(running_processes.begin(), running_processes.end(), process), running_processes.end());
}

void CPU::execute_processes() {
    std::lock_guard<std::mutex> lock(cpu_mutex);
    std::vector<Process*> to_return_to_scheduler;

    for (int i = 0; i < cores.size(); ++i) {
        if (cores[i].current_process && cores[i].current_process->status == "running") {
            Process* process = cores[i].current_process;
            process->execute_instruction(i);

            if (process->status == "finished") {
                to_return_to_scheduler.push_back(process);
                continue;
            }

            if (scheduler->get_type() == "rr") {
                cores[i].time_slice_remaining--;

                if (cores[i].time_slice_remaining <= 0) {
                    process->status = "ready";
                    to_return_to_scheduler.push_back(process);
                }
            }
        }
    }

    for (auto process : to_return_to_scheduler) {
        remove_process(process);
        scheduler->return_process(process);
    }
}


void CPU::clear_processes() {
    for (auto& core : cores) {
        if (core.current_process) {
            core.current_process->status = "ready";
            core.current_process = nullptr;
        }
    }
    running_processes.clear();
    std::cout << "[DEBUG] CPU processes cleared." << std::endl;
}
