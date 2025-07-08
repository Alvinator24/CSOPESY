#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <atomic>
#include "process.h"
#include "cpu.h"
#include "scheduler.h"
#include "config.h"

// Global variables
std::unique_ptr<CPU> cpu;
std::unique_ptr<Scheduler> scheduler;
std::unique_ptr<Config> config;
bool system_initialized = false;
bool scheduler_running = false;
std::vector<std::unique_ptr<Process>> processes;
int next_process_id = 1;

// Function declarations
void initialize_system();
void scheduler_start();
void scheduler_stop();
void screen_ls();
void screen_s(const std::string& process_name);
void process_smi(const std::string& process_name);
void report_util();
void process_command(const std::string& command);
void create_process(const std::string& name);
void attach_to_screen(Process* target_process);

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    std::cout << "  _____  _____  ____  _____  ______  _______     __\n";
    std::cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\   / /\n";
    std::cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\_/ / \n";
    std::cout << "| |     \\___ \\| |  | |  ___/|  __|  \\___ \\  \\   /  \n";
    std::cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   \n";
    std::cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   \n";
    std::cout << "Welcome to the Command-Line Operating System!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen, 'help' for commands.\n";

    std::string command;
    while (true) {
        std::cout << "root:\\> ";
        std::getline(std::cin, command);

        if (command == "exit") {
            std::cout << "Exiting CSOPESY Emulator..." << std::endl;
            break;
        }

        process_command(command);
    }

    return 0;
}

void initialize_system() {
    std::ifstream config_file("config.txt");
    if (!config_file.is_open()) {
        std::cout << "Error: config.txt not found. Creating default configuration..." << std::endl;
        config = std::make_unique<Config>();

        // Create default config file
        std::ofstream default_config("config.txt");
        default_config << "num-cpu: 2\n";
        default_config << "scheduler: fcfs\n";
        default_config << "quantum-cycles: 10\n";
        default_config << "batch-process-freq: 5\n";
        default_config << "min-ins: 5\n";
        default_config << "max-ins: 15\n";
        default_config << "delay-per-exec: 100\n";
        default_config.close();
    }
    else {
        config = std::make_unique<Config>();
        std::string line;
        while (std::getline(config_file, line)) {
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, ':') && std::getline(iss, value)) {
                // Remove leading/trailing whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "num-cpu") config->num_cpu = std::stoi(value);
                else if (key == "scheduler") config->scheduler_type = value;
                else if (key == "quantum-cycles") config->quantum_cycles = std::stoi(value);
                else if (key == "batch-process-freq") config->batch_process_freq = std::stoi(value);
                else if (key == "min-ins") config->min_instructions = std::stoi(value);
                else if (key == "max-ins") config->max_instructions = std::stoi(value);
                else if (key == "delay-per-exec") config->delay_per_exec = std::stoi(value);
            }
        }
        config_file.close();
    }

    cpu = std::make_unique<CPU>(config->num_cpu);
    scheduler = std::make_unique<Scheduler>(config->scheduler_type, config->quantum_cycles);
    system_initialized = true;

    std::cout << "System initialized successfully!" << std::endl;
    std::cout << "num-cpu: " << config->num_cpu << std::endl;
    std::cout << "scheduler: " << config->scheduler_type << std::endl;
    std::cout << "quantum-cycles: " << config->quantum_cycles << std::endl;
    std::cout << "batch-process-freq: " << config->batch_process_freq << std::endl;
    std::cout << "min-ins: " << config->min_instructions << std::endl;
    std::cout << "max-ins: " << config->max_instructions << std::endl;
    std::cout << "delay-per-exec: " << config->delay_per_exec << std::endl;
}

void scheduler_start() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }
    if (!config || !cpu || !scheduler) {
        std::cout << "Error: System components not properly initialized." << std::endl;
        return;
    }

    static std::atomic<bool> simulation_active(false);

    if (simulation_active.load()) {
        std::cout << "Process generation has been re-enabled." << std::endl;
        scheduler_running = true;
        return;
    }

    scheduler_running = true;
    std::cout << "Scheduler started. Processes will be generated and executed." << std::endl;

    std::thread([&]() {
        simulation_active = true;
        int tick_count = 0;

        while (scheduler_running || scheduler->has_pending_processes() || cpu->get_utilization() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_per_exec));
            tick_count++;

            if (scheduler_running && (tick_count % config->batch_process_freq == 0)) {
                std::ostringstream oss;
                oss << "p" << std::setw(2) << std::setfill('0') << next_process_id;
                std::string process_name = oss.str();
                create_process(process_name);
                next_process_id++;
            }

            while (cpu->can_run_process()) {
                Process* next_process = scheduler->get_next_process();
                if (next_process) {
                    cpu->add_process(next_process);
                }
                else {
                    // No more processes in the ready queue to schedule for now.
                    break;
                }
            }

            cpu->tick();
            cpu->execute_processes();
        }
        simulation_active = false;
        std::cout << "\nAll processes have finished. Simulation has stopped." << std::endl;
        std::cout << "root:\\> " << std::flush;
        }).detach();
}

void scheduler_stop() {
    scheduler_running = false;
    std::cout << "Scheduler stopped. Processes will no longer be generated." << std::endl;
}

void screen_ls() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    scheduler->display_status("screen -ls Detected");
}

void attach_to_screen(Process* target_process) {
    if (!target_process) {
        return;
    }
    system("cls");
    std::cout << "\n=== Process Screen: " << target_process->name << " ===" << std::endl;
    std::cout << "Type 'process-smi' to view process status" << std::endl;
    std::cout << "Type 'exit' to return to main menu" << std::endl;

    std::string command;
    while (true) {
        std::cout << target_process->name << ":\\> ";
        std::getline(std::cin, command);

        if (command == "exit") {
            break;
        }
        else if (command == "process-smi") {
            process_smi(target_process->name);
        }
        else {
            std::cout << "Unknown command. Available commands: process-smi, exit" << std::endl;
        }
    }
}

void screen_s(const std::string& process_name) {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    // Check if a process with the same name already exists
    for (const auto& process : processes) {
        if (process->name == process_name) {
            std::cout << "Error: Process with name '" << process_name << "' already exists. Use 'screen -r <name>' to attach." << std::endl;
            return;
        }
    }

    // If the name is unique, create the new process
    create_process(process_name);
    std::cout << "Process '" << process_name << "' created successfully." << std::endl;

    // Attach to the newly created process's screen
    Process* new_process = processes.back().get();
    attach_to_screen(new_process);
}

void process_smi(const std::string& process_name) {
    for (auto& process : processes) {
        if (process->name == process_name) {
            process->display_status();
            return;
        }
    }
    std::cout << "Process not found: " << process_name << std::endl;
}

void report_util() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    std::ofstream log_file("csopesy-log.txt");
    if (log_file.is_open()) {
        // Redirect cout to file temporarily
        std::streambuf* cout_buffer = std::cout.rdbuf();
        std::cout.rdbuf(log_file.rdbuf());

        scheduler->display_status("report-util generated");

        // Restore cout
        std::cout.rdbuf(cout_buffer);
        log_file.close();

        std::cout << "Report saved to csopesy-log.txt" << std::endl;
    }
    else {
        std::cout << "Error: Could not create log file." << std::endl;
    }
}

void create_process(const std::string& name) {
    auto new_process = std::make_unique<Process>(name, next_process_id++);
    new_process->arrival_time = cpu->get_current_time();
    processes.push_back(std::move(new_process));

    // Add to scheduler's ready queue
    scheduler->add_process(processes.back().get());
}

void process_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "initialize") {
        initialize_system();
    }
    else if (cmd == "scheduler-start") {
        scheduler_start();
    }
    else if (cmd == "scheduler-stop") {
        scheduler_stop();
    }
    else if (cmd == "screen") {
        std::string arg1, arg2;
        iss >> arg1 >> arg2;
        if (arg1 == "-ls") {
            screen_ls();
        }
        else if (arg1 == "-s" && !arg2.empty()) {
            screen_s(arg2);
        }
        else if (arg1 == "-r" && !arg2.empty()) {
            Process* target_process = nullptr;
            for (auto& p : processes) {
                if (p->name == arg2) {
                    target_process = p.get();
                    break;
                }
            }

            if (target_process) {
                if (target_process->status != "finished") {
                    attach_to_screen(target_process);
                }
                else {
                    std::cout << "Process " << arg2 << " has already finished and cannot be re-attached." << std::endl;
                }
            }
            else {
                std::cout << "Process " << arg2 << " not found." << std::endl;
            }
        }
        else {
            std::cout << "Invalid 'screen' command. Use 'screen -ls', 'screen -s <name>', or 'screen -r <name>'." << std::endl;
        }
    }
    else if (cmd == "report-util") {
        report_util();
    }
    else if (cmd == "clear") {
        system("cls");
    }
    else if (cmd == "help") {
        std::cout << "Available commands:\n";
        std::cout << "  initialize          - Initialize the system from config.txt\n";
        std::cout << "  scheduler-start     - Start process generation and execution\n";
        std::cout << "  scheduler-stop      - Stop generating new processes\n";
        std::cout << "  screen -ls          - Display system and process status\n";
        std::cout << "  screen -s <name>    - Create and attach to a new process screen\n";
        std::cout << "  screen -r <name>    - Re-attach to a running process screen\n";
        std::cout << "  report-util         - Generate a CPU utilization report\n";
        std::cout << "  clear               - Clear the console screen\n";
        std::cout << "  exit                - Exit the emulator\n";
    }
    else {
        std::cout << "Unknown command: " << command << ". Type 'help' for a list of commands." << std::endl;
    }
}
