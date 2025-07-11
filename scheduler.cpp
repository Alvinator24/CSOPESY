#include "scheduler.h"
#include "process.h"
#include "cpu.h"
#include "config.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>

extern std::unique_ptr<CPU> cpu;
extern std::vector<std::unique_ptr<Process>> processes;

Scheduler::Scheduler(const std::string& scheduler_type, int q) : type(scheduler_type), quantum(q) {}

void Scheduler::add_process(Process* process) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    ready_queue.push_back(process);
}

Process* Scheduler::get_next_process() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    if (ready_queue.empty()) {
        return nullptr;
    }

    Process* process = ready_queue.front();
    ready_queue.erase(ready_queue.begin());
    return process;
}

void Scheduler::return_process(Process* process) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    if (process->status == "finished") {
        finished_processes.push_back(process);
    } else if (process->status == "blocked") {
        blocked_queue.push_back(process);
    } else {
        ready_queue.push_back(process);
    }
}

void Scheduler::check_blocked_processes() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    std::vector<Process*> still_blocked;
    for (Process* process : blocked_queue) {
        if (cpu->get_current_time() >= process->sleep_until) {
            process->status = "ready";
            ready_queue.push_back(process);
        } else {
            still_blocked.push_back(process);
        }
    }
    blocked_queue = still_blocked;
}

void Scheduler::display_status(const std::string& status_message) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
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

        std::cout << "\nFinished processes:" << std::endl;
        if (finished_processes.empty()) {
            std::cout << "  None" << std::endl;
        }
        else {
            for (const auto& p : finished_processes) {
                std::cout << p->name << " (" << p->creation_timestamp_str << ") \tFinished \t"
                    << p->total_instructions << " / " << p->total_instructions << std::endl;
            }
        }
        std::cout << "--------------------------------------------------" << std::endl;
    }
}

bool Scheduler::has_pending_processes() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(scheduler_mutex));
    return !ready_queue.empty() || !blocked_queue.empty();
}