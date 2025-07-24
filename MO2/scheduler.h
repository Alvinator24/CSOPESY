#pragma once
#include <string>
#include <vector>

class Process;

class Scheduler {
private:
    std::string type;
    int quantum;
    std::vector<Process*> ready_queue;
    std::vector<Process*> finished_processes;


public:
    Scheduler(const std::string& scheduler_type, int q);
    void add_process(Process* process);
    Process* get_next_process();
    void return_process(Process* process);
    void display_status(const std::string& status_message);

    const std::string& get_type() const { return type; }
    int get_quantum() const { return quantum; }
    bool has_pending_processes() const { return !ready_queue.empty(); }
};
