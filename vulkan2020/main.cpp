/*
 *
 * Andrew Frost
 * main.cpp
 * 2020
 *
 */

#include "renderbackend.h"

int main(int argc, char* argv[]) {
    RenderBackend app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}