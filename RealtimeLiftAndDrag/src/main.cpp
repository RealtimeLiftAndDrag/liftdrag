﻿// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert


// Allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement(1);
}
#endif

// TODO: should average backforce when multiple geo pixels, or simply sum?
// TODO: lift should be independent of the number of slices



#include <iostream>
#include <sstream>

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
#include "UI.hpp"
#include "Text.hpp"
#include "Graph.hpp"
#include "TexViewer.hpp"
#include "Util.hpp"



class MainUIC : public UI::VerticalGroup {

    public:
    
    virtual void keyEvent(int key, int action, int mods) override;

};



enum class SimModel { airfoil, f18, sphere };



static constexpr SimModel k_simModel(SimModel::f18);

static const std::string k_defResourceDir("resources");

static const ivec2 k_defWindowSize(1280, 720);

static constexpr float k_minAngleOfAttack(-90.0f), k_maxAngleOfAttack(90.0f);
static constexpr float k_minRudderAngle(-90.0f), k_maxRudderAngle(90.0f);
static constexpr float k_minAileronAngle(-90.0f), k_maxAileronAngle(90.0f);
static constexpr float k_minElevatorAngle(-90.0f), k_maxElevatorAngle(90.0f);

static constexpr float k_angleOfAttackIncrement(1.0f); // the granularity of angle of attack
static constexpr float k_manualAngleIncrement(1.0f); // how many degrees to change the rudder, elevator, and ailerons by when using arrow keys
static constexpr float k_autoAngleIncrement(7.0f); // how many degrees to change the angle of attack by when auto progressing

static const std::string k_controlsString(
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



static std::string s_resourceDir(k_defResourceDir);


static unq<Model> s_model;
//all in degrees
static float s_angleOfAttack(0.0f);
static float s_aileronAngle(0.0f);
static float s_rudderAngle(0.0f);
static float s_elevatorAngle(0.0f);

static bool s_shouldStep(false);
static bool s_shouldSweep(true);
static bool s_shouldAutoProgress(false);

static shr<MainUIC> s_mainUIC;
static shr<TexViewer> s_frontTexViewer, s_sideTexViewer;
static shr<Text> s_angleText, s_angleLiftText, s_angleDragText;
static shr<Text> s_sliceText, s_sliceLiftText, s_sliceDragText;
static shr<Text> s_rudderText, s_elevatorText, s_aileronText;

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
        s_resourceDir = argv[1];
    }

    return true;
}

static void changeAngleOfAttack(float deltaAngle) {
    s_angleOfAttack = glm::clamp(s_angleOfAttack + deltaAngle, -k_maxAngleOfAttack, k_maxAngleOfAttack);

    s_isInfoChange = true;
}

static void changeRudderAngle(float deltaAngle) {
    s_rudderAngle = glm::clamp(s_rudderAngle + deltaAngle, -k_maxRudderAngle, k_maxRudderAngle);

    mat4 modelMat(glm::rotate(mat4(), s_rudderAngle, vec3(0.0f, 1.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("RudderL01")->localTransform(modelMat, normalMat);
    s_model->subModel("RudderR01")->localTransform(modelMat, normalMat);

    s_isF18Change = true;
}

static void changeAileronAngle(float deltaAngle) {
    s_aileronAngle = glm::clamp(s_aileronAngle + deltaAngle, -k_maxAileronAngle, k_maxAileronAngle);

    mat4 modelMat(glm::rotate(mat4(), s_aileronAngle, vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("AileronL01")->localTransform(modelMat, normalMat);

    modelMat = glm::rotate(mat4(), -s_aileronAngle, vec3(1.0f, 0.0f, 0.0f));
    normalMat = modelMat;
    s_model->subModel("AileronR01")->localTransform(modelMat, normalMat);
    
    s_isF18Change = true;
}

static void changeElevatorAngle(float deltaAngle) {
    s_elevatorAngle = glm::clamp(s_elevatorAngle + deltaAngle, -k_maxElevatorAngle, k_maxElevatorAngle);

    mat4 modelMat(glm::rotate(mat4(), s_elevatorAngle, vec3(1.0f, 0.0f, 0.0f)));
    mat3 normalMat(modelMat);
    s_model->subModel("ElevatorL01")->localTransform(modelMat, normalMat);
    s_model->subModel("ElevatorR01")->localTransform(modelMat, normalMat);
    
    s_isF18Change = true;
}

static void setSimulation(float angleOfAttack, bool debug) {
    mat4 modelMat;
    mat3 normalMat;
    float depth;
    vec3 centerOfGravity;

    switch (k_simModel) {
        case SimModel::airfoil:
            modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, 0.5f)) * modelMat;
            modelMat = glm::rotate(mat4(), glm::radians(-angleOfAttack), vec3(1.0f, 0.0f, 0.0f)) * modelMat;
            modelMat = glm::translate(mat4(), vec3(0.0f, 0.0f, -0.5f)) * modelMat;
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

static void doFastSweep(float angleOfAttack, bool submitSlices) {
    setSimulation(angleOfAttack, false);

    double then(glfwGetTime());
    Simulation::sweep();
    double dt(glfwGetTime() - then);

    vec3 lift(Simulation::lift());
    vec3 drag(Simulation::drag());
    Results::submitAngle(angleOfAttack, lift.y, drag.x);

    if (submitSlices) {
        for (int i(0); i < Simulation::sliceCount(); ++i) {
            Results::submitSlice(i, Simulation::lift(i).y, Simulation::drag(i).x);
        }
    }

    Results::update();

    std::cout << "Angle: " << angleOfAttack << ", Lift: " << lift.y << ", Drag: " << drag.x << ", SPS: " << (1.0 / dt) << std::endl;
}

static void doAllAngles() {
    Results::clearSlices();

    for (float angle(k_minAngleOfAttack); angle <= k_maxAngleOfAttack; angle += k_angleOfAttackIncrement) {
        doFastSweep(angle, false);
    }
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
        if (Simulation::slice() == 0 && !s_shouldAutoProgress) {
            doFastSweep(s_angleOfAttack, true);
        }
    }
    // If Shift-F is pressed, do fast sweep of all angles
    else if (key == GLFW_KEY_F && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
        if (Simulation::slice() == 0 && !s_shouldAutoProgress) {
            doAllAngles();
        }
    }
    // If up arrow is pressed, increase angle of attack
    else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::slice() == 0 && !s_shouldAutoProgress) {
            changeAngleOfAttack(k_angleOfAttackIncrement);
        }
    }
    // If down arrow is pressed, decrease angle of attack
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (Simulation::slice() == 0 && !s_shouldAutoProgress) {
            changeAngleOfAttack(-k_angleOfAttackIncrement);
        }
    }
    // If O key is pressed, increase rudder angle
    else if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && Simulation::slice() == 0) {
            changeRudderAngle(k_manualAngleIncrement);
        }
    }
    // If I key is pressed, decrease rudder angle
    else if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && Simulation::slice() == 0) {
            changeRudderAngle(-k_manualAngleIncrement);
        }
    }
    // If K key is pressed, increase elevator angle
    else if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && Simulation::slice() == 0) {
            changeElevatorAngle(k_manualAngleIncrement);
        }
    }
    // If J key is pressed, decrease elevator angle
    else if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && Simulation::slice() == 0) {
            changeElevatorAngle(-k_manualAngleIncrement);
        }
    }
    // If M key is pressed, increase aileron angle
    else if (key == GLFW_KEY_M && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && Simulation::slice() == 0) {
            changeAileronAngle(k_manualAngleIncrement);
        }
    }
    // If N key is pressed, decrease aileron angle
    else if (key == GLFW_KEY_N && (action == GLFW_PRESS || action == GLFW_REPEAT) && !mods) {
        if (k_simModel == SimModel::f18 && Simulation::slice() == 0) {
            changeAileronAngle(-k_manualAngleIncrement);
        }
    }
    // If equals key is pressed, reset UI stuff
    else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS && !mods) {
        s_frontTexViewer->center(vec2(0.5f));
        s_frontTexViewer->zoom(1.0f);
        s_sideTexViewer->center(vec2(0.5f));
        s_sideTexViewer->zoom(1.0f);
        Results::resetGraphs();
    }
}

static bool setup() {
    // Setup UI, which includes GLFW and GLAD
    if (!UI::setup(k_defWindowSize, "Realtime Lift and Drag Visualizer", 4, 5, false, s_resourceDir)) {
        std::cerr << "Failed to setup UI" << std::endl;
        return false;
    }

    glDisable(GL_BLEND); // need blending for ui but don't want it for simulation

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

    if (!Results::setup(s_resourceDir)) {
        std::cerr << "Failed to setup results" << std::endl;
        return false;
    }

    s_frontTexViewer.reset(new TexViewer(Simulation::frontTex(), ivec2(Simulation::size()), ivec2(128)));
    s_sideTexViewer.reset(new TexViewer(Simulation::sideTex(), ivec2(Simulation::size()), ivec2(128)));

    shr<UI::HorizontalGroup> displayGroup(new UI::HorizontalGroup());
    displayGroup->add(s_frontTexViewer);
    displayGroup->add(s_sideTexViewer);

    shr<Graph> angleGraph(Results::angleGraph());

    shr<Graph> sliceGraph(Results::sliceGraph());

    const int k_infoWidth(16);
    s_angleText    .reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_angleLiftText.reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_angleDragText.reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_sliceText    .reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_sliceLiftText.reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_sliceDragText.reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_rudderText   .reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_elevatorText .reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));
    s_aileronText  .reset(new Text("", ivec2(1, 0), vec3(1.0f), ivec2(k_infoWidth, 1), ivec2(0, 1)));

    shr<UI::HorizontalGroup> angleInfo(new UI::HorizontalGroup());
    angleInfo->add(s_angleText);
    angleInfo->add(s_angleLiftText);
    angleInfo->add(s_angleDragText);

    shr<UI::HorizontalGroup> sliceInfo(new UI::HorizontalGroup());
    sliceInfo->add(s_sliceText);
    sliceInfo->add(s_sliceLiftText);
    sliceInfo->add(s_sliceDragText);

    shr<UI::HorizontalGroup> f18Info(new UI::HorizontalGroup());
    f18Info->add(s_rudderText);
    f18Info->add(s_elevatorText);
    f18Info->add(s_aileronText);

    ivec2 textSize(Text::detDimensions(k_controlsString));
    shr<Text> controlsText(new Text(k_controlsString, ivec2(1, 0), vec3(1.0f), textSize, ivec2(textSize.x, 0)));

    shr<UI::VerticalGroup> infoGroup(new UI::VerticalGroup());
    infoGroup->add(controlsText);
    infoGroup->add(f18Info);
    infoGroup->add(sliceInfo);
    infoGroup->add(angleInfo);

    shr<UI::HorizontalGroup> bottomGroup(new UI::HorizontalGroup());
    bottomGroup->add(angleGraph);
    bottomGroup->add(sliceGraph);
    bottomGroup->add(infoGroup);

    s_mainUIC.reset(new MainUIC());
    s_mainUIC->add(bottomGroup);
    s_mainUIC->add(displayGroup);

    UI::setRootComponent(s_mainUIC);

    return true;
}

static void cleanup() {
    Simulation::cleanup();
    glfwTerminate();
}

static void updateInfoText() {
    std::stringstream ss;

    ss << "Angle: " << Util::numberString(s_angleOfAttack, 2) << "\u00B0";
    s_angleText->string(ss.str());

    ss.str(std::string());

    ss << "Lift: " << Util::numberString(Simulation::lift().y, 3);
    s_angleLiftText->string(ss.str());

    ss.str(std::string());

    ss << "Drag: " << Util::numberString(Simulation::drag().x, 3);
    s_angleDragText->string(ss.str());

    ss.str(std::string());
    
    int slice(Simulation::slice() - 1);

    ss << "Slice: " << slice;
    s_sliceText->string(ss.str());

    ss.str(std::string());

    ss << "Lift: " << Util::numberString(slice >= 0 ? Simulation::lift(slice).y : 0.0f, 3);
    s_sliceLiftText->string(ss.str());

    ss.str(std::string());

    ss << "Drag: " << Util::numberString(slice >= 0 ? Simulation::drag(slice).x : 0.0f, 3);
    s_sliceDragText->string(ss.str());
}

static void updateF18InfoText() {
    std::stringstream ss;

    ss << "Rudder: " << Util::numberString(s_rudderAngle, 2) << "\u00B0";
    s_rudderText->string(ss.str());

    ss.str(std::string());

    ss << "Elevator: " << Util::numberString(s_elevatorAngle, 2) << "\u00B0";
    s_elevatorText->string(ss.str());

    ss.str(std::string());

    ss << "Aileron: " << Util::numberString(s_aileronAngle, 2) << "\u00B0";
    s_aileronText->string(ss.str());
}

static void update() {
    if (s_shouldStep || s_shouldSweep) {
        int slice(Simulation::slice());

        if (slice == 0) { // About to do first slice
            setSimulation(s_angleOfAttack, true);
            Results::clearSlices();
        }

        if (Simulation::step()) { // That was the last slice
            s_shouldSweep = false;
            vec3 lift(Simulation::lift());
            vec3 drag(Simulation::drag());

            Results::submitAngle(s_angleOfAttack, lift.y, drag.x);
            Results::update();

            if (s_shouldAutoProgress) {
                s_angleOfAttack += k_autoAngleIncrement;
                s_angleOfAttack = std::fmod(s_angleOfAttack - k_minAngleOfAttack, k_maxAngleOfAttack - k_minAngleOfAttack) + k_minAngleOfAttack;
                s_shouldSweep = true;
            }
        }

        s_isInfoChange = true;

        Results::submitSlice(slice, Simulation::lift(slice).y, Simulation::drag(slice).x);
        Results::update();

        s_shouldStep = false;
    }

    if (s_isInfoChange) {
        updateInfoText();
        s_isInfoChange = false;
    }
    if (s_isF18Change) {
        updateF18InfoText();
        s_isF18Change = false;
    }

    UI::update();

    // Make front and side textures line up
    if (s_frontTexViewer->contains(UI::cursorPosition())) {
        s_sideTexViewer->center(vec2(s_sideTexViewer->center().x, s_frontTexViewer->center().y));
        s_sideTexViewer->zoom(s_frontTexViewer->zoom());
    }
    else if (s_sideTexViewer->contains(UI::cursorPosition())) {
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
        return -1;
    }

    int fps(0);
    double then(glfwGetTime());
    
    // Loop until the user closes the window.
    while (!UI::shouldClose()) {
        // Poll for and process events.
        UI::poll();

        update();
        
        glEnable(GL_BLEND); // need blending for ui but don't want it for simulation
        UI::render();
        glDisable(GL_BLEND);

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