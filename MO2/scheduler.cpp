#include "scheduler.h"
#include "process.h"
#include "cpu.h"
#include "config.h"
#include <iostream>
#include <iomanip>
#include<memory>

extern std::unique_ptr<CPU> cpu;
extern std::vector<std::unique_ptr<Process>> processes;

Scheduler::Scheduler(const std::string& scheduler_type, int q) : type(scheduler_type), quantum(q) {}

void Scheduler::add_process(Process* process) {
    ready_queue.push_back(process);
}

Process* Scheduler::get_next_process() {
    if (ready_queue.empty()) {
        return nullptr;
    }

    if (type == "fcfs") {
        Process* process = ready_queue.front();
        ready_queue.erase(ready_queue.begin());
        return process;
    }
    else if (type == "rr") {
        Process* process = ready_queue.front();
        ready_queue.erase(ready_queue.begin());
        return process;
    }

    return nullptr;
}

void Scheduler::return_process(Process* process) {
    if (process->status == "finished" || process->status == "memory_violation") {
        finished_processes.push_back(process);
    } else if (process->status == "blocked") {
        blocked_queue.push_back(process);
    } else {
        ready_queue.push_back(process);
    }
}

void Scheduler::check_blocked_processes(int current_tick) {
    std::vector<Process*> processes_to_unblock;
    
    blocked_queue.erase(std::remove_if(blocked_queue.begin(), blocked_queue.end(),
        [&](Process* p) {
            if (current_tick >= p->sleep_until_tick) {
                p->status = "ready";
                processes_to_unblock.push_back(p);
                return true; // remove from blocked_queue
            }
            return false;
        }), blocked_queue.end());

    for (auto* p : processes_to_unblock) {
        add_process(p);
    }
}


void Scheduler::display_status(const std::string& status_message) {
    std::cout << "\n==========" << status_message << "==========" << std::endl;
    if (cpu) {
        double utilization = (static_cast<double>(cpu->get_utilization()) / cpu->get_num_cores()) * 100;
        std::cout << "CPU Utilization: " << std::fixed << std::setprecision(2) << utilization << "%" << std::endl;
        std::cout << "Cores Used: " << cpu->get_utilization() << std::endl;
        std::cout << "Cores Available: " << cpu->get_num_cores() - cpu->get_utilization() << std::endl;

        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "Running processes:" << std::endl;
        bool no_running = true;
        for (const auto& core : cpu->get_cores()) {
            if (core.current_process) {
                no_running = false;
                Process* p = core.current_process;
                std::cout << p->name << " (" << p->creation_timestamp_str << ") \tCore: \t" << core.id
                    << "\t" << p->current_instruction << " / " << p->total_instructions << std::endl;
            }
        }
        if (no_running) {
            std::cout << "  None" << std::endl;
        }

        std::cout << "\nBlocked processes:" << std::endl;
        if (blocked_queue.empty()) {
            std::cout << "  None" << std::endl;
        } else {
            for (const auto& p : blocked_queue) {
                std::cout << p->name << " (" << p->creation_timestamp_str << ") \tBlocked until tick " << p->sleep_until_tick << std::endl;
            }
        }

        std::cout << "\nFinished processes:" << std::endl;
        if (finished_processes.empty()) {
            std::cout << "  None" << std::endl;
        }
        else {
            for (const auto& p : finished_processes) {
                std::cout << p->name << " (" << p->creation_timestamp_str << ") \t"
                          << std::left << std::setw(10) << p->status << "\t"
                          << p->current_instruction << " / " << p->total_instructions << std::endl;
            }
        }
        std::cout << "--------------------------------------------------" << std::endl;
    }
}
