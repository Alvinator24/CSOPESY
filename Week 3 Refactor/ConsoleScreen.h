#ifndef CONSOLE_SCREEN_H
#define CONSOLE_SCREEN_H

#include <vector>
#include <string>
#include <iostream>


class ConsoleScreen {
protected:
    std::vector<std::string> buffer;
    
public:
    virtual ~ConsoleScreen() = default;
    
    void addLine(const std::string &input);
    
    void displayBuffer() const;

    void clearBuffer();

    virtual void onEnter();
};

#endif