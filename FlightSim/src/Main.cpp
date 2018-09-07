// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert



// Allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement(1);
}
#endif



#include <iostream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "Common/Global.hpp"

#include "RLD/Simulation.hpp"



static const ivec2 k_windowSize(1280, 720);
static const std::string k_windowTitle("RLD Flight Simulator");
static const std::string k_resourceDir("../resources");

static GLFWwindow * s_window;



static void errorCallback(int error, const char * description) {
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

static bool setup() {
    // Setup window
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    if (!(s_window = glfwCreateWindow(k_windowSize.x, k_windowSize.y, k_windowTitle.c_str(), nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(s_window);
    glfwSwapInterval(1); // VSync on or off

    // Setup GLAD
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Setup simulation
    if (!rld::setup(k_resourceDir)) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }

    return true;
}

static void cleanup() {
    rld::cleanup();
    glfwTerminate();
}



int main(int argc, char ** argv) {
    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        return EXIT_FAILURE;
    }
    
    while (!glfwWindowShouldClose(s_window)) {
        glfwPollEvents();

        // TODO

        glfwSwapBuffers(s_window);
    }

    cleanup();

    return EXIT_SUCCESS;
}
