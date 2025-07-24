#include "mmu.h"
#include "process.h"
#include <fstream>
#include <iostream>
#include <cstring> // Required for memcpy
#include <algorithm>

MMU::MMU() : total_memory(0), frame_size(0), total_frames(0), backing_store_path("csopesy-backing-store.txt") {}

void MMU::initialize(int total_memory, int frame_size) {
    this->total_memory = total_memory;
    this->frame_size = frame_size;
    this->total_frames = total_memory / frame_size;

    physical_memory.resize(total_memory, 0);
    frame_is_free.resize(total_frames, true);

    // Create or clear the backing store file
    std::ofstream backing_store(backing_store_path, std::ios::trunc);
    backing_store.close();
}

uint16_t MMU::read(Process* process, uint32_t virtual_address) {
    if (virtual_address >= static_cast<uint32_t>(process->memory_size)) {
        throw std::out_of_range("Virtual address out of bounds");
    }

    int page_number = virtual_address / frame_size;
    int offset = virtual_address % frame_size;

    if (!page_tables[process->id][page_number].valid) {
        throw PageFaultException("Page fault on read", process->id, virtual_address);
    }

    int frame_number = page_tables[process->id][page_number].frame_number;
    uint16_t value;
    memcpy(&value, &physical_memory[frame_number * frame_size + offset], sizeof(uint16_t));

    return value;
}

void MMU::write(Process* process, uint32_t virtual_address, uint16_t value) {
    if (virtual_address >= static_cast<uint32_t>(process->memory_size)) {
        throw std::out_of_range("Virtual address out of bounds");
    }

    int page_number = virtual_address / frame_size;
    int offset = virtual_address % frame_size;

    if (!page_tables[process->id][page_number].valid) {
        throw PageFaultException("Page fault on write", process->id, virtual_address);
    }

    int frame_number = page_tables[process->id][page_number].frame_number;
    page_tables[process->id][page_number].dirty = true;
    memcpy(&physical_memory[frame_number * frame_size + offset], &value, sizeof(uint16_t));
}

bool MMU::allocate_memory(int pid, int memory_size) {
    if (page_tables.find(pid) != page_tables.end()) {
        return false; // Already allocated
    }

    int num_pages = (memory_size + frame_size - 1) / frame_size;
    page_tables[pid].resize(num_pages);
    return true;
}

void MMU::free_memory(int pid) {
    if (page_tables.find(pid) == page_tables.end()) {
        return;
    }

    for (auto& pte : page_tables[pid]) {
        if (pte.valid) {
            frame_is_free[pte.frame_number] = true;
            // remove from LRU list
            if (lru_map.count(pte.frame_number)) {
                lru_list.erase(lru_map[pte.frame_number]);
                lru_map.erase(pte.frame_number);
            }
        }
    }
    page_tables.erase(pid);
}

int MMU::handle_page_fault(Process* process, uint32_t virtual_address) {
    page_ins++;
    int page_number = virtual_address / frame_size;

    // Find a free frame
    int free_frame = -1;
    for (int i = 0; i < total_frames; ++i) {
        if (frame_is_free[i]) {
            free_frame = i;
            break;
        }
    }

    if (free_frame == -1) {
        // No free frames, need to swap
        page_outs++;
        free_frame = find_victim_frame();
        swap_out(free_frame);
    }

    // We have a free frame, either new or from a victim
    frame_is_free[free_frame] = false;
    page_tables[process->id][page_number].valid = true;
    page_tables[process->id][page_number].frame_number = free_frame;
    page_tables[process->id][page_number].dirty = false;

    swap_in(process->id, page_number, free_frame);

    // Update LRU/FIFO list
    lru_list.push_back(free_frame);
    lru_map[free_frame] = --lru_list.end();

    return free_frame;
}

int MMU::get_total_memory() const { return total_memory; }
int MMU::get_used_memory() const {
    return (total_frames - get_free_frames_count()) * frame_size;
}
int MMU::get_free_memory() const {
    return get_free_frames_count() * frame_size;
}
int MMU::get_process_memory_usage(int pid) const {
    if (page_tables.count(pid)) {
        int valid_pages = 0;
        for (const auto& pte : page_tables.at(pid)) {
            if (pte.valid) {
                valid_pages++;
            }
        }
        return valid_pages * frame_size;
    }
    return 0;
}
int MMU::get_total_frames() const { return total_frames; }
int MMU::get_free_frames_count() const {
    int free_count = 0;
    for (bool status : frame_is_free) {
        if (status) {
            free_count++;
        }
    }
    return free_count;
}
int MMU::get_page_ins() const { return page_ins; }
int MMU::get_page_outs() const { return page_outs; }

int MMU::find_victim_frame() {
    // Simple FIFO
    if (lru_list.empty()) {
        return -1;
    }

    int victim_frame = lru_list.front();
    lru_list.pop_front();
    lru_map.erase(victim_frame);

    return victim_frame;
}

void MMU::swap_in(int pid, int page_number, int frame_number) {
    std::fstream backing_store(backing_store_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!backing_store) {
        // This might happen if the file doesn't exist yet for reading.
        // Let's ensure it exists.
        std::ofstream creator(backing_store_path, std::ios::app | std::ios::binary);
        creator.close();
        backing_store.open(backing_store_path, std::ios::in | std::ios::out | std::ios::binary);
    }

    long long offset = (long long)pid * (256 * frame_size) + (long long)page_number * frame_size; // Example offset calculation
    backing_store.seekg(offset);

    std::vector<char> buffer(frame_size);
    backing_store.read(buffer.data(), frame_size);

    // If we read less than a full frame, it means the page was not in the backing store (first touch).
    // In that case, the buffer is zeroed out by default, which is the desired behavior.

    memcpy(&physical_memory[frame_number * frame_size], buffer.data(), frame_size);
}

void MMU::swap_out(int frame_number) {
    int pid = get_pid_from_frame(frame_number);
    int page_number = get_page_number_from_frame(frame_number);

    if (pid == -1 || page_number == -1) {
        // Should not happen if frame is not free
        return;
    }

    if (page_tables[pid][page_number].dirty) {
        std::fstream backing_store(backing_store_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!backing_store) {
            std::ofstream creator(backing_store_path, std::ios::binary);
            creator.close();
            backing_store.open(backing_store_path, std::ios::in | std::ios::out | std::ios::binary);
        }

        long long offset = (long long)pid * (256 * frame_size) + (long long)page_number * frame_size;
        backing_store.seekp(offset);
        backing_store.write((char*)&physical_memory[frame_number * frame_size], frame_size);
    }

    // Invalidate page table entry for the old process
    page_tables[pid][page_number].valid = false;
    page_tables[pid][page_number].frame_number = -1;
}

int MMU::get_pid_from_frame(int frame_number) {
    for (auto const& [pid, p_table] : page_tables) {
        for (const auto& pte : p_table) {
            if (pte.valid && pte.frame_number == frame_number) {
                return pid;
            }
        }
    }
    return -1;
}

int MMU::get_page_number_from_frame(int frame_number) {
    for (auto const& [pid, p_table] : page_tables) {
        for (size_t i = 0; i < p_table.size(); ++i) {
            if (p_table[i].valid && p_table[i].frame_number == frame_number) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}