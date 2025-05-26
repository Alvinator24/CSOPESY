#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include <vector>
#include "ConsoleScreen.h"

class ConsoleManager {
private:
    std::vector<std::unique_ptr<ConsoleScreen>> screens;
    int currentScreenIndex = -1;

public:
    ConsoleManager();
    void addScreen(std::unique_ptr<ConsoleScreen> screen);
    bool setCurrentScreen(int index);

    ConsoleScreen* getCurrentScreen() const;
};



#endif