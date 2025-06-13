#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <array>
#include <memory>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <filesystem>

class Process {
public:
    int pid;
    std::string name;
    std::chrono::system_clock::time_point arrivalTime;
    int totalInstructions;
    int executedInstructions;
    enum State { READY, RUNNING, FINISHED } state;
    std::vector<std::string> printCommands;
    std::string logFileName;

    Process(int id, const std::string& n, int totalInst, const std::vector<std::string>& prints): pid(id), name(n), totalInstructions(totalInst), executedInstructions(0), state(READY), printCommands(prints) {
        arrivalTime = std::chrono::system_clock::now();
        logFileName = "process_logs/" + name + "_log.txt";
    }

    void logPrintCommand(int coreId, const std::string& message) {
        std::ofstream outFile(logFileName, std::ios_base::app);
        if (outFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
            std::tm* localTime = std::localtime(&currentTime);

            std::stringstream ss;
            ss << std::put_time(localTime, "(%m/%d/%Y %I:%M:%S%p)");
            std::string timestamp = ss.str();

            outFile << timestamp << "\tCore: " << coreId << " \"" << message << "\"" << std::endl;
            outFile.close();
        } else {
            std::cerr << "Error: Could not open log file for " << name << ": " << logFileName << std::endl;
        }
    }

    void executeNextInstruction(int coreId) {
        if (executedInstructions < totalInstructions) {
            if (static_cast<size_t>(executedInstructions) < printCommands.size()) {
                logPrintCommand(coreId, printCommands[executedInstructions]);
            }
            executedInstructions++;
        }
    }

    double getProgress() const {
        return (double)executedInstructions / totalInstructions;
    }
};

class Scheduler {
private:
    const int NUM_CORES = 4;
    std::queue<std::shared_ptr<Process>> processQueue;
    std::array<std::shared_ptr<Process>, 4> runningProcesses;
    std::array<bool, 4> coreBusy;

    std::vector<std::shared_ptr<Process>> allProcesses;

    std::mutex queueMutex;
    std::mutex coreMutex;
    std::mutex printMutex;

    std::condition_variable schedulerCv;
    std::array<std::condition_variable, 4> coreCvs;

    std::thread schedulerThread;
    std::array<std::thread, 4> cpuWorkerThreads;

    std::atomic<bool> running;

    std::string formatTime(const std::chrono::system_clock::time_point& time_point) const {
        std::time_t currentTime = std::chrono::system_clock::to_time_t(time_point);
        std::tm* localTime = std::localtime(&currentTime);
        std::stringstream ss;
        ss << std::put_time(localTime, "%m/%d/%Y %I:%M:%S%p");
        std::string formatted = ss.str();
        std::transform(formatted.begin(), formatted.end(), formatted.begin(), ::toupper);
        return formatted;
    }

    void schedulerLoop() {
        while (running.load()) {
            std::unique_lock<std::mutex> lock(queueMutex);
            schedulerCv.wait(lock, [this]{ return !processQueue.empty() || !running.load(); });

            if (!running.load() && processQueue.empty()) {
                break;
            }

            for (int i = 0; i < NUM_CORES; ++i) {
                if (!processQueue.empty()) {
                    std::unique_lock<std::mutex> coreLock(coreMutex);
                    if (!coreBusy[i]) {
                        std::shared_ptr<Process> processToRun = processQueue.front();
                        processQueue.pop();

                        runningProcesses[i] = processToRun;
                        runningProcesses[i]->state = Process::RUNNING;
                        coreBusy[i] = true;
                        coreLock.unlock();
                        coreCvs[i].notify_one();
                    }
                } else {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void coreWorker(int coreId) {
        while (running.load()) {
            std::shared_ptr<Process> currentProcess;
            {
                std::unique_lock<std::mutex> lock(coreMutex);
                coreCvs[coreId].wait(lock, [this, coreId]{ return coreBusy[coreId] || !running.load(); });
                if (!running.load() && !coreBusy[coreId]) {
                    break;
                }
                currentProcess = runningProcesses[coreId];
            }

            if (currentProcess) {
                while (currentProcess->executedInstructions < currentProcess->totalInstructions && running.load()) {
                    currentProcess->executeNextInstruction(coreId);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }

                {
                    std::lock_guard<std::mutex> lock(coreMutex);
                    currentProcess->state = Process::FINISHED;
                    runningProcesses[coreId].reset();
                    coreBusy[coreId] = false;
                }
                schedulerCv.notify_one();
            }
        }
    }

public:
    Scheduler() : running(false) {
        try {
            if (!std::filesystem::exists("process_logs")) {
                std::filesystem::create_directory("process_logs");
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory 'process_logs': " << e.what() << std::endl;
        }
    }

    ~Scheduler() {
        stop();
        for (auto& process : allProcesses) {
            if (process->state != Process::FINISHED) {
                process->state = Process::FINISHED;
            }
        }
    }

    void start() {
        running.store(true); 
        schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
        for (int i = 0; i < NUM_CORES; ++i) {
            coreBusy[i] = false;
            cpuWorkerThreads[i] = std::thread(&Scheduler::coreWorker, this, i);
        }
    }

    void stop() {
        if (running.load()) {
            running.store(false);
            schedulerCv.notify_all();
            for (int i = 0; i < NUM_CORES; ++i) {
                coreCvs[i].notify_one();
            }

            if (schedulerThread.joinable()) {
                schedulerThread.join();
            }
            for (int i = 0; i < NUM_CORES; ++i) {
                if (cpuWorkerThreads[i].joinable()) {
                    cpuWorkerThreads[i].join();
                }
            }
        }
    }

    void addProcess(std::shared_ptr<Process> process) {
        std::lock_guard<std::mutex> lock(queueMutex);
        processQueue.push(process);
        allProcesses.push_back(process);
        schedulerCv.notify_one();
    }

    void printScreenLs() {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "--------------------------------------" << std::endl;
        std::cout << "Running processes:" << std::endl;
        std::vector<std::shared_ptr<Process>> currentRunningProcesses;
        {
            std::lock_guard<std::mutex> coreLock(coreMutex);
            for (int i = 0; i < NUM_CORES; ++i) {
                if (runningProcesses[i]) {
                    currentRunningProcesses.push_back(runningProcesses[i]);
                }
            }
        }

        for (const auto& p : currentRunningProcesses) {
            int coreIdx = -1;
            {
                std::lock_guard<std::mutex> coreLock(coreMutex);
                for(int i = 0; i < NUM_CORES; ++i) {
                    if (runningProcesses[i] == p) {
                        coreIdx = i;
                        break;
                    }
                }
            }
            std::cout << p->name << " (" << formatTime(p->arrivalTime) << ")\tCore: " << coreIdx << "\t"
                << p->executedInstructions << " / " << p->totalInstructions << std::endl;
        }

        std::cout << "\nFinished processes:" << std::endl;
        bool hasFinished = false;
        std::vector<std::shared_ptr<Process>> finishedProcesses;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (const auto& p : allProcesses) {
                if (p->state == Process::FINISHED) {
                    finishedProcesses.push_back(p);
                }
            }
        }

        for (const auto& p : finishedProcesses) {
            hasFinished = true;
            std::cout << p->name << " (" << formatTime(p->arrivalTime) << ")\tFinished\t"
                << p->executedInstructions << " / " << p->totalInstructions << std::endl;
        }
        if (!hasFinished) {
            std::cout << "(No processes finished yet)" << std::endl;
        }
        std::cout << "-----------------------------------------" << std::endl;
    }

    bool allProcessesFinished() {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& p : allProcesses) {
            if (p->state != Process::FINISHED) {
                return false;
            }
        }
        return true;
    }
};

std::vector<std::string> generatePrintCommands(const std::string& processName, int count) {
    std::vector<std::string> commands;
    commands.reserve(count);
    for (int i = 0; i < count; ++i) {
        commands.push_back("Hello world from " + processName + "!");
    }
    return commands;
}

int main() {
    Scheduler scheduler;

    for (int i = 0; i < 10; ++i) {
        std::string p_name = "process" + (i < 9 ? std::string("0") : std::string("")) + std::to_string(i + 1);
        std::vector<std::string> prints = generatePrintCommands(p_name, 100);
        scheduler.addProcess(std::make_shared<Process>(i + 1, p_name, 100, prints));
    }

    scheduler.start();

    std::cout << "\nType 'screen -ls' to view processes or 'exit' to quit." << std::endl;
    std::string command;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin >> std::ws, command);

        if (command == "screen -ls") {
            scheduler.printScreenLs();
        }
        
        if (command == "exit") {
            break;
        }

        if (scheduler.allProcessesFinished()) {
            std::cout << "\nAll processes have finished." << std::endl;
            break;
        }
    }

    scheduler.stop();

    return 0;
}
