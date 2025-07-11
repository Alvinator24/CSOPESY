#pragma once
#include <string>
#include <vector>
#include <mutex>

class Process;

class Scheduler {
private:
    std::string type;
    int quantum;
    std::vector<Process*> ready_queue;
    std::vector<Process*> finished_processes;
    std::vector<Process*> blocked_queue;
    std::mutex scheduler_mutex;


public:
    Scheduler(const std::string& scheduler_type, int q);
    void add_process(Process* process);
    Process* get_next_process();
    void return_process(Process* process);
    void display_status(const std::string& status_message);
    void check_blocked_processes();

    const std::string& get_type() const { return type; }
    int get_quantum() const { return quantum; }
    bool has_pending_processes() const;
};
