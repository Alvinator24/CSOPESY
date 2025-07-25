#include "cpu.h"
#include "process.h"
#include "config.h"
#include "scheduler.h"
#include "mmu.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <iostream>

extern std::unique_ptr<Config> config;
extern std::unique_ptr<Scheduler> scheduler;
extern std::unique_ptr<MMU> mmu;

CPU::CPU(int n_cores, MMU* mmu_ptr) : num_cores(n_cores), mmu(mmu_ptr), current_time(0), total_busy_time(0) {
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
    int busy_cores = 0;
    for (const auto& core : cores) {
        if (core.current_process != nullptr) {
            busy_cores++;
        }
    }
    return busy_cores;
}

void CPU::tick() {
    current_time++;
    for (const auto& core : cores) {
        if (core.current_process) {
            total_busy_time++;
        }
    }
    // Simulate instruction execution delay
    std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_per_exec));
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
    for (auto& core : cores) {
        if (core.current_process == process) {
            core.current_process = nullptr;
            break;
        }
    }
    running_processes.erase(std::remove(running_processes.begin(), running_processes.end(), process), running_processes.end());
}

void CPU::execute_processes() {
    for (auto& core : cores) {
        if (core.current_process && core.current_process->status == "running") {
            int instructions_to_execute = (scheduler->get_type() == "rr") ? core.time_slice_remaining : 1;

            for (int i = 0; i < instructions_to_execute && core.current_process->status == "running"; ++i) {
                try {
                    core.current_process->execute_instruction(core.id);
                } catch (const PageFaultException& e) {
                    mmu->handle_page_fault(core.current_process, e.address);
                    // The instruction will be retried on the next tick because we break the loop.
                    // To execute multiple instructions per tick, we'd need to adjust this.
                    // For now, let's stick to one attempt per tick to handle page faults simply.
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "Process " << core.current_process->name << " encountered a runtime error: " << e.what() << std::endl;
                    core.current_process->status = "memory_violation";
                    break;
                }
            }

            if (scheduler->get_type() == "rr") {
                core.time_slice_remaining--;
            }

            if (core.current_process->status != "running" || (scheduler->get_type() == "rr" && core.time_slice_remaining <= 0)) {
                scheduler->return_process(core.current_process);
                core.current_process = nullptr;
            }
        }
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
