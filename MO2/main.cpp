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
#include "mmu.h"

// Global variables
std::unique_ptr<CPU> cpu;
std::unique_ptr<Scheduler> scheduler;
std::unique_ptr<Config> config;
std::unique_ptr<MMU> mmu;
bool system_initialized = false;
bool scheduler_running = false; // This now only controls automatic process generation
std::vector<std::unique_ptr<Process>> processes;
int next_process_id = 1;

// For the main simulation loop
std::thread simulation_thread;
std::atomic<bool> system_shutdown(false);

// Function declarations
void initialize_system();
void scheduler_start();
void scheduler_stop();
void screen_ls();
void screen_s(const std::string& process_name, int memory_size);
void screen_c(const std::string& process_name, int memory_size, const std::string& instructions);
void screen_r(const std::string& process_name);
void process_smi();
void vmstat();
void report_util();
void process_command(const std::string& command);
void create_process(const std::string& name, int memory_size, const std::vector<std::string>& instructions = {});
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
            system_shutdown = true;
            if (simulation_thread.joinable()) {
                simulation_thread.join();
            }
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
        default_config << "max-overall-mem: 8192\n";
        default_config << "mem-per-frame: 256\n";
        default_config << "min-mem-per-proc: 512\n";
        default_config << "max-mem-per-proc: 1024\n";
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
                else if (key == "max-overall-mem") config->max_overall_mem = std::stoi(value);
                else if (key == "mem-per-frame") config->mem_per_frame = std::stoi(value);
                else if (key == "min-mem-per-proc") config->min_mem_per_proc = std::stoi(value);
                else if (key == "max-mem-per-proc") config->max_mem_per_proc = std::stoi(value);
            }
        }
        config_file.close();
    }

    mmu = std::make_unique<MMU>();
    mmu->initialize(config->max_overall_mem, config->mem_per_frame);

    cpu = std::make_unique<CPU>(config->num_cpu, mmu.get());
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
    std::cout << "max-overall-mem: " << config->max_overall_mem << std::endl;
    std::cout << "mem-per-frame: " << config->mem_per_frame << std::endl;
    std::cout << "min-mem-per-proc: " << config->min_mem_per_proc << std::endl;
    std::cout << "max-mem-per-proc: " << config->max_mem_per_proc << std::endl;

    // Start the main simulation thread
    simulation_thread = std::thread([]() {
        int tick_count = 0;
        while (!system_shutdown.load()) {
            if (!system_initialized || !config) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(config->delay_per_exec));
            tick_count++;

            // Automatic process generation (controlled by scheduler-start/stop)
            if (scheduler_running && (tick_count % config->batch_process_freq == 0)) {
                std::ostringstream oss;
                oss << "p" << std::setw(2) << std::setfill('0') << next_process_id;
                std::string process_name = oss.str();
                
                int mem_size = config->min_mem_per_proc + (rand() % (config->max_mem_per_proc - config->min_mem_per_proc + 1));
                
                // Admission Control for auto-generated processes
                if (mmu->can_admit_process(mem_size)) {
                    create_process(process_name, mem_size);
                    next_process_id++;
                }
            }

            // Scheduling and Execution (always running)
            while (cpu->can_run_process()) {
                Process* next_process = scheduler->get_next_process();
                if (next_process) {
                    cpu->add_process(next_process);
                } else {
                    break;
                }
            }

            cpu->tick();
            cpu->execute_processes();
        }
        });
}

void scheduler_start() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }
    scheduler_running = true;
    std::cout << "Automatic process generation started." << std::endl;
}

void scheduler_stop() {
    scheduler_running = false;
    std::cout << "Automatic process generation stopped." << std::endl;
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
            process_smi();
        }
        else {
            std::cout << "Unknown command. Available commands: process-smi, exit" << std::endl;
        }
    }
}

void screen_s(const std::string& process_name, int memory_size) {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    // Memory size validation (Power of 2)
    if (memory_size < 64 || memory_size > 65536 || (memory_size > 0 && (memory_size & (memory_size - 1)) != 0)) {
        std::cout << "invalid memory allocation" << std::endl;
        return;
    }

    // Check if a process with the same name already exists
    for (const auto& process : processes) {
        if (process->name == process_name) {
            std::cout << "Error: Process with name '" << process_name << "' already exists. Use 'screen -r <name>' to attach." << std::endl;
            return;
        }
    }

    // Admission Control
    if (!mmu->can_admit_process(memory_size)) {
        std::cout << "Error: Not enough memory to create process '" << process_name << "'." << std::endl;
        return;
    }

    // If the name is unique, create the new process
    create_process(process_name, memory_size);
    std::cout << "Process '" << process_name << "' created successfully with " << memory_size << " bytes of memory." << std::endl;

    // Attach to the newly created process's screen
    Process* new_process = processes.back().get();
    attach_to_screen(new_process);
}

void screen_c(const std::string& process_name, int memory_size, const std::string& instructions_str) {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    // Memory size validation
    if (memory_size < 64 || memory_size > 65536 || (memory_size > 0 && (memory_size & (memory_size - 1)) != 0)) {
        std::cout << "invalid memory allocation" << std::endl;
        return;
    }

    for (const auto& p : processes) {
        if (p->name == process_name) {
            std::cout << "Error: Process with name '" << process_name << "' already exists." << std::endl;
            return;
        }
    }

    // Admission Control
    if (!mmu->can_admit_process(memory_size)) {
        std::cout << "Error: Not enough memory to create process '" << process_name << "'." << std::endl;
        return;
    }

    std::vector<std::string> instruction_vec;
    std::stringstream ss(instructions_str);
    std::string item;
    while (std::getline(ss, item, ';')) {
        // trim leading/trailing whitespace
        item.erase(0, item.find_first_not_of(" \t\n\r"));
        item.erase(item.find_last_not_of(" \t\n\r") + 1);
        if (!item.empty()) {
            instruction_vec.push_back(item);
        }
    }

    create_process(process_name, memory_size, instruction_vec);
    std::cout << "Process '" << process_name << "' created successfully with custom instructions." << std::endl;
}

void screen_r(const std::string& process_name) {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    for (auto& process : processes) {
        if (process->name == process_name) {
            if (process->status == "memory_violation") {
                std::cout << "Process <" << process_name << "> shut down due to a memory access violation error that occurred at "
                          << std::hex << process->faulting_address << std::dec << " <Hex memory address> invalid." << std::endl;
                return;
            }
            if (process->status == "finished") {
                std::cout << "Process <" << process_name << "> has already finished." << std::endl;
                return;
            }
            attach_to_screen(process.get());
            return;
        }
    }
    std::cout << "Process <" << process_name << "> not found." << std::endl;
}

void process_smi() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }
    
    std::cout << "[PROCESS-SMI]" << std::endl;
    
    double cpu_util = (cpu->get_num_cores() > 0) ? (static_cast<double>(cpu->get_utilization()) * 100) / cpu->get_num_cores() : 0.0;
    double mem_util = (mmu->get_total_memory() > 0) ? (static_cast<double>(mmu->get_used_memory()) * 100) / mmu->get_total_memory() : 0.0;

    std::cout << "CPU-UTIL: " << std::fixed << std::setprecision(0) << cpu_util << "%" << std::endl;
    std::cout << "MEM-UTIL: " << std::fixed << std::setprecision(0) << mem_util << "%" 
              << " (" << mmu->get_used_memory() << "B / " << mmu->get_total_memory() << "B)" << std::endl;
              
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Running processes and memory usage:" << std::endl;
    for (const auto& p : processes) {
        if (p->status != "finished" && p->status != "memory_violation") {
            std::cout << p->name << ": " << mmu->get_process_memory_usage(p->id) << "B (" << p->status << ")" << std::endl;
        }
    }
}

void vmstat() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    std::cout << "[VMSTAT]" << std::endl;
    std::cout << "Total main memory: " << mmu->get_total_memory() << "B" << std::endl;
    std::cout << "Used memory: " << mmu->get_used_memory() << "B" << std::endl;
    std::cout << "Free memory: " << mmu->get_free_memory() << "B" << std::endl;
    // Active/Inactive memory is not implemented yet in MMU
    // std::cout << "Active memory: " << mmu->get_active_memory() << "B" << std::endl;
    // std::cout << "Inactive memory: " << mmu->get_inactive_memory() << "B" << std::endl;
    std::cout << "---" << std::endl;
    // Ticks are not fully implemented in CPU for this level of detail
    // std::cout << "Idle cpu ticks: " << cpu->get_idle_ticks() << std::endl;
    // std::cout << "Active cpu ticks: " << cpu->get_active_ticks() << std::endl;
    // std::cout << "Total cpu ticks: " << cpu->get_total_ticks() << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "Num paged in: " << mmu->get_page_ins() << std::endl;
    std::cout << "Num paged out: " << mmu->get_page_outs() << std::endl;
}


void report_util() {
    if (!system_initialized) {
        std::cout << "Error: System not initialized. Use 'initialize' first." << std::endl;
        return;
    }

    // Open file for writing the report
    std::ofstream report_file("csopesy-log.txt", std::ios::app);
    if (!report_file.is_open()) {
        std::cout << "Error opening report file." << std::endl;
        return;
    }

    // Redirect cout to the file
    std::streambuf* coutbuf = std::cout.rdbuf(); 
    std::cout.rdbuf(report_file.rdbuf());

    report_file << "report-util triggered" << std::endl;
    scheduler->display_status("report-util");

    // Restore cout
    std::cout.rdbuf(coutbuf);

    std::cout << "Report generated at C:/csopesy-log.txt" << std::endl;
}

void create_process(const std::string& name, int memory_size, const std::vector<std::string>& instructions) {
    auto new_process = std::make_unique<Process>(name, next_process_id++, memory_size, mmu.get());
    
    if (instructions.empty()) {
        new_process->generate_instructions(config->min_instructions, config->max_instructions);
    } else {
        new_process->instructions = instructions;
        new_process->total_instructions = static_cast<int>(instructions.size());
    }
    
    processes.push_back(std::move(new_process));
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
        std::string sub_cmd;
        iss >> sub_cmd;
        if (sub_cmd == "-ls") {
            screen_ls();
        } else if (sub_cmd == "-s") {
            std::string name;
            int mem_size;
            if (iss >> name >> mem_size) {
                screen_s(name, mem_size);
            } else {
                std::cout << "Usage: screen -s <name> <mem_size>" << std::endl;
            }
        } else if (sub_cmd == "-c") {
            std::string name;
            int mem_size;
            std::string instructions;
            if (iss >> name >> mem_size >> std::ws && std::getline(iss, instructions)) {
                // remove quotes if present
                if (instructions.front() == '"' && instructions.back() == '"') {
                    instructions = instructions.substr(1, instructions.length() - 2);
                }
                screen_c(name, mem_size, instructions);
            } else {
                std::cout << "Usage: screen -c <name> <mem_size> \"<instructions>\"" << std::endl;
            }
        } else if (sub_cmd == "-r") {
            std::string name;
            if (iss >> name) {
                screen_r(name);
            } else {
                std::cout << "Usage: screen -r <name>" << std::endl;
            }
        } else {
            std::cout << "Unknown screen command." << std::endl;
        }
    }
    else if (cmd == "process-smi") {
        process_smi();
    }
    else if (cmd == "vmstat") {
        vmstat();
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
