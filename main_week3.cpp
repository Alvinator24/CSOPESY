#include <iostream>
#include <string>
#include <ctime>

void printHeader() {
    std::cout << "  _____  _____  ____  _____  ______  _______     __\n";
    std::cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\   / /\n";
    std::cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\_/ / \n";
    std::cout << "| |     \\___ \\| |  | |  ___/|  __|  \\___ \\  \\   /  \n";
    std::cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   \n";
    std::cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   \n";
    std::cout << "Welcome to the Command-Line Operating System!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
}

class Screen {
public:
    std::string name;
    std::string timestamp;
    int currentLine;
    int totalLines;


    Screen(std::string n) : name(n), currentLine(1), totalLines(10) { //example placeholder for no. of lines
        time_t now = time(0);
        struct tm tstruct;
        char buf[35];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", &tstruct);
        timestamp = buf;
    }

    void display() {
        std::cout << "\n[SCREEN: " << name << "]\n";
        std::cout << "Process Name: " << name << "\n";
        std::cout << "Instruction: Line " << currentLine << " / " << totalLines << "\n";
        std::cout << "Created At: " << timestamp << "\n";
        std::cout << "Type 'exit' to return to main menu.\n";

        std::string subcmd;
        while (true) {
            std::cout << name << " > ";
            std::getline(std::cin, subcmd);
            if (subcmd == "exit") {
            break;
            }
            std::cout << "You entered: " << subcmd << "\n";
        }
    }
};

int main() {
    std::string command;
    
    printHeader();
    
    while (true) {
        std::cout << "Enter a command: ";
        std::getline(std::cin, command);  // Changed to getline to handle spaces
        
        if (command == "initialize") {
            std::cout << "'initialize' command recognized. Doing something.\n";
            std::cout << "============================================\n";
        }
        else if (command.rfind("screen -r ", 0) == 0) {
            std::string screenName = command.substr(10);
            if (screenName.empty()) {
                std::cout << "Error: No screen name provided.\n";
            } else {
                Screen screen(screenName);
                screen.display();
            }
        }
        else if (command.rfind("screen -s ", 0) == 0) {
            std::string screenName = command.substr(10);
            if (screenName.empty()) {
                std::cout << "Error: No screen name provided.\n";
            } else {
                Screen screen(screenName);
                screen.display();
            }
        }    
        else if (command == "screen") {
            std::cout << "'screen' command recognized. Doing something.\n";
            std::cout << "============================================\n";
        } 
        else if (command == "scheduler-test") {
            std::cout << "'scheduler-test' command recognized. Doing something.\n";
            std::cout << "============================================\n";
        } 
        else if (command == "scheduler-stop") {
            std::cout << "'scheduler-stop' command recognized. Doing something.\n";
            std::cout << "============================================\n";
        } 
        else if (command == "report-util") {
            std::cout << "'report-util' command recognized. Doing something.\n";
            std::cout << "============================================\n";
        } 
        else if (command == "clear") {
            system("cls");
            printHeader();
        } 
        else if (command == "exit") {
            std::cout << "Exiting program...\n";
            break;
        } else {
            std::cout << "Unknown command: " << command << "\n";
        }
    }
    
    return 0;
}
