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

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include "Simulation.hpp"
#include "Results.hpp"
#include "SideView.hpp"



static const std::string k_defResourceDir("../resources");
static constexpr float k_angleIncrement(1.0f); // how many degrees to change the angle of attack by
static constexpr float k_maxAngleOfAttack(90.0f);



static std::string f_resourceDir(k_defResourceDir);
static GLFWwindow * f_mainWindow;
static bool f_shouldStep(false);
static bool f_shouldSweep(false);
static bool f_shouldAutoProgress(false);
static bool f_shouldRender(true);
static float f_nextAngleOfAttack(0.0f); // in degrees
static bool f_angleOfAttackChanged;



static void printUsage() {
    std::cout << "Usage: liftdrag [resource_directory]" << std::endl;
}

static bool processArgs(int argc, char ** argv) {
    if (argc > 2) {
        printUsage();
        return false;
    }

    if (argc == 2) {
        f_resourceDir = argv[1];
    }

    return true;
}

void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    // If space bar is pressed, do one slice
    if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        f_shouldStep = true;
        f_shouldSweep = false;
    }
    // If shift-space is pressed, do (or finish) entire sweep
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT)) {
        f_shouldSweep = true;
    }
    // If ctrl-space is pressed, will automatically move on to the next angle
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        f_shouldAutoProgress = !f_shouldAutoProgress;
    }
    // If tab is pressed, toggle rendering of simulation to screen
    else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        f_shouldRender = !f_shouldRender;
    }
    // If up arrow is pressed, increase angle of attack
    else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!f_shouldAutoProgress) {
            float nextAngle(f_nextAngleOfAttack + k_angleIncrement);
            if (nextAngle > k_maxAngleOfAttack) nextAngle = k_maxAngleOfAttack;
            if (nextAngle != f_nextAngleOfAttack) {
                f_nextAngleOfAttack = nextAngle;
                f_angleOfAttackChanged = true;
            }
        }
    }
    // If down arrow is pressed, decrease angle of attack
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!f_shouldAutoProgress) {
            float nextAngle(f_nextAngleOfAttack - k_angleIncrement);
            if (nextAngle < -k_maxAngleOfAttack) nextAngle = -k_maxAngleOfAttack;
            if (nextAngle != f_nextAngleOfAttack) {
                f_nextAngleOfAttack = nextAngle;
                f_angleOfAttackChanged = true;
            }
        }
    }
}




int main(int argc, char ** argv) {
    if (!processArgs(argc, argv)) {
        std::exit(-1);
    }

    // Setup simulation
    if (!simulation::setup(f_resourceDir)) {
        std::cerr << "Failed to setup simulation" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    f_mainWindow = simulation::getWindow();
    glfwSetKeyCallback(f_mainWindow, keyCallback);
    glfwSwapInterval(0);
    
    // Results Window
    if (!results::setup(f_resourceDir, simulation::getWindow())) {
        std::cerr << "Failed to setup results" << std::endl;
        std::exit(EXIT_FAILURE);
    }

	//Side view Window
	if (!sideview::setup(f_resourceDir, simulation::getWindow())) {
		std::cerr << "Failed to setup results" << std::endl;
		std::exit(EXIT_FAILURE);
	}

    glfwMakeContextCurrent(f_mainWindow);
    glfwFocusWindow(f_mainWindow);

    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(f_mainWindow)) {

        // Simulation ----------------------------------------------------------

        if (simulation::getSlice() == 0) { // Prior to starting sweep
            if (f_angleOfAttackChanged) {
                simulation::setAngleOfAttack(f_nextAngleOfAttack);
                f_angleOfAttackChanged = false;
                std::cout << "angle set to " << f_nextAngleOfAttack << " degrees" << std::endl;
            }
        }
        if (f_shouldStep || f_shouldSweep) {
            if (simulation::step()) {
                // That was the last slice
                f_shouldSweep = false;
                float angle(simulation::getAngleOfAttack());
                glm::vec3 lift(simulation::getLift());
                std::cout << "angle: " << angle << ", lift: " << lift.x << std::endl;
                results::submit(simulation::getAngleOfAttack(), lift.x, 0.0f);

                if (f_shouldAutoProgress) {
                    angle += k_angleIncrement;
                    if (angle > k_maxAngleOfAttack) angle = -k_maxAngleOfAttack;
                    simulation::setAngleOfAttack(angle);
                    f_shouldSweep = true;
                }
            }
            f_shouldStep = false;
        }

        if (f_shouldRender) {
            simulation::render();
			//sideview::submitOutline(simulation::getSlice(), simulation::getSideviewOutline());
        }

        // Results -------------------------------------------------------------

        results::render();
        glfwMakeContextCurrent(f_mainWindow);

        // Poll for and process events.
        glfwPollEvents();
    }

    // Quit program.
    simulation::cleanup();
    results::cleanup();

    return EXIT_SUCCESS;
}
