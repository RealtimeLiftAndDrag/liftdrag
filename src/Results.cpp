#include "Results.hpp"

#include <iostream>

#include "GLFW/glfw3.h"



namespace results {



namespace {



constexpr int k_defWindowWidth(400), k_defWindowHeight(300);

GLFWwindow * window;



}



bool setup() {
    if (!(window = glfwCreateWindow(k_defWindowWidth, k_defWindowHeight, "Results", nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    return true;
}

void cleanup() {
    if (window) {
        glfwDestroyWindow(window);
    }
}

void render() {
    glfwMakeContextCurrent(window);

    // render

    glfwSwapBuffers(window);
}



}