### **Project Overview: Process Scheduler and CLI Emulator**

The objective is to build a command-line application that emulates a basic operating system, featuring a process scheduler, a process multiplexer, and a command-line interpreter (CLI). The application will manage and execute dummy processes based on a set of predefined instructions and configuration settings.

---

### **1. Core Components & Behavior**

#### **1.1. Main Console (CLI)**
*   **Appearance:** The console should present a welcome screen upon launch and then a command prompt.
    
*   **Initial State:** The system is uninitialized. The only commands recognized initially are `initialize` and `exit`. No other command should work until `initialize` has been successfully executed.

#### **1.2. Process Scheduler**
*   **Function:** A real-time scheduler that continuously assigns processes to available CPU cores as long as the console is running.
*   **Algorithm:** The scheduling algorithm is set via the `config.txt` file. Supported algorithms are:
    *   `fcfs` (First-Come, First-Served)
    *   `rr` (Round-Robin)
*   **CPU Ticks:** Time is measured in "CPU ticks," which is a simple integer counter that increments continuously while the application is running.
    ```c
    int cpuCycles = 0;
    while (<OS is running>) {
        cpuCycles++;
        // ... scheduling and execution logic ...
    }
    ```

#### **1.3. Process Representation**
Each process must have the following attributes:
*   **Process ID (ID):** A unique identifier.
*   **Process Name:** A human-readable name (e.g., `screen_01`, `p01`).
*   **State:** Running, Ready, Sleeping, Finished.
*   **Instructions:** A list of pre-determined or randomized "barebones instructions" to execute.
*   **Instruction Pointer:** The current line number of the instruction being executed.
*   **Total Lines of Code:** The total number of instructions for the process.
*   **Logs:** A history of outputs from `PRINT` instructions, timestamped and marked with the core that executed it.
*   **Variables:** A key-value store for variables declared within the process (e.g., using `DECLARE`). Variables are of type `uint16` and are retained until the process finishes.

---

### **2. Configuration (`config.txt`)**

The `initialize` command reads its settings from a space-separated file named `config.txt`.

| Parameter | Type | Range | Description |
| :--- | :--- | :--- | :--- |
| `num-cpu` | integer | `[1, 128]` | Number of available CPU cores. |
| `scheduler` | string | `"fcfs"` or `"rr"` | The scheduling algorithm to use. |
| `quantum-cycles`| integer | `[1, 2^32]` | Time slice for the Round-Robin (`rr`) scheduler. No effect on `fcfs`. |
| `batch-process-freq` | integer | `[1, 2^32]` | Frequency (in CPU ticks) for generating new processes when `scheduler-start` is active. |
| `min-ins` | integer | `[1, 2^32]` | Minimum number of instructions a new process can have. |
| `max-ins` | integer | `[1, 2^32]` | Maximum number of instructions a new process can have. |
| `delays-per-exec` | integer | `[0, 2^32]` | A "busy-wait" delay. The process holds the CPU for this many extra ticks before executing its next instruction. `0` means one instruction per tick. |

**Sample `config.txt`:**
```
num-cpu 4
scheduler "rr"
quantum-cycles 5
batch-process-freq 1
min-ins 1000
max-ins 2000
delay-per-exec 0
```

---

### **3. Command Specification**

#### **3.1. Main Menu Commands**

These commands are entered at the main `root:\>` prompt.

| Command | Description |
| :--- | :--- |
| `initialize` | **Must be run first.** Reads `config.txt` to set up the CPU and scheduler parameters. Enables all other commands. |
| `exit` | Terminates the entire console application. |
| `screen -s <process_name>` | Creates a **new process** with the given name. The process is populated with a randomized set of instructions. The console view immediately switches to this new process's screen. |
| `screen -r <process_name>` | Attaches to a **running process's screen**. If the process is not found or has already finished, it prints `Process <process_name> not found.` |
| `screen -ls` | Lists system status and all processes. Does not save to a file. See **Section 4.1** for output format. |
| `scheduler-start` | Starts automatically generating a batch of dummy processes. Every `batch-process-freq` CPU ticks, a new process is created (e.g., `p01`, `p02`, ...) and added to the ready queue. |
| `scheduler-stop` | Stops the automatic generation of new processes started by `scheduler-start`. |
| `report-util` | Generates a CPU utilization report with the same content as `screen -ls`, but saves it to a file named `csopesy-log.txt`. Prints `Report generated at C:/csopesy-log.txt!` to the console. |

#### **3.2. Process Screen Commands**

These commands are available only after attaching to a process via `screen -s` or `screen -r`. The prompt remains `root:\>`.

| Command | Description |
| :--- | :--- |
| `process-smi` | ("Process-System Monitoring Interface") Displays the real-time status of the attached process. See **Section 4.2** for output format. If the process has finished, it should display "Finished!" after the logs. |
| `exit` | Detaches from the current process screen and returns to the main menu console. If the process finishes while attached, the user can `exit`, but cannot re-attach via `screen -r`. |

---

### **4. UI/Output Mockups**

#### **4.1. `screen -ls` and `report-util` Output Format**

```
root:\> screen -ls
CPU utilization: 100%
Cores used: 4
Cores available: 0

Running processes:
process05   (01/18/2024 09:15:22AM)   Core: 0   1235 / 5876
process06   (01/18/2024 09:17:22AM)   Core: 1   3 / 5876
process07   (01/18/2024 09:17:45AM)   Core: 2   9 / 1000
process08   (01/18/2024 09:18:58AM)   Core: 3   12 / 80

Finished processes:
process01   (01/18/2024 09:00:21AM)   Finished   5876 / 5876
process02   (01/18/2024 09:00:22AM)   Finished   5876 / 5876
process03   (01/18/2024 09:00:42AM)   Finished   1000 / 1000
process04   (01/18/2024 09:00:53AM)   Finished   80 / 80

root:\>
```

#### **4.2. `process-smi` Output Format (within a Process Screen)**

```
root:\> process-smi
Process name: screen_01
ID: 1
Logs:
(08/06/2024 09:15:22AM) Core:0 "Hello world from screen_01!"
(08/06/2024 09:32:09AM) Core:1 "Hello world from screen_01!"

Current instruction line: 153
Lines of code: 1240

root:\>
```
*If the process has finished, the output should be:*
```
Process name: screen_01
ID: 1
Logs:
(08/06/2024 09:15:22AM) Core:0 "Hello world from screen_01!"
...
(08/06/2024 09:33:12AM) Core:1 "Hello world from screen_01!"

Finished!

root:\>
```

---

### **5. Barebones Process Instructions**

These are the operations that constitute a process's "code." They are pre-determined for each process and are **not** typed by the user.

| Instruction | Parameters | Description |
| :--- | :--- | :--- |
| `PRINT(msg)` | `msg`: a string, can include variables | Displays the `msg` to the console, but the output is **only visible when the user is attached** to that process's screen. The output must be added to the process's logs. By default, the message should be `"Hello world from <process_name>!"`. |
| `DECLARE(var, value)` | `var`: variable name, `value`: initial `uint16` value | Declares a `uint16` variable with the given name and initial value. Variables are clamped between 0 and `max(uint16)`. |
| `ADD(var1, p2, p3)` | `var1`: result variable, `p2, p3`: variable names or `uint16` values | Performs `var1 = p2 + p3`. Variables are automatically declared with a value of 0 if not previously declared. |
| `SUBTRACT(var1, p2, p3)` | `var1`: result variable, `p2, p3`: variable names or `uint16` values | Performs `var1 = p2 - p3`. Variables are automatically declared with a value of 0 if not previously declared. |
| `SLEEP(X)` | `X`: `uint8` value | The process relinquishes the CPU and enters a "sleeping" state for `X` CPU ticks. |
| `FOR([instructions], repeats)` | `instructions`: array of instructions, `repeats`: integer | Performs a for-loop, executing the given set of instructions `repeats` times. Can be nested up to 3 levels deep. |
