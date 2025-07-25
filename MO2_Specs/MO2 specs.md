### **Project Overview: Multitasking OS with Memory Management**

This project extends the previous CLI and Process Scheduler by introducing a **Memory Manager**. The core new features include simulating physical and virtual memory, demand paging, a backing store for page swapping, and new commands/instructions to interact with memory. Processes now have a defined memory footprint and can be terminated due to memory access violations.

---

### **1. Core System Component: The Memory Manager**

#### **1.1. Memory Model**
*   **Global Memory:** The system simulates a fixed, finite amount of main memory (RAM), defined by `max-overall-mem` in `config.txt`. This memory is pre-allocated at startup.
*   **Paging:** Memory is managed using demand paging.
    *   **Page/Frame Size:** The total memory is divided into fixed-size blocks called **frames**. The size of a frame is equal to the size of a **page** and is defined by `mem-per-frame`.
    *   **Total Frames:** The total number of available frames is `max-overall-mem / mem-per-frame`.
*   **Process Memory:**
    *   Each process is allocated a specific amount of logical memory upon creation.
    *   This logical address space is not loaded into RAM all at once. Pages are loaded on demand.
    *   **Symbol Table:** Each process has a **fixed 64-byte symbol table segment** to store its variables. This can hold a maximum of 32 `uint16` variables. Any variable declarations beyond this limit are ignored.

#### **1.2. Demand Paging and Page Faults**
*   **Mechanism:** When a process attempts to execute an instruction or access data (`READ`/`WRITE`) that resides on a page not currently in a physical memory frame, a **page fault** occurs.
*   **Page Fault Handling Flow:**
    1.  The scheduler assigns a process to a CPU worker.
    2.  The process attempts to execute an instruction (e.g., `WRITE 0x500 varA`).
    3.  The Memory Management Unit (MMU) determines that the page containing address `0x500` is not in a physical frame. A page fault is triggered.
    4.  The OS finds a free frame in physical memory.
    5.  If no frames are free, a **page replacement algorithm** is used to select a "victim" frame.
    6.  The page currently in the victim frame is evicted (written to the backing store if it was modified).
    7.  The required page (containing `0x500`) is loaded from the backing store into the now-available frame.
    8.  The page table for the process is updated.
    9.  The instruction that caused the fault (`WRITE 0x500 varA`) is **restarted**.
    10. This process repeats indefinitely until a valid frame is secured.
*   **Scheduler Interaction:** A process instruction **cannot be executed** until the page it resides on is loaded into physical memory. Page fault handling must complete before the instruction is performed.

#### **1.3. Backing Store**
*   **Function:** A file named `csopesy-backing-store.txt` acts as the secondary storage for all virtual pages of all processes.
*   **Operations:** When a page is evicted from RAM, it's saved to this file. When a page is needed due to a page fault, it's loaded from this file.

---

### **2. Updated Configuration (`config.txt`)**

The `config.txt` file includes new parameters for memory management. All memory ranges are specified in bytes and must be a power of 2.

| Parameter | Type | Range | Description |
| :--- | :--- | :--- | :--- |
| `max-overall-mem` | integer | `[2^n, 2^16]` | Maximum physical memory available in the system (in bytes). |
| `mem-per-frame` | integer | `[2^n, 2^16]` | The size of a single memory frame/page (in bytes). |
| `min-mem-per-proc`| integer | `[2^n, 2^16]` | The minimum memory size a process created by `scheduler-start` can have. |
| `max-mem-per-proc`| integer | `[2^n, 2^16]` | The maximum memory size a process created by `scheduler-start` can have. |

---

### **3. New and Modified Commands**

#### **3.1. Main Menu Commands**

| Command | Description |
| :--- | :--- |
| `process-smi` | **(New)** Provides a summarized view of the system's memory allocation and utilization, similar to `nvidia-smi`. See **Section 5.1** for output format. |
| `vmstat` | **(New)** Provides a detailed, tabular view of active/inactive processes, available memory, and paging statistics. See **Section 5.2** for output format. |
| `screen -s <p_name> <p_mem_size>` | **(Modified)** Creates a new process with a specified name and **memory allocation size**. The memory size must be a power of 2 between `[2^4, 2^16]` bytes. The system will throw an "invalid memory allocation" message if the size is outside this range. A process requires at least 64 bytes for its variable symbol table. |
| `screen -c <p_name> <p_mem_size> "<instructions>"` | **(New)** Creates a new process with a specified name, memory size, and a user-defined, semicolon-separated list of instructions. Throws "Invalid command" if the instruction set size is not met (e.g., not 50 instructions if required). |
| `screen -r <p_name>` | **(Modified Behavior)** In addition to previous functionality, if the process being attached to was terminated due to a memory access violation, the console should print: `Process <process name> shut down due to memory access violation error that occurred at <HH:MM:SS>: <Hex memory address> invalid.` |

#### **3.2. Sample `screen -c` Usage**

```
screen -c process2 256 "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT(\"Result: \" + varC)"
```
This command creates `process2` with 256 bytes of memory and provides it with 6 specific instructions to execute.

---

### **4. New and Modified Process Instructions**

New instructions are added to simulate memory access.

| Instruction | Parameters | Description |
| :--- | :--- | :--- |
| `READ(var, memory_address)` | `var`: destination variable name, `memory_address`: hexadecimal address | Performs a retrieval of a `uint16` value from the specified memory address and stores it in `var`. If the memory block has not been initialized, the returned value is `0`. This operation can trigger a page fault. |
| `WRITE(memory_address, value)`| `memory_address`: hexadecimal address, `value`: `uint16` value or variable name | Writes the specified `uint16` value to the specified memory address. This operation can trigger a page fault. |
| `DECLARE(var, value)` | `var`: variable name, `value`: initial `uint16` value | **(Modified Behavior)** This instruction now requires the process's symbol table segment (the first 64 bytes of its memory) to be loaded in a physical frame. If not, it will trigger a page fault. |

**Critical Behavior Note:** Attempting to `READ` or `WRITE` to an invalid memory address (i.e., an address outside the process's allocated memory space) must trigger an **access violation error**. This error immediately terminates the process and shuts it down.

---

### **5. UI/Output Mockups**

#### **5.1. `process-smi` Output Format**

```
root:\> process-smi
+--------------------------------------------------+
| PROCESS-SMI V01 Driver Version: 01.00            |
|--------------------------------------------------|
| CPU-Util: 100%                                   |
| Memory Usage: 1345MiB / 4759MiB                  |
| Memory Util: 28%                                 |
|--------------------------------------------------|
| Running processes and memory usage:              |
|==================================================|
| process05  134MiB                                |
| process06  12MiB                                 |
| process07  977MiB                                |
+--------------------------------------------------+
root:\>
```

#### **5.2. `vmstat` Output Format**

| Header | Description |
| :--- | :--- |
| `Total memory` | Total main memory in bytes. |
| `Used memory` | Total active memory used by all processes. |
| `Free memory` | Total free memory that can still be used by other processes. |
| `Idle ticks` | Number of ticks wherein CPU cores remained idle. |
| `Active cpu ticks` | Number of ticks wherein CPU cores were actually executing instructions. |
| `Total cpu ticks`| Total number of ticks that passed for all CPU cores. |
| `Num paged in` | Accumulated number of pages paged in from the backing store. |
| `Num paged out`| Accumulated number of pages paged out to the backing store. |

### **6. Additional Information & Clarifications**

This section consolidates clarifications based on common questions to ensure consistent implementation.

#### **6.1. Memory Management & Process Creation**

*   **Process Memory Validation:** When a process is created (via `scheduler-start` or `screen -c`), the system must verify if the process's code can fit into its allocated memory. A process's total required memory is its instruction size plus the **fixed 64-byte symbol table**. If this total exceeds the memory allocated to the process (e.g., `p_mem_size` from `screen -c` or the randomly assigned size from `scheduler-start`), the process cannot be created. This should be treated as an "out-of-memory" condition, and the process must be discarded/terminated immediately, preventing it from ever entering the ready queue. This simulates a system starved for RAM.
*   **Memory Write Behavior:** The system should not prevent a process from overwriting its own data within its allocated memory space. For example, a `WRITE` instruction targeting a valid address that currently holds another instruction is a legal operation. This may corrupt the process and cause it to fail later, which is intended behavior. The only specified memory error that must cause immediate termination is an **access violation**, which occurs when a `READ` or `WRITE` targets an address *outside* of the process's allocated logical memory space.
*   **Backing Store Format:** The `csopesy-backing-store.txt` file can store data in a binary format. It does not need to be human-readable text.

#### **6.2. Process Instruction Behavior (MCO1 Carry-over)**

*   **`SLEEP(X)` Instruction:** A process executing `SLEEP(X)` must relinquish the CPU immediately. It should be moved from the `Running` state to a `Sleeping` state and placed in a separate sleeping/waiting queue. It should not consume CPU resources while sleeping (i.e., no busy-waiting). After `X` CPU ticks have passed, the process should be moved back to the `Ready` queue.
*   **`FOR` Loop Parameters:** The `repeats` parameter in a `FOR` loop must be an integer literal. It cannot be a variable.
*   **Variable Scope:** Variables declared with `DECLARE` are global to the process. There is no block-level or loop-level scoping. Declaring a variable inside a `FOR` loop will create or overwrite the process-wide variable.

#### **6.3. Command & UI Clarifications (MCO1 Carry-over)**

*   **`screen -r` vs. `screen -s`:**
    *   `screen -r <p_name>` attaches to a process's console view, primarily to see the real-time output of its `PRINT` instructions.
    *   `screen -s <p_name>` does everything `screen -r` does, but **also** enables the `process-smi` command to view the process's detailed status (ID, instruction pointer, logs, etc.).
    *   The `exit` command is used to detach from the process screen and return to the main CLI in both modes.
*   **`process-smi` Log Content:** The `Logs:` section of the `process-smi` output should **only** display the timestamped messages generated by the `PRINT` instruction.

#### **6.4. General Implementation Notes**

*   **Scheduler Design:** The scheduler does not need to be a single, dedicated thread that dispatches jobs. An alternative, valid implementation is to have the CPU worker threads pull processes directly from a thread-safe (synchronized) ready queue.
*   **Debugging with `sleep()`:** Using system `sleep()` calls within your host application's threads (e.g., in the main loop or worker threads) to slow down the simulation for debugging or demonstration is permitted. However, be aware that this can alter the natural timing and behavior of the scheduler. Relying on log files (`csopesy-log.txt`) is the preferred method for verifying correct behavior.
