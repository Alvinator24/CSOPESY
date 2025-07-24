#ifndef MMU_H
#define MMU_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <list>
#include <stdexcept>

class Process; // Forward declaration

struct PageFaultException : public std::runtime_error {
    int pid;
    uint32_t address;
    PageFaultException(const std::string& msg, int process_id, uint32_t fault_address)
        : std::runtime_error(msg), pid(process_id), address(fault_address) {}
};

struct PageTableEntry {
    bool valid = false;
    int frame_number = -1;
    bool dirty = false;
    uint64_t last_access_time = 0; // For LRU
};

class MMU {
public:
    MMU();
    void initialize(int total_memory, int frame_size);

    // Memory access
    uint16_t read(Process* process, uint32_t virtual_address);
    void write(Process* process, uint32_t virtual_address, uint16_t value);

    // Process memory management
    bool allocate_memory(int pid, int memory_size);
    void free_memory(int pid);

    // Page fault handler
    int handle_page_fault(Process* process, uint32_t virtual_address);

    // For vmstat and process-smi
    int get_total_memory() const;
    int get_used_memory() const;
    int get_free_memory() const;
    int get_process_memory_usage(int pid) const;
    int get_total_frames() const;
    int get_free_frames_count() const;
    int get_page_ins() const;
    int get_page_outs() const;


private:
    int total_memory;
    int frame_size;
    int total_frames;
    std::vector<uint8_t> physical_memory;
    std::vector<bool> frame_is_free;
    std::map<int, std::vector<PageTableEntry>> page_tables;
    std::string backing_store_path;

    // Page replacement data structures
    std::list<int> lru_list; // For LRU page replacement
    std::map<int, std::list<int>::iterator> lru_map;

    // Page replacement algorithm
    int find_victim_frame();
    void swap_in(int pid, int page_number, int frame_number);
    void swap_out(int frame_number);

    // Statistics
    int page_ins = 0;
    int page_outs = 0;

    // Helper
    int get_pid_from_frame(int frame_number);
    int get_page_number_from_frame(int frame_number);
};

#endif // MMU_H 