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
#include "glm/gtx/string_cast.hpp"

#include "Common/Global.hpp"
#include "Common/Model.hpp"
#include "Common/Shader.hpp"
#include "Common/Controller.hpp"
#include "UI/Group.hpp"
#include "RLD/Simulation.hpp"

#include "SimObject.hpp"
#include "ProgTerrain.hpp"



class MainUIC : public ui::VerticalGroup {

public:

    virtual void keyEvent(int key, int action, int mods) override;

};

static const ivec2 k_defWindowSize(1280, 1280);
static const std::string k_windowTitle("RLD Flight Simulator");

static constexpr int k_simTexSize(1024);
static constexpr int k_simSliceCount(100);
static constexpr float k_simLiftC(0.3f);
static constexpr float k_simDragC(1.0f);
static constexpr float k_windframeWidth(14.5f);
static constexpr float k_windframeDepth(22.0f);

static constexpr float k_fov(glm::radians(75.0f));
static constexpr float k_near(0.01f), k_far(1000.0f);
static constexpr float k_gravity(0.0f);//9.8f);

static vec3 k_initPos(0.0f, 80.0f, 0.0f);
static vec3 k_initDir(0.0f, 0.0f, -1.0f);
static float k_initSpeed(60.0f);

static ivec2 s_windowSize(k_defWindowSize);

static unq<Model> s_model;
static unq<SimObject> s_simObject;
static unq<Shader> s_planeShader;

static mat4 s_modelMat; 
static mat3 s_normalMat;
static float s_windSpeed;
static float s_turbulenceDist;
static float s_maxSearchDist;
static float s_windShadDist;
static float s_backforceC;
static float s_flowback;
static float s_initVelC;

// all in degrees
static float s_rudderAngle(0.0f);
static float s_aileronAngle(0.0f);
static float s_elevatorAngle(0.0f);
static bool s_increaseThrust(false);

static constexpr float k_maxRudderAngle(30.0f);
static constexpr float k_maxAileronAngle(30.0f);
static constexpr float k_maxElevatorAngle(30.0f);

static constexpr float k_keyAngleSpeed(90.0f); // how quickly the rudders/ailerons/elevators will rotate when using the keyboard in degrees per second
static constexpr float k_returnAngleSpeed(90.0f); // how quickly the rudders/ailerons/elevators will return to their default position in degrees per second

static GLFWwindow * s_window;
static Controller s_controller(0);

// These apply when using the keyboard to control the plane
static bool s_keyboardYawCCW, s_keyboardYawCW;
static bool s_keyboardPitchCCW, s_keyboardPitchCW;
static bool s_keyboardRollCCW, s_keyboardRollCW;
static bool s_keyboardThrust;

// These apply when using a controller to control the plane
static float s_controllerYaw;
static float s_controllerPitch;
static float s_controllerRoll;
static float s_controllerThrust;

static bool s_windView;

static mat4 s_perspectiveMat;
static mat4 s_windViewViewMat;
static mat4 s_windViewOrthoMat;



static void errorCallback(int error, const char * description) {
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

// Positive angle means positive yaw and vice versa
static void setRudderAngle(float angle) {
    angle = glm::clamp(angle, -k_maxRudderAngle, k_maxRudderAngle);
    if (angle != s_rudderAngle) {
        s_rudderAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(-s_rudderAngle), vec3(0.0f, 1.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
        s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);
    }
}

static void changeRudderAngle(float deltaAngle) {
    setRudderAngle(s_rudderAngle + deltaAngle);
}

// Positive angle means positive roll and vice versa
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

// Positive angle means positive pitch and vice versa
static void setElevatorAngle(float angle) {
    angle = glm::clamp(angle, -k_maxElevatorAngle, k_maxElevatorAngle);
    if (angle != s_elevatorAngle) {
        s_elevatorAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(-s_elevatorAngle), vec3(1.0f, 0.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
        s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);
    }
}

static void changeElevatorAngle(float deltaAngle) {
    setElevatorAngle(s_elevatorAngle + deltaAngle);
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // TODO: verify these axes
    // D and A control yaw
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) s_keyboardYawCCW = true;
        else if (action == GLFW_RELEASE) s_keyboardYawCCW = false;
    }
    else if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) s_keyboardYawCW = true;
        else if (action == GLFW_RELEASE) s_keyboardYawCW = false;
    }
    // W and S control pitch
    else if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) s_keyboardPitchCCW = true;
        else if (action == GLFW_RELEASE) s_keyboardPitchCCW = false;
    }
    else if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS && !mods) s_keyboardPitchCW = true;
        else if (action == GLFW_RELEASE) s_keyboardPitchCW = false;
    }
    // E and Q control roll
    else if (key == GLFW_KEY_E) {
        if (action == GLFW_PRESS) s_keyboardRollCCW = true;
        else if (action == GLFW_RELEASE) s_keyboardRollCCW = false;
    }
    else if (key == GLFW_KEY_Q) {
        if (action == GLFW_PRESS) s_keyboardRollCW = true;
        else if (action == GLFW_RELEASE) s_keyboardRollCW = false;
    }
    // Space controls thrust
    else if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) s_keyboardThrust = true;
        else if (action == GLFW_RELEASE) s_keyboardThrust = false;
    }
    // K enables wind view
    else if (key == GLFW_KEY_K) {
        if (action == GLFW_PRESS) s_windView = true;
        else if (action == GLFW_RELEASE) s_windView = false;
    }
	// R resets the object
	else if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
		s_simObject->reset(k_initPos, k_initDir, k_initSpeed);
	}
}

void detMatrices();

static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
	s_windowSize.x = width;
	s_windowSize.y = height;
	detMatrices();
}

static bool setupObject() {
    // The plane model is upside-down, pointed toward +z
    s_model = Model::load(g_resourcesDir + "/models/f18_zero_normals.grl");
    if (!s_model) {
        std::cerr << "Failed to load model" << std::endl;
        return false;
    }

    s_modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 0.0f, 1.0f)); // flip right-side up
    s_modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 1.0f, 0.0f)) * s_modelMat; // turn to face -z
    s_modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, 2.0f)) * s_modelMat;
    s_normalMat = glm::transpose(glm::inverse(s_modelMat));

    s_turbulenceDist = 0.225f;
    s_windShadDist = 1.5f;
    s_backforceC = 50000.0f;
    s_flowback = 0.15f;
    s_initVelC = 1.0f;

    // Numbers taken from "Susceptibility of F/A-18 Flight Controllers to the Falling-Leaf Mode: Linear Analysis" - Chakraborty, Seiler, Balas
    // http://www.aem.umn.edu/~AerospaceControl/V&VWebpage/Papers/AIAALin.pdf
    float mass(15097.393f); // gross weight in kg pulled from wiki
    vec3 inertiaTensor(205125.765f, 230414.482f, 31183.813f); // pitch, yaw, roll
    float dryThrust(71616.368f * 2.0f); // thrust in N without afterburners pulled from wiki (62.3kN per enginer)

    s_simObject.reset(new SimObject(mass, inertiaTensor, dryThrust, k_initPos, k_initDir, k_initSpeed));

    return true;
}

static void detMatrices() {
    // Perspective matrix
    float aspect(float(s_windowSize.x) / float(s_windowSize.y));
    float fov(s_windowSize.x >= s_windowSize.y ? k_fov : k_fov / aspect); // fov is always for smaller dimension
    s_perspectiveMat = glm::perspective(fov, aspect, k_near, k_far);

    // Wind view view matrix
    s_windViewViewMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -k_windframeDepth * 0.5f));

    // Wind view orthographic matrix
    s_windViewOrthoMat = glm::ortho(
        -k_windframeWidth * 0.5f, // left
         k_windframeWidth * 0.5f, // right
        -k_windframeWidth * 0.5f, // bottom
         k_windframeWidth * 0.5f, // top
        -k_windframeDepth * 0.5f, // near
         k_windframeDepth * 0.5f  // far
    );
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
    if (!(s_window = glfwCreateWindow(s_windowSize.x, s_windowSize.y, k_windowTitle.c_str(), nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(s_window);
    glfwSwapInterval(0); // VSync on or off
    glfwSetKeyCallback(s_window, keyCallback);
	glfwSetFramebufferSizeCallback(s_window, framebufferSizeCallback);

    // Setup GLAD
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, s_windowSize.x, s_windowSize.y);

    // Setup object
    if (!setupObject()) {
        std::cerr << "Failed to setup model" << std::endl;
        return false;
    }

    // Setup RLD
    if (!rld::setup(k_simTexSize, k_simSliceCount, k_simLiftC, k_simDragC, s_turbulenceDist, s_maxSearchDist, s_windShadDist, s_backforceC, s_flowback, s_initVelC)) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }

    // Setup plane shader
    std::string shadersPath(g_resourcesDir + "/FlightSim/shaders/");
    if (!(s_planeShader = Shader::load(shadersPath + "plane.vert", shadersPath + "plane.frag"))) {
        std::cerr << "Failed to load plane shader" << std::endl;
        return false;
    }

    //setup progterrain
    ProgTerrain::init_shaders();
    ProgTerrain::init_geom();

    // Setup xbox controller
    s_controller.poll();
    if (s_controller.connected()) {
        std::cout << "Controller connected" << std::endl;
    }

    detMatrices();

    return true;
}

static void cleanup() {
    rld::cleanup();
    glfwTerminate();
}

static mat4 getWindViewMatrix(const vec3 & wind) {
    return glm::lookAt(vec3(), wind, vec3(0.0f, 1.0f, 0.0f));
}

static std::string matrixToString(mat4 m) {
    std::string mstring = "";
    for (int i = 0; i < 4; i++) {
        mstring += "[" + std::to_string(m[0][i]) + ", " + std::to_string(m[1][i]) + ", " + std::to_string(m[2][i]) + ", " + std::to_string(m[3][i]) + "]\n";
    }
    return mstring;
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

static void processController() {
    if (s_controller.connected()) {
        s_controller.poll();
        s_controllerYaw = s_controller.rStick().x;
        s_controllerPitch = -s_controller.lStick().y;
        s_controllerRoll = s_controller.lStick().x;
        s_controllerThrust = s_controller.rTrigger();
    }
}

static void update(float dt) {
    processController();

    // Update yaw
    if (s_controllerYaw) { // Controller takes priority
        setRudderAngle(s_controllerYaw * k_maxRudderAngle);
    }
    else { // Otherwise keyboard
        if (s_keyboardYawCCW || s_keyboardYawCW) {
            float keyboardYaw(float(s_keyboardYawCCW) + -float(s_keyboardYawCW));
            changeRudderAngle(keyboardYaw * k_keyAngleSpeed * dt);
        }
        else { // No input, return to default position
            float newAngle(s_rudderAngle + k_returnAngleSpeed * -glm::sign(s_rudderAngle) * dt);
            if (newAngle * s_rudderAngle < 0.0f) newAngle = 0.0f;
            setRudderAngle(newAngle);
        }
    }

    // Update pitch
    if (s_controllerPitch) { // Controller takes priority
        setElevatorAngle(s_controllerPitch * k_maxElevatorAngle);
    }
    else { // Otherwise keyboard
        if (s_keyboardPitchCCW || s_keyboardPitchCW) {
            float keyboardPitch(float(s_keyboardPitchCCW) + -float(s_keyboardPitchCW));
            changeElevatorAngle(keyboardPitch * k_keyAngleSpeed * dt);
        }
        else { // No input, return to default position
            float newAngle(s_elevatorAngle + k_returnAngleSpeed * -glm::sign(s_elevatorAngle) * dt);
            if (newAngle * s_elevatorAngle < 0.0f) newAngle = 0.0f;
            setElevatorAngle(newAngle);
        }
    }

    // Update roll
    if (s_controllerRoll) { // Controller takes priority
        setAileronAngle(s_controllerRoll * k_maxAileronAngle);
    }
    else { // Otherwise keyboard
        if (s_keyboardRollCCW || s_keyboardRollCW) {
            float keyboardRoll(float(s_keyboardRollCCW) + -float(s_keyboardRollCW));
            changeAileronAngle(keyboardRoll * k_keyAngleSpeed * dt);
        }
        else { // No input, return to default position
            float newAngle(s_aileronAngle + k_returnAngleSpeed * -glm::sign(s_aileronAngle) * dt);
            if (newAngle * s_aileronAngle < 0.0f) newAngle = 0.0f;
            setAileronAngle(newAngle);
        }
    }

    // Update thrust
    if (s_controllerThrust) { // Controller takes priority
        s_simObject->thrust(s_controllerThrust);
    }
    else { // Otherwise keyboard
        s_simObject->thrust(float(s_keyboardThrust));
    }
}

static void render(float dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 wind(-s_simObject->velocity()); //wind is equivalent to opposite direction/speed of velocity
    float windSpeed(glm::length(wind));
    vec3 windW(wind / -windSpeed); // Normalize. Negative because the W vector is opposite the direction "looked in"
    vec3 windU(glm::normalize(glm::cross(s_simObject->v(), windW))); // TODO: will break if wind direction is parallel to object's v
    vec3 windV(glm::cross(windW, windU));
    mat3 windBasis(windU, windV, windW);

    //mat3 turnAroundMat(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f); // manual because I don't want the imprecision from the trig functions
    mat3 rldOrientMat(glm::transpose(windBasis) * s_simObject->orientMatrix());// * turnAroundMat);
    mat4 rldModelMat(mat4(rldOrientMat) * s_modelMat);
    mat3 rldNormalMat(rldOrientMat * s_normalMat);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    rld::set(*s_model, rldModelMat, rldNormalMat, k_windframeWidth, k_windframeDepth, glm::length(wind), false);
    rld::sweep();

    vec3 lift = windBasis * vec3(rld::lift().x, rld::lift().y, 0.0f); // TODO: figure out what is up with lift along z axis
    vec3 drag = windBasis * rld::drag();
    vec3 torq = windBasis * rld::torq();
    //lift.x = lift.y = 0.0f;
    //drag.x = drag.y = 0.0f;
    //torq.x = torq.z = 0.0f;

    s_simObject->addTranslationalForce(lift + drag);
    s_simObject->addAngularForce(torq);
    s_simObject->update(dt);

    std::cout << glm::to_string(s_simObject->velocity()) << std::endl;

    glViewport(0, 0, s_windowSize.x, s_windowSize.y);

    mat4 modelMat;
    mat3 normalMat;
    mat4 viewMat;
    mat4 projMat;

    if (s_windView) { // What the wind sees (use for debugging)
        modelMat = rldModelMat;
        normalMat = rldNormalMat;
        if (glfwGetKey(s_window, GLFW_KEY_LEFT_SHIFT)) { // Orthographic if shift is pressed
            projMat = s_windViewOrthoMat;
        }
        else { // Perspective otherwise
            viewMat = s_windViewViewMat;
            projMat = s_perspectiveMat;
        }
    }
    else {        
        modelMat = s_simObject->orientMatrix();
        modelMat = modelMat * s_modelMat;
        modelMat[3] = vec4(s_simObject->position(), 1.0f);
        normalMat = s_simObject->orientMatrix() * s_normalMat;
        vec3 camPos(s_simObject->position() + s_simObject->orientMatrix() * vec3(0.0f, 0.0f, 20.0f));
        viewMat = glm::lookAt(camPos, s_simObject->position(), vec3(0.0f, 1.0f, 0.0f));
        projMat = s_perspectiveMat;
        ProgTerrain::render(viewMat, projMat, -camPos); //todo no idea why I have to invert this
    }

    glEnable(GL_DEPTH_TEST);

    // TODO: disable backface culling

    s_planeShader->bind();
    s_planeShader->uniform("u_projMat", projMat);
    s_planeShader->uniform("u_viewMat", viewMat);
    s_model->draw(modelMat, normalMat, s_planeShader->uniformLocation("u_modelMat"), s_planeShader->uniformLocation("u_normalMat"));

    s_planeShader->unbind();


    //reset gl variables set to not mess up rld sim
    glDisable(GL_DEPTH_TEST);

}



int main(int argc, char ** argv) {
    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }

    double then(glfwGetTime());
    while (!glfwWindowShouldClose(s_window)) {
        double now(glfwGetTime());
        float dt(float(now - then));
        then = now;

        glfwPollEvents();

        update(dt);
        render(dt);
        glfwSwapBuffers(s_window);
    }

    cleanup();

    return EXIT_SUCCESS;
}
