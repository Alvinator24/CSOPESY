#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include "process.h"

class MMU;

class Scheduler {
public:
    Scheduler(const std::string& scheduler_type, int q);
    void add_process(Process* process);
    Process* get_next_process();
    void return_process(Process* process);
    void check_blocked_processes(int current_tick, MMU* mmu);
    void display_status(const std::string& status_message);
    int get_quantum() const { return quantum; }
    const std::string& get_type() const { return type; }

private:
    std::string type;
    int quantum;
    std::vector<Process*> ready_queue;
    std::vector<Process*> blocked_queue;
    std::vector<Process*> memory_blocked_queue;
    std::vector<Process*> finished_processes;
};

#endif //SCHEDULER_H
