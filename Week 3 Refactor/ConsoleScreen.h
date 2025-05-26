#ifndef CONSOLE_SCREEN_H
#defien CONSOLE_SCREEN_H

#include <vector>
#include <string>
#include <iostream>
#include <memory>

//class ConsoleManager;

class ConsoleScreen {
protected:
    std::vector<std::string> buffer;
    
public:
    virtual ~ConsoleScreen() = default;
    
    void addline(const std::string &input);
    
    void displayBuffer() const;

    void clearBuffer();
}
