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
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/constants.hpp"


#include "Common/Global.hpp"

#include "RLD/Simulation.hpp"
#include "Common/Model.hpp"
#include "Common/Program.h"
#include "Common/UI.hpp"


#include "SimObject.hpp"

class MainUIC : public UI::VerticalGroup {

public:

    virtual void keyEvent(int key, int action, int mods) override;

};

static const ivec2 k_windowSize(1280, 720);
static const std::string k_windowTitle("RLD Flight Simulator");
static const std::string k_resourceDir("../resources");
static unq<Model> s_model;
static unq<SimObject> s_simObject;
static shr<Program> s_phongProg;

static mat4 s_modelMat;
static mat3 s_normalMat;
static float s_momentOfInertia;
static float s_windframeWidth, s_windframeDepth;
static float s_windSpeed;

//all in degrees
static float s_aileronAngle(0.0f);
static float s_rudderAngle(0.0f);
static float s_elevatorAngle(0.0f);

static constexpr float k_minRudderAngle(-90.0f), k_maxRudderAngle(90.0f);
static constexpr float k_minAileronAngle(-90.0f), k_maxAileronAngle(90.0f);
static constexpr float k_minElevatorAngle(-90.0f), k_maxElevatorAngle(90.0f);

static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the rudder, elevator, and ailerons by when using arrow keys


static GLFWwindow * s_window;



static void errorCallback(int error, const char * description) {
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

static void changeRudderAngle(float deltaAngle) {
    s_rudderAngle = glm::clamp(s_rudderAngle + deltaAngle, -k_maxRudderAngle, k_maxRudderAngle);
    std::cout << "rudder angle: " << s_rudderAngle << std::endl;
    mat4 modelMat(glm::rotate(mat4(), glm::radians(s_rudderAngle), vec3(0.0f, 1.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
    s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);

}

static void changeAileronAngle(float deltaAngle) {
    s_aileronAngle = glm::clamp(s_aileronAngle + deltaAngle, -k_maxAileronAngle, k_maxAileronAngle);

    mat4 modelMat(glm::rotate(mat4(), glm::radians(s_aileronAngle), vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("AileronL01")->localTransform(modelMat, normalMat);

    modelMat = glm::rotate(mat4(), glm::radians(-s_aileronAngle), vec3(1.0f, 0.0f, 0.0f));
    normalMat = modelMat;
    s_model->subModel("AileronR01")->localTransform(modelMat, normalMat);

}

static void changeElevatorAngle(float deltaAngle) {
    s_elevatorAngle = glm::clamp(s_elevatorAngle + deltaAngle, -k_maxElevatorAngle, k_maxElevatorAngle);

    mat4 modelMat(glm::rotate(mat4(), glm::radians(s_elevatorAngle), vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
    s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);

}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // If A key is pressed, increase rudder angle (i.e. turn left)
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        changeRudderAngle(k_manualAngleIncrement);
    }
    // If D key is pressed, decrease rudder angle (i.e. turn right)
    else if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        changeRudderAngle(-k_manualAngleIncrement);
    }
    // If S key is pressed, increase elevator angle (i.e. decrease angle of attack)
    else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        changeElevatorAngle(k_manualAngleIncrement);
    }
    // If W key is pressed, decrease elevator angle (i.e. increase angle of attack)
    else if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        changeElevatorAngle(-k_manualAngleIncrement);
    }
    // If E key is pressed, increase aileron angle (i.e. roll cw)
    else if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        changeAileronAngle(k_manualAngleIncrement);
    }
    // If Q key is pressed, decrease aileron angle (i.e. roll ccw)
    else if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        changeAileronAngle(-k_manualAngleIncrement);
    }
}


static bool setupModel(const std::string &resourceDir) {
    s_model = Model::load(resourceDir + "/models/f18.grl");
    if (!s_model) {
        std::cerr << "Failed to load model" << std::endl;
        return false;
    }

    s_modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 0.0f, 1.0f));
    s_momentOfInertia = 1.0f;
    s_windframeWidth = 14.5f;
    s_windframeDepth = 22.0f;
    s_windSpeed = 10.0f;

    s_normalMat = glm::transpose(glm::inverse(s_modelMat));

    //setup simObject
    s_simObject = std::make_unique<SimObject>();
    if (!s_simObject) {
        std::cerr << "Failed to create sim object" << std::endl;
        return false;
    }
    s_simObject->setMass(16769); //gross weight in kg pulled from wiki
    s_simObject->setGravityOn(false);

    return true;
}

static bool setupShader(const std::string & resourcesDir) {
    std::string shadersDir(resourcesDir + "/shaders");

    // Foil Shader
    s_phongProg = std::make_shared<Program>();
    s_phongProg->setVerbose(true);
    s_phongProg->setShaderNames(shadersDir + "/phong.vert", shadersDir + "/phong.frag");
    if (!s_phongProg->init()) {
        std::cerr << "Failed to initialize foil shader" << std::endl;
        return false;
    }
    s_phongProg->addUniform("u_projMat");
    s_phongProg->addUniform("u_viewMat");
    s_phongProg->addUniform("u_modelMat");
    s_phongProg->addUniform("u_normalMat");

    return true;
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
    glfwSetKeyCallback(s_window, keyCallback);


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

    //setup model for sim
    if (!setupModel(k_resourceDir)) {
        std::cerr << "Failed to load model for Sim" << std::endl;
        return false;
    }

    //load shader for sim
    if (!setupShader(k_resourceDir)) {
        std::cerr << "Failed to load shader for Sim" << std::endl;
        return false;
    }

    return true;
}

static void cleanup() {
    rld::cleanup();
    glfwTerminate();
}

static mat4 getPerspectiveMatrix() {
    float fov(3.14159f / 4.0f);
    float aspect;
    if (k_windowSize.x < k_windowSize.y) {
        aspect = float(k_windowSize.y) / float(k_windowSize.x);
    }
    else {
        aspect = float(k_windowSize.x) / float(k_windowSize.y);
    }

    return glm::perspective(fov, aspect, 0.01f, 1000.f);
}

static mat4 getViewMatrix() {
    vec3 camPos = vec3(0, 4, -17.5);
    vec3 lookPos = s_simObject->pos;
    vec3 viewVec = lookPos - camPos;
    vec3 right = glm::cross(viewVec, vec3(0, 1, 0));
    vec3 up = glm::cross(right, viewVec);
    return glm::lookAt(
        camPos,
        lookPos,
        up
    );
}

static void render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    s_simObject->update();
    //s_simObject->pos = vec3(0, 5, 0);

    mat4 modelMat, normalMat, simTransformMat;
    mat4 projMat, viewMat;

    modelMat = s_modelMat;
    normalMat = s_normalMat;

    simTransformMat = s_simObject->getTransform();

    modelMat = simTransformMat * modelMat;

    projMat = getPerspectiveMatrix();
    viewMat = getViewMatrix();

    s_phongProg->bind();
    glUniformMatrix4fv(s_phongProg->getUniform("u_projMat"), 1, GL_FALSE, reinterpret_cast<const float *>(&projMat));
    glUniformMatrix4fv(s_phongProg->getUniform("u_viewMat"), 1, GL_FALSE, reinterpret_cast<const float *>(&viewMat));
    s_model->draw(modelMat, normalMat, s_phongProg->getUniform("u_modelMat"), s_phongProg->getUniform("u_normalMat"));

    s_phongProg->unbind();

    //reset gl variables set to not mess up rld sim
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glDisable(GL_DEPTH_TEST);

}



int main(int argc, char ** argv) {
    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(s_window)) {
        glfwPollEvents();

        // TODO
        render();
        glfwSwapBuffers(s_window);
    }

    cleanup();

    return EXIT_SUCCESS;
}
