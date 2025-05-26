#include "ConsoleManager.h"


ConsoleManager::ConsoleManager() = default; //idk yet. create the default one ig?

ConsoleScreen *ConsoleManager::getCurrentScreen() const {
    if (currentScreenIndex >= 0 && currentScreenIndex < screens.size()) {
        return screens[currentScreenIndex].get();
    }
    return nullptr;
}

bool ConsoleManager::setCurrentScreen(int index) {
    if (index >= 0 && index < screens.size()) {
        currentScreenIndex = index;
        return true;
    } else {
        return false;
    }
}

void ConsoleManager::addScreen(std::unique_ptr<ConsoleScreen> screen) {
    if (screen) {
        screens.push_back(std::move(screen));

        if (currentScreenIndex == -1) {
            currentScreenIndex = screens.size() - 1;
        }

    } else {
        std::cerr << "Warning: Attempted to add a null screen to ConsoleManager!" << std::endl;
    }
}
