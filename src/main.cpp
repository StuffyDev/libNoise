// src/main.cpp

#include <iostream>

extern int main_render_loop();

int main() {
    std::cout << "--- libNoise: Starting Graphical Demo ---\n";
    return main_render_loop();
}
