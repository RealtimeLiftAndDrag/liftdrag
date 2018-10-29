﻿// Realtime Lift and Drag SURP 2018
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
#include "glm/gtx/string_cast.hpp"

#include "Common/Global.hpp"

#include "RLD/Simulation.hpp"
#include "Common/Model.hpp"
#include "Common/Shader.hpp"
#include "UI/Group.hpp"

#include "SimObject.hpp"
#include "ProgTerrain.hpp"

#include "xboxcontroller.h"



CXBOXController XBOXController(1);

class MainUIC : public ui::VerticalGroup {

public:

    virtual void keyEvent(int key, int action, int mods) override;

};

static const bool k_windDebug(false);
static const ivec2 k_windowSize(1280, 1280);
static const std::string k_windowTitle("RLD Flight Simulator");

static constexpr int k_simTexSize = 1024;
static constexpr int k_simSliceCount = 100;
static constexpr float k_simLiftC = 0.2f;
static constexpr float k_simDragC = 0.1f;

static const float k_timeScale(1.0f);
static const float k_thrust(0.5f);
static const float k_thrustIncrease(0.1f);

static unq<Model> s_model;
static unq<SimObject> s_simObject;
static unq<Shader> s_phongProg;

static mat4 s_modelMat;
static mat3 s_normalMat;
static float s_momentOfInertia;
static float s_windframeWidth, s_windframeDepth;
static float s_windSpeed;

// all in degrees
static float s_aileronAngle(0.0f);
static float s_rudderAngle(0.0f);
static float s_elevatorAngle(0.0f);
static bool s_increaseThrust(false);

static constexpr float k_maxRudderAngle(30.0f);
static constexpr float k_maxAileronAngle(30.0f);
static constexpr float k_maxElevatorAngle(30.0f);

static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the rudder, elevator, and ailerons by when using arrow keys


static GLFWwindow * s_window;



static void errorCallback(int error, const char * description) {
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

static void setRudderAngle(float angle) {
    angle = glm::clamp(angle, -k_maxRudderAngle, k_maxRudderAngle);
    if (angle != s_rudderAngle) {
        s_rudderAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(s_rudderAngle), vec3(0.0f, 1.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
        s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);
    }
}

static void changeRudderAngle(float deltaAngle) {
    setRudderAngle(s_rudderAngle + deltaAngle);
}

static void setAileronAngle(float angle) {
    angle = glm::clamp(angle, -k_maxAileronAngle, k_maxAileronAngle);
    if (angle != s_aileronAngle) {
        s_aileronAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(s_aileronAngle), vec3(1.0f, 0.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("AileronL01")->localTransform(modelMat, normalMat);

        modelMat = glm::rotate(mat4(), glm::radians(-s_aileronAngle), vec3(1.0f, 0.0f, 0.0f));
        normalMat = modelMat;
        s_model->subModel("AileronR01")->localTransform(modelMat, normalMat);
    }
}

static void changeAileronAngle(float deltaAngle) {
    setAileronAngle(s_aileronAngle + deltaAngle);
}

static void setElevatorAngle(float angle) {
    angle = glm::clamp(angle, -k_maxElevatorAngle, k_maxElevatorAngle);
    if (angle != s_elevatorAngle) {
        s_elevatorAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(s_elevatorAngle), vec3(1.0f, 0.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
        s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);
    }
}

static void changeElevatorAngle(float deltaAngle) {
    setElevatorAngle(s_elevatorAngle + deltaAngle);
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
    //if space key is pressed, increase thrust
    else if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        s_increaseThrust = true;
    }

    else if (key == GLFW_KEY_L && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        s_simObject->a_pos.y += 0.03125;
    }

    else if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        s_simObject->a_pos.y -= 0.03125;
    }
}


static bool setupModel() {
    s_model = Model::load(g_resourcesDir + "/models/f18.grl");
    if (!s_model) {
        std::cerr << "Failed to load model" << std::endl;
        return false;
    }

    s_modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 0.0f, 1.0f)); //upside down at first
    s_modelMat = s_modelMat * glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 1.0f, 0.0f)); //upside down at first
    //s_modelMat = glm::translate(mat4(), vec3(0, 0, 1)) * s_modelMat;
    s_momentOfInertia = 1.0f;
    s_windframeWidth = 20.5f;
    s_windframeDepth = 22.0f;

    s_normalMat = glm::transpose(glm::inverse(s_modelMat));

    //send first rotation matrix and model to the rld
    rld::set(*s_model, s_modelMat, s_normalMat, s_windframeWidth, s_windframeDepth, s_windSpeed, false);

    //setup simObject
    s_simObject = std::make_unique<SimObject>();
    if (!s_simObject) {
        std::cerr << "Failed to create sim object" << std::endl;
        return false;
    }
    s_simObject->setMass(16769); //gross weight in kg pulled from wiki
    s_simObject->setMaxThrust(62.3 * 1000.f * 2.f); //dry thrust from wiki without afterburner (62.3kN per enginer)
    s_simObject->setGravityOn(false);
    s_simObject->setTimeScale(k_timeScale);
    s_simObject->pos.y = 30.f; //in meters
    s_simObject->vel.z = -120.f; //in m/s
    //s_simObject->a_pos.y = 0.001; //in m/s

    return true;
}

static bool setupShader() {
    std::string shadersPath(g_resourcesDir + "/FlightSim/shaders/");

    // Foil Shader
    if (!(s_phongProg = Shader::load(shadersPath + "phong.vert", shadersPath + "phong.frag"))) {
        std::cerr << "Failed to load foil shader" << std::endl;
        return false;
    }

    return true;
}


static bool setup() {
    if (XBOXController.IsConnected())
        std::cout << "xbos controller connected" << std::endl;
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
    glViewport(0, 0, k_windowSize.x, k_windowSize.y);


    // Setup simulation
    if (!rld::setup(k_simTexSize, k_simSliceCount, k_simLiftC, k_simDragC)) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }

    //setup model for sim
    if (!setupModel()) {
        std::cerr << "Failed to load model for Sim" << std::endl;
        return false;
    }

    //load shader for sim
    if (!setupShader()) {
        std::cerr << "Failed to load shader for Sim" << std::endl;
        return false;
    }


    //setup progterrain
    ProgTerrain::init_shaders();
    ProgTerrain::init_geom();

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

static mat4 getOrthographicMatrix() {
    float windframeRadius(s_windframeWidth * 0.5f);

    return glm::ortho(
        -windframeRadius, // left
        windframeRadius,  // right
        -windframeRadius, // bottom
        windframeRadius,  // top
        0.01f, // near
        100.f // far
    );
}

static mat4 getViewMatrix(vec3 camPos) {
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

static mat4 getWindViewMatrix(vec3 wind) {
    return glm::lookAt(
        vec3(0, 0, 0), //camPos
        wind, //looking in the direction of the wind
        vec3(0, 1, 0) //don't care about roll so just always global up
    );
}

static std::string matrixToString(mat4 m) {
    std::string mstring = "";
    for (int i = 0; i < 4; i++) {
        mstring += "[" + std::to_string(m[0][i]) + ", " + std::to_string(m[1][i]) + ", " + std::to_string(m[2][i]) + ", " + std::to_string(m[3][i]) + "]\n";
    }
    return mstring;
}


static void render(float frametime) {
    //glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 wind = -(s_simObject->vel); //wind is equivalent to opposite direction/speed of velocity
    s_windSpeed = length(s_simObject->vel);
    mat4 windViewMatrix = getWindViewMatrix(wind);
    mat4 simRotateMat = s_simObject->getRotate();
    mat4 simTranslateMat = s_simObject->getTranslate();

    vec3 camOffset = vec3(simRotateMat * vec4(0, 0, 25.0, 1));
    vec3 camPos = s_simObject->pos + camOffset;
    mat4 viewMat = getViewMatrix(camPos);

    //std::cout << "combined force: " << glm::to_string(combinedForce) << std::endl;
    //std::cout << "torque: " << glm::to_string(torque) << std::endl;
    //std::cout << "time scale: " << k_timeScale << std::endl;
    //std::cout << "curPos: " << glm::to_string(s_simObject->pos) << std::endl;
    //std::cout << "curAngle: " << glm::to_string(s_simObject->a_pos) << std::endl;
    //std::cout << "Wind vector: " << glm::to_string(wind) << std::endl;
    //std::cout << "sim rotate mat\n" << matrixToString(simRotateMat) << std::endl << std::endl;
    //std::cout << "wind view matrix\n" << matrixToString(windViewMatrix) << std::endl << std::endl;
    //std::cout << "s_model matrix\n" << matrixToString(s_modelMat) << std::endl << std::endl;
    //std::cout << "Final rld modelMat\n" << matrixToString(windViewMatrix * simRotateMat * s_modelMat) << std::endl << std::endl;
    //std::cout << "Rotate mat of what it should be:\n" << matrixToString(glm::rotate(mat4(), -3.14159f / 2.f, vec3(0, 0, 1))) << std::endl << std::endl;
    //
    s_normalMat = glm::transpose(glm::inverse(s_modelMat));
    rld::set(*s_model, windViewMatrix * simRotateMat * s_modelMat, s_normalMat, s_windframeWidth, s_windframeDepth, s_windSpeed, false);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    rld::sweep();
    vec3 lift = rld::lift();
    vec3 drag = rld::drag();
    vec3 combinedForce = lift + drag;
    vec3 torque = rld::torque();

    //std::cout << "thrustVal (in Newtons): " << s_simObject->getThrustVal() << std::endl;
    //std::cout << "vel: " << glm::to_string(s_simObject->vel) << std::endl << std::endl;
    s_simObject->addTranslationalForce(combinedForce * 10.f);
    float mEV = 100000.f;
    /*if (length(torque) > mEV || length(lift) > mEV || length(drag) > mEV) {
        torque = vec3(0);
        lift = vec3(0);
        drag = vec3(0);
    }*/
    torque *= 10.f;
    s_simObject->addAngularForce(torque);
    s_simObject->update(frametime);
    std::cout << "lift: " << glm::to_string(lift) << std::endl;
    //std::cout << "drag: " << glm::to_string(drag) << std::endl;
    std::cout << "torque: " << torque.y /1000.f << std::endl;
    //std::cout << "y angle (in degrees) " << glm::degrees(s_simObject->a_pos.y) << std::endl;
    //std::cout << "y angle (in radians) " << s_simObject->a_pos.y << std::endl;
    std::cout << "angle: " << s_simObject->a_pos.y << std::endl;

    //std::cout << "pos " << glm::to_string(s_simObject->pos) << std::endl;
    glViewport(0, 0, k_windowSize.x, k_windowSize.y);
    std::cout << std::endl;


    mat4 modelMat, normalMat;
    mat4 projMat;

    modelMat = s_modelMat;
    normalMat = s_normalMat;


    //modelMat = mat4();

    if (k_windDebug) {
        projMat = getOrthographicMatrix();
        modelMat = windViewMatrix * simRotateMat * s_modelMat; //what the wind sees (use for debugging)
        viewMat = glm::translate(mat4(), vec3(0, 0, -17.5)); //just move model away so we can see it on screen. Just for debugging not used anywhere for sim
    }
    else {
        projMat = getPerspectiveMatrix();
        modelMat = simTranslateMat * simRotateMat * modelMat; //what should be rendered
        normalMat = glm::transpose(glm::inverse(modelMat));
        ProgTerrain::render(viewMat, projMat, -camPos); //todo no idea why I have to invert this
    }


    glEnable(GL_DEPTH_TEST);

    s_phongProg->bind();
    s_phongProg->uniform("u_projMat", projMat);
    s_phongProg->uniform("u_viewMat", viewMat);
    s_model->draw(modelMat, normalMat, s_phongProg->uniformLocation("u_modelMat"), s_phongProg->uniformLocation("u_normalMat"));

    s_phongProg->unbind();


    //reset gl variables set to not mess up rld sim
    glDisable(GL_DEPTH_TEST);

}

static float stickVal(s16 v) {
    constexpr float k_posNormFactor(1.0f / 32767.0f), k_negNormFactor(1.0f / 32768.0f);
    float fv(v >= 0 ? v * k_posNormFactor : v * k_negNormFactor);

    constexpr float k_threshold(0.25f);
    constexpr float k_invFactor(1.0f / (1.0f - k_threshold));
    if (fv >= +k_threshold) return (fv - k_threshold) * k_invFactor;
    if (fv <= -k_threshold) return (fv + k_threshold) * k_invFactor;
    return 0.0f;
}

static float triggerVal(u08 v) {
    constexpr float k_normFactor(1.0f / 255.0f);
    float fv(v * k_normFactor);

    constexpr float k_threshold(0.25f);
    constexpr float k_invFactor(1.0f / (1.0f - k_threshold));
    return fv >= k_threshold ? (fv - k_threshold) * k_invFactor : 0.0f;
}

static void processStick(float dt) {
    XINPUT_STATE state(XBOXController.GetState());
    float lx(stickVal(state.Gamepad.sThumbLX));
    float ly(stickVal(state.Gamepad.sThumbLY));
    float rx(stickVal(state.Gamepad.sThumbRX));
    float rsh(triggerVal(state.Gamepad.bRightTrigger));

    setRudderAngle(rx * k_maxRudderAngle);
    setAileronAngle(lx * k_maxAileronAngle);
    setElevatorAngle(ly * k_maxElevatorAngle);

    if (rsh > 0.0f) {
        int x = 9;
    }

    s_simObject->thrust = rsh;
}

double get_last_elapsed_time()
{
    static double lasttime = glfwGetTime();
    double actualtime = glfwGetTime();
    double difference = actualtime - lasttime;
    lasttime = actualtime;
    return difference;
}



int main(int argc, char ** argv) {
    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(s_window)) {
        float frametime{float(get_last_elapsed_time())};
        glfwPollEvents();
        processStick(frametime);

        render(frametime);
        glfwSwapBuffers(s_window);
    }

    cleanup();

    return EXIT_SUCCESS;
}
