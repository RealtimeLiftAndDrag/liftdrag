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
#include "glm/gtc/matrix_transform.hpp"

#include "Simulation.hpp"
#include "Results.hpp"
#include "SideView.hpp"
#include "Model.hpp"



enum class SimModel { airfoil, f18, sphere };



static constexpr SimModel k_simModel(SimModel::f18);

static const std::string k_defResourceDir("resources");

static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the angle of attack, rudder, elevator, and ailerons by when using arrow keys

static constexpr float k_autoAngleIncrement(7.0f); // how many degrees to change the angle of attack by when auto progressing

static constexpr float k_minAngleOfAttack(-90.0f), k_maxAngleOfAttack(90.0f);
static constexpr float k_minRudderAngle(-90.0f), k_maxRudderAngle(90.0f);
static constexpr float k_minAileronAngle(-90.0f), k_maxAileronAngle(90.0f);
static constexpr float k_minElevatorAngle(-90.0f), k_maxElevatorAngle(90.0f);



static std::string s_resourceDir(k_defResourceDir);

static std::unique_ptr<Model> s_model;
//all in degrees
static float s_angleOfAttack(0.0f);
static float s_aileronAngle(0.0f);
static float s_rudderAngle(0.0f);
static float s_elevatorAngle(0.0f);

static GLFWwindow * s_mainWindow;
static bool s_shouldStep(false);
static bool s_shouldSweep(true);
static bool s_shouldAutoProgress(false);
static bool s_shouldRender(true);



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

static void changeAngleOfAttack(float deltaAngle) {
    s_angleOfAttack = glm::clamp(s_angleOfAttack + deltaAngle, -k_maxAngleOfAttack, k_maxAngleOfAttack);

    std::cout << "Angle of attack set to " << s_angleOfAttack << "\u00B0" << std::endl;
}

static void changeRudderAngle(float deltaAngle) {
    s_rudderAngle = glm::clamp(s_rudderAngle + deltaAngle, -k_maxRudderAngle, k_maxRudderAngle);

    mat4 modelMat(glm::rotate(mat4(), s_rudderAngle, vec3(0.0f, 1.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
    s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);

    std::cout << "Rudder angle set to " << s_rudderAngle << "\u00B0" << std::endl;
}

static void changeAileronAngle(float deltaAngle) {
    s_aileronAngle = glm::clamp(s_aileronAngle + deltaAngle, -k_maxAileronAngle, k_maxAileronAngle);

    mat4 modelMat(glm::rotate(mat4(), s_aileronAngle, vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("AileronL01")->localTransform(modelMat, normalMat);

    modelMat = glm::rotate(mat4(), -s_aileronAngle, vec3(1.0f, 0.0f, 0.0f));
    normalMat = modelMat;
    s_model->subModel("AileronR01")->localTransform(modelMat, normalMat);

    std::cout << "Aileron angle set to " << s_aileronAngle << "\u00B0" << std::endl;
}

static void changeElevatorAngle(float deltaAngle) {
    s_elevatorAngle = glm::clamp(s_elevatorAngle + deltaAngle, -k_maxElevatorAngle, k_maxElevatorAngle);

    mat4 modelMat(glm::rotate(mat4(), s_elevatorAngle, vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
    s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);

    std::cout << "Elevator angle set to " << s_elevatorAngle << "\u00B0" << std::endl;

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
        if (Simulation::step() == 0 && !s_shouldAutoProgress) {
            changeAngleOfAttack(k_manualAngleIncrement);
        }
    }
    // If down arrow is pressed, decrease angle of attack
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0 && !s_shouldAutoProgress) {
            changeAngleOfAttack(-k_manualAngleIncrement);
        }
    }
    // If O key is pressed, increase rudder angle
    else if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0) {
            changeRudderAngle(k_manualAngleIncrement);
        }
    }
    // If I key is pressed, decrease rudder angle
    else if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0) {
            changeRudderAngle(-k_manualAngleIncrement);
        }
    }
    // If K key is pressed, increase elevator angle
    else if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0) {
            changeElevatorAngle(k_manualAngleIncrement);
        }
    }
    // If J key is pressed, decrease elevator angle
    else if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0) {
            changeElevatorAngle(-k_manualAngleIncrement);
        }
    }
    // If M key is pressed, increase aileron angle
    else if (key == GLFW_KEY_M && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0) {
            changeAileronAngle(k_manualAngleIncrement);
        }
    }
    // If N key is pressed, decrease aileron angle
    else if (key == GLFW_KEY_N && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0) {
            changeAileronAngle(-k_manualAngleIncrement);
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

    // Setup model    
    switch (k_simModel) {
        case SimModel::airfoil: s_model = Model::load(s_resourceDir + "/models/0012.obj"  ); break;
        case SimModel::    f18: s_model = Model::load(s_resourceDir + "/models/f18.grl"   ); break;
        case SimModel:: sphere: s_model = Model::load(s_resourceDir + "/models/sphere.obj"); break;
    }
    if (!s_model) {
        std::cerr << "Failed to load model" << std::endl;
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

static void cleanup() {
    Simulation::cleanup();
    Results::cleanup();
    glfwTerminate();
}

static void setSimulation() {
    mat4 modelMat;
    mat3 normalMat;
    float depth;
    vec3 centerOfGravity;

    switch (k_simModel) {
        case SimModel::airfoil:
            modelMat = glm::rotate(mat4(), glm::radians(-s_angleOfAttack), vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::scale(mat4(), vec3(0.875f, 1.0f, 1.0f)) * modelMat;
            normalMat = glm::transpose(glm::inverse(modelMat));
            depth = 1.0f;
            centerOfGravity = vec3(0.0f, 0.0f, depth * 0.5f);
            break;

        case SimModel::f18:
            modelMat = glm::scale(mat4(), vec3(0.10f));
            modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 0.0f, 1.0f)) * modelMat;
            modelMat = glm::rotate(mat4(1.0f), glm::radians(-s_angleOfAttack), vec3(1.0f, 0.0f, 0.0f)) * modelMat;
            modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -1.1f)) * modelMat;
            normalMat = glm::transpose(glm::inverse(modelMat));
            depth = 2.0f;
            centerOfGravity = vec3(0.0f, 0.0f, depth * 0.5f);
            break;

        case SimModel::sphere:
            modelMat = glm::rotate(mat4(), glm::radians(-s_angleOfAttack), vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::scale(mat4(), vec3(0.5f)) * modelMat;
            modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -0.5f)) * modelMat;
            normalMat = glm::transpose(glm::inverse(modelMat));
            depth = 1.0f;
            centerOfGravity = vec3(0.0f, 0.0f, depth * 0.5f);
            break;
    }

    Simulation::set(*s_model, modelMat, normalMat, depth, centerOfGravity);
}

static void update() {
    if (s_shouldStep || s_shouldSweep) {
        if (Simulation::getSlice() == 0) { // About to do first slice
            setSimulation();
        }

        if (Simulation::step()) { // That was the last slice
            s_shouldSweep = false;
            vec3 lift(Simulation::getLift());
            vec3 drag(Simulation::getDrag());
            std::cout << "angle: " << s_angleOfAttack << ", lift: " << lift.x << ", drag: " << drag.x << std::endl;

            Results::submit(s_angleOfAttack, lift.x, drag.x);

            if (s_shouldAutoProgress) {
                s_angleOfAttack += k_autoAngleIncrement;
                s_angleOfAttack = std::fmod(s_angleOfAttack - k_minAngleOfAttack, k_maxAngleOfAttack - k_minAngleOfAttack) + k_minAngleOfAttack;
                s_shouldSweep = true;
            }
        }

        s_shouldStep = false;
    }
}

static void render() {
    if (s_shouldRender) {
        Simulation::render();
        glfwSwapBuffers(s_mainWindow);

        SideView::render();
        glfwMakeContextCurrent(s_mainWindow);
    }

    // Results -------------------------------------------------------------

    Results::render();
    glfwMakeContextCurrent(s_mainWindow);
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

        update();

        render();

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
