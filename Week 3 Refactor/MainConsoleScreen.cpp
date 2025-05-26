#include "MainConsoleScreen.h"


MainConsoleScreen::MainConsoleScreen() {
    pushHeader();
}

void ::MainConsoleScreen::pushHeader() {
    buffer.emplace_back(
        "  _____  _____  ____  _____  ______  _______     __\n"
        " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\   / /\n"
        "| |    | (___ | |  | | |__) | |__  | (___  \\ \\_/ / \n"
        "| |     \\___ \\| |  | |  ___/|  __|  \\___ \\  \\   /  \n"
        "| |____ ____) | |__| | |    | |____ ____) |  | |   \n"
        " \\_____|_____/ \\____/|_|    |______|_____/   |_|   \n"
        "Welcome to the Command-Line Operating System!\n"
        "Type 'exit' to quit, 'clear' to clear the screen, 'help' for commands.\n"
    );
}
