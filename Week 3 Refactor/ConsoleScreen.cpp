#include "ConsoleScreen.h"
#include <iostream>

void ConsoleScreen::addline(const std::string &input) {
    buffer.push_back(input);
}

void ConsoleScreen::displayBuffer() const {
    for (const std::string &s: buffer) {
        std::cout << s << std::endl;
    }
}

void ConsoleScreen::clearBuffer() {
    buffer.clear();
}

void ConsoleScreen::onEnter() {
    displayBuffer();
}

