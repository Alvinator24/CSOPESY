#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>

struct LogEntry {
    std::string timestamp;
    int core_id;
    std::string message;
};

class Process {
public:
    std::string name;
    int id;
    int priority;
    std::string status; // "ready", "running", "blocked", "finished"
    int current_instruction;
    std::vector<std::string> instructions;
    std::vector<LogEntry> output_log;
    std::map<std::string, int> variables;
    int arrival_time;
    int start_time;
    int completion_time;
    int total_instructions;
    std::string creation_timestamp_str;

    Process(const std::string& n, int p_id);
    void generate_instructions();
    void execute_instruction(int core_id);
    void display_status();
    ~Process();
};
