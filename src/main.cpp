// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert


// Allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement(1);
}
#endif

// TODO: do we care about z world space?
// TODO: can we assume square?
// TODO: idea of and air frame or air space
// TODO: lift proportional to slice size?
// TODO: space in front of airfoil



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
#include "Program.h"



enum class SimModel { airfoil, f18, sphere };



static constexpr SimModel k_simModel(SimModel::airfoil);

static const std::string k_defResourceDir("resources");

static constexpr float k_minAngleOfAttack(-90.0f), k_maxAngleOfAttack(90.0f);
static constexpr float k_minRudderAngle(-90.0f), k_maxRudderAngle(90.0f);
static constexpr float k_minAileronAngle(-90.0f), k_maxAileronAngle(90.0f);
static constexpr float k_minElevatorAngle(-90.0f), k_maxElevatorAngle(90.0f);

static constexpr float k_angleOfAttackIncrement(1.0f); // the granularity of angle of attack
static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the rudder, elevator, and ailerons by when using arrow keys
static constexpr float k_autoAngleIncrement(7.0f); // how many degrees to change the angle of attack by when auto progressing



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

static std::shared_ptr<Program> s_texProg;
static uint s_screenVAO;
static uint s_screenVBO;



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

    std::cout << "Angle of attack set to " << s_angleOfAttack << std::endl;
}

static void changeRudderAngle(float deltaAngle) {
    s_rudderAngle = glm::clamp(s_rudderAngle + deltaAngle, -k_maxRudderAngle, k_maxRudderAngle);

    mat4 modelMat(glm::rotate(mat4(), s_rudderAngle, vec3(0.0f, 1.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
    s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);

    std::cout << "Rudder angle set to " << s_rudderAngle << std::endl;
}

static void changeAileronAngle(float deltaAngle) {
    s_aileronAngle = glm::clamp(s_aileronAngle + deltaAngle, -k_maxAileronAngle, k_maxAileronAngle);

    mat4 modelMat(glm::rotate(mat4(), s_aileronAngle, vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("AileronL01")->localTransform(modelMat, normalMat);

    modelMat = glm::rotate(mat4(), -s_aileronAngle, vec3(1.0f, 0.0f, 0.0f));
    normalMat = modelMat;
    s_model->subModel("AileronR01")->localTransform(modelMat, normalMat);

    std::cout << "Aileron angle set to " << s_aileronAngle << std::endl;
}

static void changeElevatorAngle(float deltaAngle) {
    s_elevatorAngle = glm::clamp(s_elevatorAngle + deltaAngle, -k_maxElevatorAngle, k_maxElevatorAngle);

    mat4 modelMat(glm::rotate(mat4(), s_elevatorAngle, vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
    s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);

    std::cout << "Elevator angle set to " << s_elevatorAngle << std::endl;
}

static void setSimulation(float angleOfAttack, bool debug) {
    mat4 modelMat;
    mat3 normalMat;
    float depth;
    vec3 centerOfGravity;

    switch (k_simModel) {
        case SimModel::airfoil:
            modelMat = glm::rotate(mat4(), glm::radians(-angleOfAttack), vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::scale(mat4(), vec3(0.875f, 1.0f, 1.0f)) * modelMat;
            normalMat = glm::transpose(glm::inverse(modelMat));
            depth = 1.0f;
            centerOfGravity = vec3(0.0f, 0.0f, depth * 0.5f);
            break;

        case SimModel::f18:
            modelMat = glm::scale(mat4(), vec3(0.10f));
            modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 0.0f, 1.0f)) * modelMat;
            modelMat = glm::rotate(mat4(1.0f), glm::radians(-angleOfAttack), vec3(1.0f, 0.0f, 0.0f)) * modelMat;
            modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -1.1f)) * modelMat;
            normalMat = glm::transpose(glm::inverse(modelMat));
            depth = 2.0f;
            centerOfGravity = vec3(0.0f, 0.0f, depth * 0.5f);
            break;

        case SimModel::sphere:
            modelMat = glm::rotate(mat4(), glm::radians(-angleOfAttack), vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::scale(mat4(), vec3(0.5f)) * modelMat;
            modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -0.5f)) * modelMat;
            normalMat = glm::transpose(glm::inverse(modelMat));
            depth = 1.0f;
            centerOfGravity = vec3(0.0f, 0.0f, depth * 0.5f);
            break;
    }

    Simulation::set(*s_model, modelMat, normalMat, depth, centerOfGravity, debug);
}

static void doFastSweep(float angleOfAttack) {
    setSimulation(angleOfAttack, false);

    double then(glfwGetTime());
    Simulation::sweep();
    double dt(glfwGetTime() - then);

    vec3 lift(Simulation::lift());
    vec3 drag(Simulation::drag());
    Results::submit(angleOfAttack, lift.y, drag.y);

    std::cout << "Angle: " << angleOfAttack << ", Lift: " << lift.y << ", Drag: " << drag.y << ", SPS: " << (1.0 / dt) << std::endl;
}

static void doAllAngles() {
    for (float angle(k_minAngleOfAttack); angle <= k_maxAngleOfAttack; angle += k_angleOfAttackIncrement) {
        doFastSweep(angle);
    }
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
    // If F is pressed, do fast sweep
    else if (key == GLFW_KEY_F && (action == GLFW_PRESS) && !mods) {
        if (Simulation::step() == 0 && !s_shouldAutoProgress) {
            doFastSweep(s_angleOfAttack);
        }
    }
    // If Shift-F is pressed, do fast sweep of all angles
    else if (key == GLFW_KEY_F && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
        if (Simulation::step() == 0 && !s_shouldAutoProgress) {
            doAllAngles();
        }
    }
    // If up arrow is pressed, increase angle of attack
    else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0 && !s_shouldAutoProgress) {
            changeAngleOfAttack(k_angleOfAttackIncrement);
        }
    }
    // If down arrow is pressed, decrease angle of attack
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::step() == 0 && !s_shouldAutoProgress) {
            changeAngleOfAttack(-k_angleOfAttackIncrement);
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

static bool setupSimDisplay() {
    // Setup display shader
    s_texProg = std::make_shared<Program>();
    s_texProg->setVerbose(true);
    s_texProg->setShaderNames(s_resourceDir + "/shaders/tex.vert", s_resourceDir + "/shaders/tex.frag");
    if (!s_texProg->init()) {
        std::cerr << "Failed to initialize texture shader" << std::endl;
        return false;
    }
    s_texProg->addUniform("u_tex");
    glUseProgram(s_texProg->pid);
    glUniform1i(s_texProg->getUniform("u_tex"), 0);
    glUseProgram(0);

    // Setup display geometry

    glGenBuffers(1, &s_screenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_screenVBO);

    vec2 vertData[]{
        { -1.0f, -1.0f },
        { 1.0f, -1.0f },
        { 1.0f,  1.0f },

        { 1.0f,  1.0f },
        { -1.0f,  1.0f },
        { -1.0f, -1.0f }
    };
    glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(vec2), vertData, GL_STATIC_DRAW);

    glGenVertexArrays(1, &s_screenVAO);
    glBindVertexArray(s_screenVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
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
    if (!(s_mainWindow = glfwCreateWindow(Simulation::size().x * 2, Simulation::size().y, "Simulation", nullptr, nullptr))) {
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

    if (!setupSimDisplay()) {
        std::cerr << "Failed to setup simulation display" << std::endl;
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
    //if (!SideView::setup(s_resourceDir, Simulation::sideTex(), s_mainWindow)) {
    //    std::cerr << "Failed to setup results" << std::endl;
    //    return false;
    //}
    //GLFWwindow * sideViewWindow(SideView::getWindow());

    // Arrange windows
    glfwSetWindowPos(s_mainWindow, 100, 100);
    int windowWidth, windowHeight;
    glfwGetWindowSize(s_mainWindow, &windowWidth, &windowHeight);
    //glfwSetWindowPos(sideViewWindow, 100 + windowWidth + 10, 100);
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

static void update() {
    if (s_shouldStep || s_shouldSweep) {
        if (Simulation::slice() == 0) { // About to do first slice
            setSimulation(s_angleOfAttack, true);
        }

        if (Simulation::step()) { // That was the last slice
            s_shouldSweep = false;
            vec3 lift(Simulation::lift());
            vec3 drag(Simulation::drag());
            std::cout << "angle: " << s_angleOfAttack << ", lift: " << lift.y << ", drag: " << drag.y << std::endl;

            Results::submit(s_angleOfAttack, lift.y, drag.y);

            if (s_shouldAutoProgress) {
                s_angleOfAttack += k_autoAngleIncrement;
                s_angleOfAttack = std::fmod(s_angleOfAttack - k_minAngleOfAttack, k_maxAngleOfAttack - k_minAngleOfAttack) + k_minAngleOfAttack;
                s_shouldSweep = true;
            }
        }

        s_shouldStep = false;
    }
}

static void renderDisplay() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    s_texProg->bind();
    glBindVertexArray(s_screenVAO);
    glActiveTexture(GL_TEXTURE0);

    // Front
    glViewport(0, 0, Simulation::size().x, Simulation::size().y);
    glBindTexture(GL_TEXTURE_2D, Simulation::frontTex());
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Side
    glViewport(Simulation::size().x, 0, Simulation::size().x, Simulation::size().y);
    glBindTexture(GL_TEXTURE_2D, Simulation::sideTex());
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    s_texProg->unbind();
}

static void render() {
    renderDisplay();
    glfwSwapBuffers(s_mainWindow);

    //SideView::render();
    //glfwMakeContextCurrent(s_mainWindow);

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
