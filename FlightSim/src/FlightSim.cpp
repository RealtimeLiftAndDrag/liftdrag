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
#include <sstream>
#include <iomanip>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/string_cast.hpp"

#include "Common/Global.hpp"
#include "Common/Model.hpp"
#include "Common/Shader.hpp"
#include "Common/Controller.hpp"
#include "Common/SkyBox.hpp"
#include "Common/Camera.hpp"
#include "UI/Text.hpp"
#include "UI/Group.hpp"
#include "RLD/Simulation.hpp"

#include "SimObject.hpp"
#include "ProgTerrain.hpp"



class MainComp;



static const ivec2 k_defWindowSize(1280, 720);
static const std::string k_windowTitle("RLD Flight Simulator");

static constexpr int k_simTexSize(1024);
static constexpr int k_simSliceCount(100);
static constexpr float k_simLiftC(1.0f);
static constexpr float k_simDragC(1.0f);
static constexpr float k_windframeWidth(14.5f);
static constexpr float k_windframeDepth(22.0f);

static constexpr float k_gravity(0.0f);//9.8f);
static const vec3 k_lightDir(glm::normalize(vec3(-1.0f, 0.25f, -1.0f)));

static const float k_fov(glm::radians(75.0f));
static const float k_camPanAngle(0.01f);
static const float k_camZoomAmount(0.1f);
static const float k_camMinDist(5.0f);
static const float k_camMaxDist(50.0f);
static const float k_camNear(0.1f);
static const float k_camFar(1000.0f);

static const vec3 k_initPos(0.0f, 80.0f, 0.0f);
static const vec3 k_initDir(0.0f, 0.0f, -1.0f);
static constexpr float k_initSpeed(60.0f);
static constexpr float k_minCamDist(5.0f), k_maxCamDist(50.0f);
static constexpr float k_camSpeed(0.05f);

static unq<Model> s_model;
static unq<SimObject> s_simObject;
static unq<Shader> s_planeShader;
static unq<SkyBox> s_skyBox;

static mat4 s_modelMat;
static mat3 s_normalMat;
static float s_windSpeed;
static float s_turbulenceDist;
static float s_maxSearchDist;
static float s_windShadDist;
static float s_backforceC;
static float s_flowback;
static float s_initVelC;
static bool s_unpaused;

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

static ivec2 s_camControlDelta;
static bool s_windView;

static mat4 s_rldModelMat;
static mat3 s_rldNormalMat;

static shr<MainComp> s_mainComp;
static shr<ui::Text> s_textComp;



class MainComp : public ui::Single {

    public:

    MainComp() :
        Single(),
        m_camera(k_camMinDist, k_camMaxDist)
    {
        m_camera.zoom(0.5f);
        m_camera.near(k_camNear);
        m_camera.far(k_camFar);
    }

    virtual void pack() override {        
        vec2 aspect(aspect());
        m_camera.fov(k_fov * aspect);
        m_windViewViewMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -k_windframeDepth * 0.5f));
        m_windViewOrthoMat = glm::ortho(
            -k_windframeWidth * 0.5f * aspect.x, // left
             k_windframeWidth * 0.5f * aspect.x, // right
            -k_windframeWidth * 0.5f * aspect.y, // bottom
             k_windframeWidth * 0.5f * aspect.y, // top
            -k_windframeDepth * 0.5f, // near
             k_windframeDepth * 0.5f  // far
        );
    }

    virtual void render() const override {
        setViewport();

        glEnable(GL_DEPTH_TEST);

        mat4 modelMat;
        mat3 normalMat;
        mat4 viewMat;
        mat4 projMat;
        vec3 camPos;

        if (s_windView) { // What the wind sees (use for debugging)
            modelMat = s_rldModelMat;
            normalMat = s_rldNormalMat;
            // Orthographic if shift is pressed
            if (ui::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
                projMat = m_windViewOrthoMat;
            }
            // Perspective otherwise
            else {
                viewMat = m_windViewViewMat;
                projMat = m_camera.projMat();
            }
        }
        else {
            modelMat = s_simObject->orientMatrix();
            modelMat = modelMat * s_modelMat;
            modelMat[3] = vec4(s_simObject->position(), 1.0f);
            normalMat = s_simObject->orientMatrix() * s_normalMat;
            viewMat = mat4(glm::transpose(s_simObject->orientMatrix())) * glm::translate(-s_simObject->position());
            viewMat = m_camera.viewMat() * viewMat;
            camPos = s_simObject->position() + s_simObject->orientMatrix() * m_camera.position();
            projMat = m_camera.projMat();
            ProgTerrain::render(viewMat, projMat, -camPos);
            s_skyBox->render(viewMat, projMat);
        }

        // TODO: disable backface culling

        s_planeShader->bind();
        s_planeShader->uniform("u_projMat", projMat);
        s_planeShader->uniform("u_viewMat", viewMat);
        s_planeShader->uniform("u_camPos", camPos);
        s_model->draw(modelMat, normalMat, s_planeShader->uniformLocation("u_modelMat"), s_planeShader->uniformLocation("u_normalMat"));
        Shader::unbind();

        glDisable(GL_DEPTH_TEST);
    }

    virtual void keyEvent(int key, int action, int mods) override {
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
        else if (key == GLFW_KEY_K && action == GLFW_RELEASE) {
            s_windView = !s_windView;
        }
        // R resets the object
        else if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
            s_simObject->reset(k_initPos, k_initDir, k_initSpeed);
        }
        // R resets the object
        else if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
            s_unpaused = !s_unpaused;
        }
        // Excape exits
        else if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
            ui::requestExit();
        }
    }

    virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override {
        if (ui::isMouseButtonPressed(0)) {
            m_camera.thetaPhi(
                m_camera.theta() - float(delta.x) * k_camPanAngle,
                m_camera.phi() + float(delta.y) * k_camPanAngle
            );
        }
    }

    virtual void mouseButtonEvent(int button, int action, int mods) override {
        // Reset camera
        if (button == 0 && action == GLFW_RELEASE && !mods) {
            m_camera.thetaPhi(0.0f, glm::pi<float>() * 0.5f);
        }
    }

    virtual void scrollEvent(const ivec2 & delta) override {
        m_camera.zoom(m_camera.zoom() - float(delta.y) * k_camZoomAmount);
    }

    private:

    ThirdPersonCamera m_camera;
    mat4 m_windViewViewMat;
    mat4 m_windViewOrthoMat;

};



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

void stickCallback(int player, Controller::Stick stick, vec2 val) {
    if (stick == Controller::Stick::left) {
        s_controllerPitch = -val.y;
        s_controllerRoll = val.x;
    }
    else if (stick == Controller::Stick::right) {
        s_controllerYaw = val.x;
    }
}

void triggerCallback(int player, Controller::Trigger trigger, float val) {\
    if (trigger == Controller::Trigger::right) {
        s_controllerThrust = val;
    }
}

static std::string createTextString(const vec3 & lift, const vec3 & drag, const vec3 & torq, float speed) {
    constexpr int k_preDigits(7), k_postDigits(2);
    constexpr int k_width(k_preDigits + k_postDigits + 2);
    std::stringstream ss;
    ss.precision(k_postDigits);
    ss << std::fixed;
    ss << "Lift: <" << std::setw(k_width) << lift.x << " " << std::setw(k_width) << lift.y << " " << std::setw(k_width) << lift.z << ">\n";
    ss << "Drag: <" << std::setw(k_width) << drag.x << " " << std::setw(k_width) << drag.y << " " << std::setw(k_width) << drag.z << ">\n";
    ss << "Torq: <" << std::setw(k_width) << torq.x << " " << std::setw(k_width) << torq.y << " " << std::setw(k_width) << torq.z << ">\n";
	ss << "Speed: " << std::setw(k_width) << speed;
    return ss.str();
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
    s_modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, 2.0f)) * s_modelMat; // move center of gravity forward to help with stability
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

static void setupUI() {
    s_mainComp.reset(new MainComp());
    s_textComp.reset(new ui::Text(createTextString(vec3(), vec3(), vec3(), 0.0f), ivec2(1, 1), vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    shr<ui::HorizontalGroup> horizGroup(new ui::HorizontalGroup());
    horizGroup->add(s_textComp);
    horizGroup->add(shr<ui::Space>(new ui::Space()));
    shr<ui::VerticalGroup> vertGroup(new ui::VerticalGroup());
    vertGroup->add(shr<ui::Space>(new ui::Space()));
    vertGroup->add(horizGroup);
    shr<ui::LayerGroup> layerGroup(new ui::LayerGroup());
    layerGroup->add(s_mainComp);
    layerGroup->add(vertGroup);
    ui::setRootComponent(layerGroup);
}

static bool setup() {
    // Setup UI, which includes GLFW and GLAD
    if (!ui::setup(k_defWindowSize, k_windowTitle.c_str(), 4, 5, false)) {
        std::cerr << "Failed to setup UI" << std::endl;
        return false;
    }
    ui::disableCursor();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    // Setup object
    if (!setupObject()) {
        std::cerr << "Failed to setup model" << std::endl;
        return false;
    }

    // Setup RLD
    if (!rld::setup(
        k_simTexSize,
        k_simSliceCount,
        k_simLiftC,
        k_simDragC,
        s_turbulenceDist,
        s_maxSearchDist,
        s_windShadDist,
        s_backforceC,
        s_flowback,
        s_initVelC,
        false,
        false,
        false
    )) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }

    // Setup plane shader
    std::string shadersPath(g_resourcesDir + "/FlightSim/shaders/");
    if (!(s_planeShader = Shader::load(shadersPath + "plane.vert", shadersPath + "plane.frag"))) {
        std::cerr << "Failed to load plane shader" << std::endl;
        return false;
    }
    s_planeShader->bind();
    s_planeShader->uniform("u_lightDir", k_lightDir);
    Shader::unbind();

    //setup progterrain
    ProgTerrain::init_shaders();
    ProgTerrain::init_geom();

    // Setup xbox controller
    Controller::poll(1);
    Controller::stickCallback(stickCallback);
    Controller::triggerCallback(triggerCallback);

    // Setup sky box
    std::string skyBoxPath(g_resourcesDir + "/FlightSim/textures/sky_florida_1024/");
    if (!(s_skyBox = SkyBox::create({
        skyBoxPath + "right.png",
        skyBoxPath + "left.png",
        skyBoxPath + "top.png",
        skyBoxPath + "bottom.png",
        skyBoxPath + "front.png",
        skyBoxPath + "back.png"
    }))) {
        std::cerr << "Failed to create sky box" << std::endl;
        return false;
    }

    setupUI();

    s_unpaused = true;
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

static void updatePlane(float dt) {
    vec3 wind(-s_simObject->velocity()); //wind is equivalent to opposite direction/speed of velocity
    float windSpeed(glm::length(wind));
    vec3 windW(wind / -windSpeed); // Normalize. Negative because the W vector is opposite the direction "looked in"
    vec3 windU(glm::normalize(glm::cross(s_simObject->v(), windW))); // TODO: will break if wind direction is parallel to object's v
    vec3 windV(glm::cross(windW, windU));
    mat3 windBasis(windU, windV, windW);

    //mat3 turnAroundMat(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f); // manual because I don't want the imprecision from the trig functions
    mat3 rldOrientMat(glm::transpose(windBasis) * s_simObject->orientMatrix());// * turnAroundMat);
    s_rldModelMat = mat4(rldOrientMat) * s_modelMat;
    s_rldNormalMat = rldOrientMat * s_normalMat;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    rld::set(*s_model, s_rldModelMat, s_rldNormalMat, k_windframeWidth, k_windframeDepth, glm::length(wind), false);
    rld::sweep();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    rld::Result result(rld::result());
    vec3 lift = windBasis * vec3(result.lift.x, result.lift.y, 0.0f); // TODO: figure out what is up with lift along z axis
    vec3 drag = windBasis * result.drag;
    vec3 torq = windBasis * result.torq;
    //lift.x = lift.y = 0.0f;
    //drag.x = drag.y = 0.0f;
    //torq.y = torq.z = 0.0f;

    s_simObject->addTranslationalForce(lift + drag);
    s_simObject->addAngularForce(torq);
    s_simObject->update(dt);

    s_textComp->string(createTextString(lift, drag, torq, glm::length(s_simObject->velocity())));
}

static void update(float dt) {
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

    // Update plane using RLD
    if (s_unpaused) {
        updatePlane(dt);
    }
}



int main(int argc, char ** argv) {
    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }

    double then(glfwGetTime());
    while (!ui::shouldExit()) {
        double now(glfwGetTime());
        float dt(float(now - then));
        then = now;

        ui::poll();
        Controller::poll(1);

        update(dt);
        ui::update();
        ui::render();
    }

    cleanup();

    return EXIT_SUCCESS;
}
