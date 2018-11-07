// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert


// Allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement(1);
}
#endif

// TODO: should average backforce when multiple geo pixels, or simply sum? -> sum
// TODO: lift should be independent of the number of slices
// TODO: how far away should turbulence start? linear or squared air speed? -> don't know, no drag
// TODO: front-facing drag only on outline?? -> approx using prospect, long term solution as density flow nonsense
// TODO: infinity area thing
// TODO: velocity updating -> add windspeed to z, normalize, multiply so that z is equal to windspeed



#include <iostream>
#include <sstream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Common/Util.hpp"
#include "Common/Model.hpp"
#include "Common/Shader.hpp"
#include "Common/GLInterface.hpp"
#include "Common/Controller.hpp"
#include "UI/UI.hpp"
#include "UI/Group.hpp"
#include "UI/Text.hpp"
#include "UI/Graph.hpp"
#include "UI/TexViewer.hpp"

#include "RLD/Simulation.hpp"

#include "Results.hpp"



class MainUIC : public ui::VerticalGroup {

    public:

    virtual void keyEvent(int key, int action, int mods) override;

};



enum class SimModel { airfoil, f18, sphere };



static constexpr SimModel k_simModel(SimModel::f18);

static constexpr int k_simTexSize = 1024;
static constexpr int k_simSliceCount = 100;
static constexpr float k_simLiftC = 1.0f;
static constexpr float k_simDragC = 1.0f;
static constexpr float k_defWindSpeed = 100.0f;

static const ivec2 k_defWindowSize(1280, 720);

static constexpr float k_maxAutoAoA(45.0f);
static constexpr float k_autoAngleIncrement(1.0f); // how many degrees to change the angle of attack by when auto progressing
static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the rudder, elevator, and ailerons by when using arrow keys
static constexpr float k_seekSpeed(15.0f); // how many degrees to seek per second

static const std::string & k_controlsString(
    "Controls"                                      "\n"
    "  space       : step to next slice"            "\n"
    "  shift-space : sweep through all steps"       "\n"
    "  ctrl-space  : toggle auto angle progression" "\n"
    "  f           : do fast sweep"                 "\n"
    "  shift-f     : do fast sweep of all angles"   "\n"
    "  o or i      : change rudder angle"           "\n"
    "  k or j      : change elevator angle"         "\n"
    "  m or n      : change aileron angle"          "\n"
    "  =           : reset UI elements"
);



static unq<Model> s_model;
static mat4 s_modelMat;
static mat3 s_normalMat;
static float s_windframeWidth, s_windframeDepth;
static float s_windSpeed = k_defWindSpeed;
static float s_turbulenceDist;
static float s_maxSearchDist;
static float s_windShadDist;
static float s_backforceC;
static float s_flowback;
static float s_initVelC;
static vec2 s_angleGraphRange, s_sliceGraphRange;

static Controller s_controller(0);

//all in degrees
static float s_angleOfAttack(0.0f);
static float s_aileronAngle(0.0f);
static float s_rudderAngle(0.0f);
static float s_elevatorAngle(0.0f);

static bool s_shouldStep(false);
static bool s_shouldSweep(true);
static bool s_shouldAutoProgress(false);
static int s_seekDir(0);
static float s_seekAngle(0.0f);

static shr<MainUIC> s_mainUIC;
static shr<ui::TexViewer> s_frontTexViewer, s_turbTexViewer, s_sideTexViewer;
static shr<ui::String> s_angleLabel, s_angleLiftLabel, s_angleDragLabel, s_angleTorqueLabel;
static shr<ui::NumberField> s_angleField;
static shr<ui::Vector> s_angleLiftNum, s_angleDragNum, s_angleTorqueNum;
static shr<ui::String> s_rudderLabel, s_elevatorLabel, s_aileronLabel;
static shr<ui::Number> s_rudderNum, s_elevatorNum, s_aileronNum;

static bool s_isInfoChange(true);
static bool s_isF18Change(true);



static void printUsage() {
    std::cout << "Usage: liftdrag [resource_directory]" << std::endl;
}

static bool processArgs(int argc, char ** argv) {
    if (argc > 2) {
        printUsage();
        return false;
    }

    if (argc == 2) {
        g_resourcesDir = argv[1];
    }

    return true;
}

static float nearestVal(float v, float increment) {
    return glm::round(v / increment) * increment;
}

static void setAngleOfAttack(float angle) {
    if (glm::abs(angle) <= 90.0f) {
        s_angleOfAttack = angle;
        s_isInfoChange = true;
    }
}

static void changeAngleOfAttack(float deltaAngle) {
    setAngleOfAttack(s_angleOfAttack + deltaAngle);
}

// Positive angle means positive yaw and vice versa
static void setRudderAngle(float angle) {
    angle = glm::clamp(angle, -90.0f, 90.0f);
    if (angle != s_rudderAngle) {
        s_rudderAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(-s_rudderAngle), vec3(0.0f, 1.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
        s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);

        s_isF18Change = true;
    }
}

static void changeRudderAngle(float deltaAngle) {
    setRudderAngle(s_rudderAngle + deltaAngle);
}

// Positive angle means positive roll and vice versa
static void setAileronAngle(float angle) {
    angle = glm::clamp(angle, -90.0f, 90.0f);
    if (angle != s_aileronAngle) {
        s_aileronAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(s_aileronAngle), vec3(1.0f, 0.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("AileronL01")->localTransform(modelMat, normalMat);

        modelMat = glm::rotate(mat4(), glm::radians(-s_aileronAngle), vec3(1.0f, 0.0f, 0.0f));
        normalMat = modelMat;
        s_model->subModel("AileronR01")->localTransform(modelMat, normalMat);

        s_isF18Change = true;
    }
}

static void changeAileronAngle(float deltaAngle) {
    setAileronAngle(s_aileronAngle + deltaAngle);
}

// Positive angle means positive pitch and vice versa
static void setElevatorAngle(float angle) {
    angle = glm::clamp(angle, -90.0f, 90.0f);
    if (angle != s_elevatorAngle) {
        s_elevatorAngle = angle;

        mat4 modelMat(glm::rotate(mat4(), glm::radians(-s_elevatorAngle), vec3(1.0f, 0.0f, 0.0f)));
        mat3 normalMat(modelMat);
        s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
        s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);

        s_isF18Change = true;
    }
}

static void changeElevatorAngle(float deltaAngle) {
    setElevatorAngle(s_elevatorAngle + deltaAngle);
}

static void setSimulation(float angleOfAttack, bool debug) {
    mat4 rotMat(glm::rotate(mat4(), glm::radians(angleOfAttack), vec3(-1.0f, 0.0f, 0.0f)));

    rld::set(*s_model, rotMat * s_modelMat, mat3(rotMat) * s_normalMat, s_windframeWidth, s_windframeDepth, s_windSpeed, debug);
}

static void submitResults(float angleOfAttack) {
    results::submitAngle(angleOfAttack, { rld::lift(), rld::drag(), rld::torq() });
    const vec3 * lifts(rld::lifts());
    const vec3 * drags(rld::drags());
    const vec3 * torqs(rld::torqs());
    for (int i(0); i < rld::sliceCount(); ++i) {
        results::submitSlice(i, { lifts[i], drags[i], torqs[i] });
    }

    results::update();
}

static void doFastSweep(float angleOfAttack) {
    setSimulation(angleOfAttack, false);

    glDisable(GL_BLEND); // Can't have blending for simulation

    double then(glfwGetTime());
    rld::sweep();
    double dt(glfwGetTime() - then);

    glEnable(GL_BLEND);

    submitResults(angleOfAttack);

    std::cout << "Angle: " << angleOfAttack << ", Lift: " << rld::lift().y << ", Drag: " << glm::length(rld::drag()) << ", Torque: " << rld::torq().x << ", SPS: " << (1.0 / dt) << std::endl;
}

static void doAllAngles() {
    results::clearSlices();

    double then(glfwGetTime());

    int count(0);
    for (float angle(0.0f); angle <= k_maxAutoAoA; angle += k_autoAngleIncrement) {
        doFastSweep(angle);
        doFastSweep(-angle);
        count += 2;
    }

    double dt(glfwGetTime() - then);
    std::cout << "Average SPS: " << (double(count) / dt) << std::endl;
}

void doAngle(float angle) {
    if (glm::abs(angle) > 90.0f) {
        return;
    }

    s_shouldStep = false;
    s_shouldAutoProgress = false;
    
    if (angle == s_angleOfAttack && s_shouldSweep) {
        return;
    }
    
    rld::reset();
    setAngleOfAttack(angle);
    s_shouldSweep = true;
}

void startSeeking(int dir) {
    if (s_seekDir == dir) {
        return;
    }

    s_seekDir = dir;
    s_seekAngle = nearestVal(s_angleOfAttack, k_manualAngleIncrement);
    s_shouldStep = false;
    s_shouldSweep = false;
    s_shouldAutoProgress = false;
    rld::reset();
}

void stopSeeking() {
    s_seekDir = 0;
    s_shouldSweep = true;
}

void MainUIC::keyEvent(int key, int action, int mods) {
    Group::keyEvent(key, action, mods);

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
        if (rld::slice() == 0 && !s_shouldAutoProgress && !s_seekDir) {
            doFastSweep(s_angleOfAttack);
        }
    }
    // If Shift-F is pressed, do fast sweep of all angles
    else if (key == GLFW_KEY_F && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
        if (rld::slice() == 0 && !s_shouldAutoProgress && !s_seekDir) {
            doAllAngles();
        }
    }
    // If up arrow is pressed, increase angle of attack
    else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (rld::slice() == 0) {
            doAngle(nearestVal(s_angleOfAttack, k_manualAngleIncrement) + k_manualAngleIncrement);
        }
    }
    // If down arrow is pressed, decrease angle of attack
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (rld::slice() == 0) {
            doAngle(nearestVal(s_angleOfAttack, k_manualAngleIncrement) - k_manualAngleIncrement);
        }
    }
    // If left arrow is pressed, seek toward negative angles of attack
    else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && !mods) {
        startSeeking(-1);
    }
    // If right arrow is pressed, seek toward positive angles of attack
    else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && !mods) {
        startSeeking(1);
    }
    // If left or right arrows are released stop seeking
    else if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) && action == GLFW_RELEASE) {
        stopSeeking();
    }
    // If O key is pressed, increase rudder angle
    else if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && rld::slice() == 0) {
            changeRudderAngle(k_manualAngleIncrement);
        }
    }
    // If I key is pressed, decrease rudder angle
    else if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && rld::slice() == 0) {
            changeRudderAngle(-k_manualAngleIncrement);
        }
    }
    // If K key is pressed, increase elevator angle
    else if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && rld::slice() == 0) {
            changeElevatorAngle(k_manualAngleIncrement);
        }
    }
    // If J key is pressed, decrease elevator angle
    else if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && rld::slice() == 0) {
            changeElevatorAngle(-k_manualAngleIncrement);
        }
    }
    // If M key is pressed, increase aileron angle
    else if (key == GLFW_KEY_M && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && rld::slice() == 0) {
            changeAileronAngle(k_manualAngleIncrement);
        }
    }
    // If N key is pressed, decrease aileron angle
    else if (key == GLFW_KEY_N && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && rld::slice() == 0) {
            changeAileronAngle(-k_manualAngleIncrement);
        }
    }
    // If equals key is pressed, reset UI stuff
    else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS && !mods) {
        s_frontTexViewer->center(vec2(0.5f));
        s_frontTexViewer->zoomReset();
        s_sideTexViewer->center(vec2(0.5f));
        s_sideTexViewer->zoomReset();
        results::resetGraphs();
    }
}

static bool setupModel() {
    std::string modelsPath(g_resourcesDir + "/models/");
    switch (k_simModel) {
        case SimModel::airfoil: s_model = Model::load(modelsPath + "0012.obj"  ); break;
        case SimModel::    f18: s_model = Model::load(modelsPath + "f18_zero_normals.grl"   ); break;
        case SimModel:: sphere: s_model = Model::load(modelsPath + "sphere.obj"); break;
    }
    if (!s_model) {
        std::cerr << "Failed to load model" << std::endl;
        return false;
    }

    s_modelMat = mat4();

    switch (k_simModel) {
        case SimModel::airfoil:
            s_modelMat = glm::scale(mat4(), vec3(0.5f, 1.0f, 1.0f)) * s_modelMat;
            s_windframeWidth = 1.25f;
            s_windframeDepth = 1.5f;
            s_turbulenceDist = 0.045f;
            s_windShadDist = 0.1f;
            s_backforceC = 1000000.0f;
            s_flowback = 0.01f;
            s_initVelC = 0.5f;
            s_angleGraphRange = vec2(-3000.0f, 3000.0f);
            s_sliceGraphRange = vec2(-200.0f, 200.0f);
            break;

        case SimModel::f18:
            s_modelMat = glm::rotate(mat4(), glm::pi<float>(), vec3(0.0f, 0.0f, 1.0f)) * s_modelMat;
            s_windframeWidth = 14.5f;
            s_windframeDepth = 22.0f;
            s_turbulenceDist = 0.225f;
            s_windShadDist = 1.5f;
            s_backforceC = 50000.0f;
            s_flowback = 0.15f;
            s_initVelC = 1.0f;
            s_angleGraphRange = vec2(-500000, 500000.0f);
            s_sliceGraphRange = vec2(-50000.0f, 50000.0f);
            break;

        case SimModel::sphere:
            s_windframeWidth = 2.5f;
            s_windframeDepth = 2.5f;
            s_turbulenceDist = 0.045f;
            s_windShadDist = 0.1f;
            s_backforceC = 1000000.0f;
            s_flowback = 0.01f;
            s_initVelC = 0.5f;
            s_angleGraphRange = vec2(-10000.0f, 10000.0f);
            s_sliceGraphRange = vec2(-1000.0f, 1000.0f);
            break;
    }

    s_normalMat = glm::transpose(glm::inverse(s_modelMat));
    s_maxSearchDist = 2.0f * s_turbulenceDist;

    return true;
}

static bool setup() {
    // Setup UI, which includes GLFW and GLAD
    if (!ui::setup(k_defWindowSize, "Realtime Lift and Drag Visualizer", 4, 5, false)) {
        std::cerr << "Failed to setup UI" << std::endl;
        return false;
    }
    //ui::exitCallback(exitCallback);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    // Setup model
    if (!setupModel()) {
        std::cerr << "Failed to setup model" << std::endl;
        return false;
    }

    // Setup simulation
    if (!rld::setup(k_simTexSize, k_simSliceCount, k_simLiftC, k_simDragC, s_turbulenceDist, s_maxSearchDist, s_windShadDist, s_backforceC, s_flowback, s_initVelC)) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }

    if (!results::setup(k_simSliceCount, s_angleGraphRange, s_sliceGraphRange)) {
        std::cerr << "Failed to setup results" << std::endl;
        return false;
    }

    // Set connected state
    s_controller.poll();

    s_frontTexViewer.reset(new ui::TexViewer(rld::frontTex(), ivec2(rld::texSize()), ivec2(128)));
    s_turbTexViewer.reset(new ui::TexViewer(rld::turbulenceTex(), ivec2(rld::texSize()) / 4, ivec2(128)));
    s_sideTexViewer.reset(new ui::TexViewer(rld::sideTex(), ivec2(rld::texSize()), ivec2(128)));

    shr<ui::HorizontalGroup> displayGroup(new ui::HorizontalGroup());
    displayGroup->add(s_frontTexViewer);
    displayGroup->add(s_sideTexViewer);

    shr<ui::Graph> angleGraph(results::angleGraph());
    shr<ui::Graph> sliceGraph(results::sliceGraph());

    s_angleLabel      .reset(new ui::String("Angle: ", 1, vec4(1.0f)));
    s_angleLiftLabel  .reset(new ui::String("Lift: ", 1, vec4(1.0f)));
    s_angleDragLabel  .reset(new ui::String("Drag: ", 1, vec4(1.0f)));
    s_angleTorqueLabel.reset(new ui::String("Torq: ", 1, vec4(1.0f)));
    s_rudderLabel     .reset(new ui::String("Rudder: ", 1, vec4(1.0f)));
    s_elevatorLabel   .reset(new ui::String("Elevator: ", 1, vec4(1.0f)));
    s_aileronLabel    .reset(new ui::String("Aileron: ", 1, vec4(1.0f)));

    s_angleField    .reset(new ui::BoundedNumberField(0.0, 1, vec4(1.0f), 5, 0, true, 1, -90.0f, 90.0f));
    s_angleField->actionCallback([&](){
        if (rld::slice() == 0 && !s_shouldAutoProgress) {
            setAngleOfAttack(float(s_angleField->value()));
        }
    });
    s_angleLiftNum  .reset(new ui::Vector(vec3(0.0), vec4(1.0f), 10, false, 3));
    s_angleDragNum  .reset(new ui::Vector(vec3(0.0), vec4(1.0f), 10, false, 3));
    s_angleTorqueNum.reset(new ui::Vector(vec3(0.0), vec4(1.0f), 10, false, 3));
    s_rudderNum     .reset(new ui::Number(0.0, 1, vec4(1.0f), 4, 4, false, 2));
    s_elevatorNum   .reset(new ui::Number(0.0, 1, vec4(1.0f), 4, 4, false, 2));
    s_aileronNum    .reset(new ui::Number(0.0, 1, vec4(1.0f), 4, 4, false, 2));

    shr<ui::HorizontalGroup> f18Info(new ui::HorizontalGroup());
    f18Info->add(s_rudderLabel);
    f18Info->add(s_rudderNum);
    f18Info->add(s_elevatorLabel);
    f18Info->add(s_elevatorNum);
    f18Info->add(s_aileronLabel);
    f18Info->add(s_aileronNum);

    ivec2 textSize(Text::detDimensions(k_controlsString));
    shr<ui::Text> controlsText(new ui::Text(k_controlsString, ivec2(1, 0), vec4(1.0f), textSize, ivec2(textSize.x, 0)));

    shr<ui::VerticalGroup> infoGroup(new ui::VerticalGroup());
    infoGroup->add(controlsText);
    infoGroup->add(f18Info);
    shr<ui::HorizontalGroup> angleTorqueGroup(new ui::HorizontalGroup());
    angleTorqueGroup->add(s_angleTorqueLabel);
    angleTorqueGroup->add(s_angleTorqueNum);
    infoGroup->add(angleTorqueGroup);
    shr<ui::HorizontalGroup> angleDragGroup(new ui::HorizontalGroup());
    angleDragGroup->add(s_angleDragLabel);
    angleDragGroup->add(s_angleDragNum);
    infoGroup->add(angleDragGroup);
    shr<ui::HorizontalGroup> angleLiftGroup(new ui::HorizontalGroup());
    angleLiftGroup->add(s_angleLiftLabel);
    angleLiftGroup->add(s_angleLiftNum);
    infoGroup->add(angleLiftGroup);
    shr<ui::HorizontalGroup> angleGroup(new ui::HorizontalGroup());
    angleGroup->add(s_angleLabel);
    angleGroup->add(s_angleField);
    infoGroup->add(angleGroup);

    shr<ui::HorizontalGroup> bottomGroup(new ui::HorizontalGroup());
    bottomGroup->add(angleGraph);
    bottomGroup->add(sliceGraph);
    bottomGroup->add(infoGroup);

    s_mainUIC.reset(new MainUIC());
    s_mainUIC->add(bottomGroup);
    s_mainUIC->add(displayGroup);

    ui::setRootComponent(s_mainUIC);

    return true;
}

static void cleanup() {
    rld::cleanup();
    glfwTerminate();
}

static void updateInfoText() {
    s_angleField->value(s_angleOfAttack);
    s_angleLiftNum->value(rld::lift());
    s_angleDragNum->value(rld::drag());
    s_angleTorqueNum->value(rld::torq());}

static void updateF18InfoText() {
    s_rudderNum->value(s_rudderAngle);
    s_elevatorNum->value(s_elevatorAngle);
    s_aileronNum->value(s_aileronAngle);
}

void processController(float dt) {
    if (!s_controller.connected()) {
        return;
    }
    s_controller.poll();

    // Use stick to seek to an angle, release to sweep
    static int s_prevLStickXDir(0);
    int lStickXDir(glm::sign(s_controller.state().lStick.x));
    if (lStickXDir != s_prevLStickXDir) {
        s_prevLStickXDir = lStickXDir;
        if (lStickXDir) startSeeking(lStickXDir);
        else stopSeeking();
    }

    // Triggers increment or decrement angle of attack
    if (!rld::slice()) {
        float dir(glm::ceil(s_controller.state().rTrigger) - glm::ceil(s_controller.state().lTrigger));
        if (dir) {
            doAngle(nearestVal(s_angleOfAttack, k_manualAngleIncrement) + dir * k_manualAngleIncrement);
        }
    }

    static constexpr float s_interval(0.05f);
    static float s_time(s_interval);
    bool state(s_controller.state().a);
    if (state) {
        s_time += dt;
        if (s_time >= s_interval) {
            s_shouldStep = true;
            s_shouldSweep = false;
            s_shouldAutoProgress = false;
            s_time -= s_interval;
        }
    }
    else {
        s_time = s_interval;
    }
}

static void update(float dt) {
    if (s_seekDir) {
        s_seekAngle += float(s_seekDir) * k_seekSpeed * dt;
        setAngleOfAttack(nearestVal(s_seekAngle, k_manualAngleIncrement));
    }

    if (s_shouldStep || s_shouldSweep) {
        int slice(rld::slice());

        if (slice == 0) { // About to do first slice
            setSimulation(s_angleOfAttack, true);
            results::clearSlices();
        }

        glDisable(GL_BLEND); // can't have blending for simulation

        if (rld::step()) { // That was the last slice
            s_shouldSweep = false;
            vec3 lift(rld::lift());
            vec3 drag(rld::drag());
            vec3 torq(rld::torq());

            submitResults(s_angleOfAttack);

            if (s_shouldAutoProgress) {
                s_angleOfAttack += k_autoAngleIncrement;
                s_angleOfAttack = nearestVal(s_angleOfAttack, k_autoAngleIncrement);
                if (s_angleOfAttack > k_maxAutoAoA) {
                    s_angleOfAttack = nearestVal(-k_maxAutoAoA, k_autoAngleIncrement);
                    if (s_angleOfAttack < -k_maxAutoAoA) s_angleOfAttack += k_autoAngleIncrement;
                }
                s_shouldSweep = true;
                s_isInfoChange = true;
            }
        }

        glEnable(GL_BLEND);

        s_isInfoChange = true;
        s_shouldStep = false;
    }

    if (s_isInfoChange) {
        updateInfoText();
        results::angleGraph()->markX(s_angleOfAttack);
        s_isInfoChange = false;
    }
    if (s_isF18Change) {
        updateF18InfoText();
        s_isF18Change = false;
    }

    ui::update();

    // Make front and side textures line up
    if (s_frontTexViewer->contains(ui::cursorPosition())) {
        s_sideTexViewer->center(vec2(s_sideTexViewer->center().x, s_frontTexViewer->center().y));
        s_sideTexViewer->zoom(s_frontTexViewer->zoom());
    }
    else if (s_sideTexViewer->contains(ui::cursorPosition())) {
        s_frontTexViewer->center(vec2(s_frontTexViewer->center().x, s_sideTexViewer->center().y));
        s_frontTexViewer->zoom(s_sideTexViewer->zoom());
    }
}



int main(int argc, char ** argv) {
    if (!processArgs(argc, argv)) {
        std::exit(-1);
    }

    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }

    // Loop until the user closes the window.
    double then(glfwGetTime());
    while (!ui::shouldExit()) {
        double now(glfwGetTime());
        float dt(float(now - then));
        then = now;

        // Poll for and process events.
        ui::poll();
        processController(dt);

        update(dt);

        glDisable(GL_DEPTH_TEST); // don't want depth test for UI
        ui::render();
        glEnable(GL_DEPTH_TEST);
    }

    cleanup();

    return EXIT_SUCCESS;
}
