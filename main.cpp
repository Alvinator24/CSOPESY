#include <iostream>
#include <string>

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
