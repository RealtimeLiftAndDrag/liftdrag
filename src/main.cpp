// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert

// TODO: Multiple geometry per air
// TODO: air mergers
// TODO: do we care about exact world position? could save a texture that way
// TODO: can save normals as four 16 bit normalized ints
// TODO: improve lift and drag accumulation


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
static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the angle of attack, rudder, elevator, and ailerons by when using arrow keys

static constexpr float k_autoAngleIncrement(7.0f); // how many degrees to change the angle of attack by when auto progressing

static constexpr float k_maxAngleOfAttack(90.0f);
static constexpr float k_maxRudderAngle(90.0f);
static constexpr float k_maxAileronAngle(90.0f);
static constexpr float k_maxElevatorAngle(90.0f);



static std::string s_resourceDir(k_defResourceDir);
static GLFWwindow * s_mainWindow;
static bool s_shouldStep(false);
static bool s_shouldSweep(true);
static bool s_shouldAutoProgress(false);
static bool s_shouldRender(true);


static bool s_angleOfAttackChanged;
static bool s_aileronAngleChanged;
static bool s_rudderAngleChanged;
static bool s_elevatorAngleChanged;

//all in degrees
static float s_nextAngleOfAttack(0.0f);
static float s_nextAileronAngle(0.0f);
static float s_nextRudderAngle(0.0f);
static float s_nextElevatorAngle(0.0f);






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
    // If O key is pressed, increase rudder angle
    else if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextRudderAngle + k_manualAngleIncrement);
            if (abs(nextAngle) < k_maxRudderAngle) {
                s_nextRudderAngle = nextAngle;
                s_rudderAngleChanged = true;
            }
        }
    }
    // If I key is pressed, decrease rudder angle
    else if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextRudderAngle - k_manualAngleIncrement);
            if (abs(nextAngle) < k_maxRudderAngle) {
                s_nextRudderAngle = nextAngle;
                s_rudderAngleChanged = true;
            }
        }
    }
    // If K key is pressed, increase elevator angle
    else if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextElevatorAngle + k_manualAngleIncrement);
            if (abs(nextAngle) < k_maxElevatorAngle) {
                s_nextElevatorAngle = nextAngle;
                s_elevatorAngleChanged = true;
            }
        }
    }
    // If J key is pressed, decrease elevator angle
    else if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextElevatorAngle - k_manualAngleIncrement);
            if (abs(nextAngle) < k_maxElevatorAngle) {
                s_nextElevatorAngle = nextAngle;
                s_elevatorAngleChanged = true;
            }
        }
    }
    // If M key is pressed, increase aileron angle
    else if (key == GLFW_KEY_M && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextAileronAngle + k_manualAngleIncrement);
            if (abs(nextAngle) < k_maxRudderAngle) {
                s_nextAileronAngle = nextAngle;
                s_aileronAngleChanged = true;
            }
        }
    }
    // If N key is pressed, decrease aileron angle
    else if (key == GLFW_KEY_N && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (!s_shouldAutoProgress) {
            float nextAngle(s_nextAileronAngle - k_manualAngleIncrement);
            if (abs(nextAngle) < k_maxRudderAngle) {
                s_nextAileronAngle = nextAngle;
                s_aileronAngleChanged = true;
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
                std::cout << "angle of attack set to " << s_nextAngleOfAttack << " degrees" << std::endl;
            }
            if (s_rudderAngleChanged) {
                Simulation::setRudderAngle(s_nextRudderAngle);
                s_rudderAngleChanged = false;
                std::cout << "rudder angle set to " << s_nextRudderAngle << " degrees" << std::endl;
            }
            if (s_elevatorAngleChanged) {
                Simulation::setElevatorAngle(s_nextElevatorAngle);
                s_elevatorAngleChanged = false;
                std::cout << "elevator/stabiliser angle set to " << s_nextElevatorAngle << " degrees" << std::endl;
            }
            if (s_aileronAngleChanged) {
                Simulation::setAileronAngle(s_nextAileronAngle);
                s_aileronAngleChanged = false;
                std::cout << "aileron angle set to " << s_nextAileronAngle << " degrees" << std::endl;
            }
        }
        if (s_shouldStep || s_shouldSweep) {
            if (Simulation::step()) {
                // That was the last slice
                s_shouldSweep = false;
                float angleOfAttack(Simulation::getAngleOfAttack());
                vec3 lift(Simulation::getLift());
                vec3 drag(Simulation::getDrag());
                std::cout << "angle: " << angleOfAttack << ", lift: " << lift.x << ", drag: " << drag.x << std::endl;
                printf("Animation Angles:\n\tRudder: %f\n\tElevator: %f\n\tAileron: %f\n",
                    Simulation::getRudderAngle(),
                    Simulation::getElevatorAngle(),
                    Simulation::getAileronAngle()
                );

                Results::submit(Simulation::getAngleOfAttack(), lift.x, drag.x);

                if (s_shouldAutoProgress) {
                    angleOfAttack += k_autoAngleIncrement;
                    angleOfAttack = (std::fmod((angleOfAttack + 90.0f), 180.f)) - 90.0f;
                    if (angleOfAttack > k_maxAngleOfAttack) angleOfAttack = -k_maxAngleOfAttack;
                    Simulation::setAngleOfAttack(angleOfAttack);
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
