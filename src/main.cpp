// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert

// What was the point of using textures for the dense arrays rather than ssbo?
// Multiple geometry per air


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
#include "glm/gtc/constants.hpp"

#include "Simulation.hpp"
#include "Results.hpp"
#include "SideView.hpp"



static const std::string k_defResourceDir("C:\\liftdrag\\resources\\");
static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the angle of attack by when using arrow keys
static constexpr float k_autoAngleIncrement(7.0f); // how many degrees to change the angle of attack by when auto progressing
static constexpr float k_maxAngleOfAttack(90.0f);



static std::string s_resourceDir(k_defResourceDir);
static GLFWwindow * s_mainWindow;
static bool s_shouldStep(false);
static bool s_shouldSweep(true);
static bool s_shouldAutoProgress(false);
static bool s_shouldRender(true);
static float s_nextAngleOfAttack(0.0f); // in degrees
static bool s_angleOfAttackChanged;



static void printUsage() {
    std::cout << "Usage: liftdrag [resource_directory]" << std::endl;
}

static bool processArgs(int argc, char ** argv) {
    if (argc > 2) {
        printUsage();
        return false;
    }

    if (argc == 2) {
        s_resourceDir = argv[1];
    }

    return true;
}

static void errorCallback(int error, const char * description) {
    std::cerr << "GLFW error: " << description << std::endl;
}

static void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    // If space bar is pressed, do one slice
    if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        s_shouldStep = true;
        s_shouldSweep = false;
        s_shouldAutoProgress = false;
    }
    // If shift-space is pressed, do (or finish) entire sweep
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT)) {
        s_shouldSweep = true;
    }
    // If ctrl-space is pressed, will automatically move on to the next angle
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        s_shouldAutoProgress = !s_shouldAutoProgress;
    }
    // If tab is pressed, toggle rendering of simulation to screen
    else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        s_shouldRender = !s_shouldRender;
    }
    // If up arrow is pressed, increase angle of attack
    else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextAngleOfAttack + k_manualAngleIncrement);
            if (nextAngle > k_maxAngleOfAttack) nextAngle = k_maxAngleOfAttack;
            if (nextAngle != s_nextAngleOfAttack) {
                s_nextAngleOfAttack = nextAngle;
                s_angleOfAttackChanged = true;
            }
        }
    }
    // If down arrow is pressed, decrease angle of attack
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextAngleOfAttack - k_manualAngleIncrement);
            if (nextAngle < -k_maxAngleOfAttack) nextAngle = -k_maxAngleOfAttack;
            if (nextAngle != s_nextAngleOfAttack) {
                s_nextAngleOfAttack = nextAngle;
                s_angleOfAttackChanged = true;
            }
        }
    }
}

static bool setup() {
    // Setup GLFW
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    if (!(s_mainWindow = glfwCreateWindow(Simulation::getSize().x, Simulation::getSize().y, "Simulation", nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    glfwDefaultWindowHints();

    glfwMakeContextCurrent(s_mainWindow);

    glfwSetKeyCallback(s_mainWindow, keyCallback);
    glfwSwapInterval(0);

    // Setup GLAD
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Setup simulation
    if (!Simulation::setup(s_resourceDir)) {
        std::cerr << "Failed to setup simulation" << std::endl;
        return false;
    }

    // Results Window
    if (!Results::setup(s_resourceDir, s_mainWindow)) {
        std::cerr << "Failed to setup results" << std::endl;
        return false;
    }
    GLFWwindow * resultsWindow(Results::getWindow());

    //Side view Window
    if (!SideView::setup(s_resourceDir, Simulation::getSideTextureID(), s_mainWindow)) {
        std::cerr << "Failed to setup results" << std::endl;
        return false;
    }
    GLFWwindow * sideViewWindow(SideView::getWindow());

    // Arrange windows
    glfwSetWindowPos(s_mainWindow, 100, 100);
    int windowWidth, windowHeight;
    glfwGetWindowSize(s_mainWindow, &windowWidth, &windowHeight);
    glfwSetWindowPos(sideViewWindow, 100 + windowWidth + 10, 100);
    glfwSetWindowPos(resultsWindow, 100, 100 + windowHeight + 40);

    glfwMakeContextCurrent(s_mainWindow);
    glfwFocusWindow(s_mainWindow);

    return true;
}

void cleanup() {
    Simulation::cleanup();

    Results::cleanup();

    glfwTerminate();
}



int main(int argc, char ** argv) {
    if (!processArgs(argc, argv)) {
        std::exit(-1);
    }

    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        return -1;
    }

    int fps(0);
    double then(glfwGetTime());
    
    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(s_mainWindow)) {
        // Poll for and process events.
        glfwPollEvents();

        // Simulation ----------------------------------------------------------

        if (Simulation::getSlice() == 0) { // Prior to starting sweep
            if (s_angleOfAttackChanged) {
                Simulation::setAngleOfAttack(s_nextAngleOfAttack);
                s_angleOfAttackChanged = false;
                std::cout << "angle set to " << s_nextAngleOfAttack << " degrees" << std::endl;
            }
        }
        if (s_shouldStep || s_shouldSweep) {
            if (Simulation::step()) {
                // That was the last slice
                s_shouldSweep = false;
                float angle(Simulation::getAngleOfAttack());
                vec3 lift(Simulation::getLift());
                std::cout << "angle: " << angle << ", lift: " << lift.x << std::endl;
                Results::submit(Simulation::getAngleOfAttack(), lift.x, 0.0f);

                if (s_shouldAutoProgress) {
                    angle += k_autoAngleIncrement;
                    angle = (std::fmod((angle + 90.0f), 180.f)) - 90.0f;
                    if (angle > k_maxAngleOfAttack) angle = -k_maxAngleOfAttack;
                    Simulation::setAngleOfAttack(angle);
                    s_shouldSweep = true;
                }
            }
            //sideview::submitOutline(Simulation::getSlice(), Simulation::getSideviewOutline());
            s_shouldStep = false;
        }

        if (s_shouldRender) {
            Simulation::render();
            glfwSwapBuffers(s_mainWindow);

            SideView::render();
            glfwMakeContextCurrent(s_mainWindow);
        }

        // Results -------------------------------------------------------------

        Results::render();
        glfwMakeContextCurrent(s_mainWindow);

        ++fps;
        double now(glfwGetTime());
        if (now - then >= 1.0) {
            //std::cout << "fps: " << fps << std::endl;
            fps = 0;
            then = now;
        }
    }

    cleanup();

    return 0;
}
