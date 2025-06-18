#include <iostream>

extern "C" void function_from_CPP() {
    std::cout << "C++ function" << std::endl;
}